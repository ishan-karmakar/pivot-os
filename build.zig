const std = @import("std");
const Step = std.Build.Step;
const Options = Step.Options;

const KERNEL_TARGET_QUERY = std.Target.Query{
    .cpu_arch = .x86_64,
    .abi = .none,
    .os_tag = .freestanding,
    .cpu_features_add = std.Target.x86.featureSet(&.{.soft_float}),
    .cpu_features_sub = std.Target.x86.featureSet(&.{
        .mmx,
        .sse,
        .sse2,
        .avx,
        .avx2,
    }),
};

const MODULE_TARGET_QUERY = std.Target.Query{
    .cpu_arch = .x86_64,
    .abi = .none,
    .os_tag = .freestanding,
    .cpu_features_add = std.Target.x86.featureSet(&.{}),
    .cpu_features_sub = std.Target.x86.featureSet(&.{}),
};

const ISO_COPY = [_][]const u8{ "limine.conf", "u_vga16.sfn" };
const MODULES_DIR = "src/modules";
const ISO_MODULES_DIR = "kmod";

const XORRISO_ARGS = .{
    "xorriso",
    "-as",
    "mkisofs",
    "-b",
    "limine-bios-cd.bin",
    "-no-emul-boot",
    "-boot-load-size",
    "4",
    "-boot-info-table",
    "--efi-boot",
    "limine-uefi-cd.bin",
    "-efi-boot-part",
    "--efi-boot-image",
    "--protective-msdos-label",
};

const QEMU_ARGS = .{
    "qemu-system-x86_64",
    "-m",
    "128M",
    "-smp",
    "2",
    "-bios",
    "OVMF.fd",
    "-serial",
    "stdio",
    "-no-reboot",
    "-no-shutdown",
    "-nic",
    "none",
};

var limineZigModule: *std.Build.Module = undefined;
var limineBinDep: *std.Build.Dependency = undefined;
var limineBuildExeStep: *Step = undefined;

var uacpiModule: *std.Build.Module = undefined;
var uacpiCSourceFileOptions: std.Build.Module.AddCSourceFilesOptions = undefined;
var uacpiIncludePath: std.Build.LazyPath = undefined;

var ssfnModule: *std.Build.Module = undefined;
var ssfnCSourceFile: std.Build.Module.CSourceFile = undefined;
var ssfnIncludePath: std.Build.LazyPath = undefined;

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    initSSFN(b, optimize);
    initUACPI(b, optimize);
    initLimine(b);

    createNoQEMUPipeline(b, optimize);
    createQEMUPipeline(b, optimize);
}

fn createNoQEMUPipeline(b: *std.Build, optimize: std.builtin.OptimizeMode) void {
    const options = b.addOptions();
    options.addOption(bool, "qemu", false);
    const kernel = getKernelStep(b, optimize, options); // Options implicitly added as dependency
    const iso_dir = getISODirStep(b, kernel); // Implicitly add kernel as dependency
    const iso_out = getXorrisoStep(b, iso_dir.getDirectory()); // Implictly added iso_dir as dependency
    const install_iso = b.addInstallBinFile(iso_out, "os.iso");
    install_iso.step.dependOn(getLimineBiosStep(b, iso_out));

    b.getInstallStep().dependOn(&b.addInstallArtifact(kernel, .{}).step);
    b.getInstallStep().dependOn(&install_iso.step);
}

fn createQEMUPipeline(b: *std.Build, optimize: std.builtin.OptimizeMode) void {
    const options = b.addOptions();
    options.addOption(bool, "qemu", true);
    const kernel = getKernelStep(b, optimize, options);
    const iso_dir = getISODirStep(b, kernel);
    const iso_out = getXorrisoStep(b, iso_dir.getDirectory());
    const install_iso = b.addInstallBinFile(iso_out, "os.iso");
    install_iso.step.dependOn(getLimineBiosStep(b, iso_out));

    const qemu = b.addSystemCommand(&QEMU_ARGS);
    qemu.step.dependOn(&install_iso.step);
    qemu.addArg("-cdrom");
    qemu.addFileArg(iso_out);
    const run_step = b.step("run", "Run with QEMU emulator");
    run_step.dependOn(&qemu.step);
    run_step.dependOn(&b.addInstallArtifact(kernel, .{}).step);
}

fn getLimineBiosStep(b: *std.Build, iso: std.Build.LazyPath) *Step {
    const step = b.addSystemCommand(&.{ "./limine", "bios-install" });
    step.step.dependOn(limineBuildExeStep);
    step.setCwd(limineBinDep.path(""));
    step.addFileArg(iso);
    return &step.step;
}

fn getXorrisoStep(b: *std.Build, dir: std.Build.LazyPath) std.Build.LazyPath {
    const xorriso = b.addSystemCommand(&XORRISO_ARGS);
    xorriso.addDirectoryArg(dir);
    xorriso.addArg("-o");
    return xorriso.addOutputFileArg("os.iso");
}

fn getISODirStep(b: *std.Build, kernel: *Step.Compile) *Step.WriteFile {
    const wf = b.addWriteFiles();
    _ = wf.addCopyDirectory(
        limineBinDep.path(""),
        "",
        .{ .include_extensions = &.{
            "sys",
            "bin",
        } },
    );
    _ = wf.addCopyDirectory(
        limineBinDep.path(""),
        "EFI/BOOT",
        .{ .include_extensions = &.{"EFI"} },
    );
    _ = wf.addCopyFile(kernel.getEmittedBin(), "pivot-os");
    for (ISO_COPY) |f| _ = wf.addCopyFile(b.path(f), f);
    return wf;
}

fn getKernelStep(b: *std.Build, optimize: std.builtin.OptimizeMode, options: *Options) *Step.Compile {
    const kernel = b.addExecutable(.{
        .name = "pivot-os",
        .root_source_file = b.path("src/main.zig"),
        .target = b.resolveTargetQuery(KERNEL_TARGET_QUERY),
        .optimize = optimize,
        .code_model = .kernel,
        .linkage = .static,
        .strip = false,
    });
    kernel.want_lto = false;
    kernel.setLinkerScript(b.path("linker.ld"));
    kernel.addCSourceFiles(uacpiCSourceFileOptions);
    kernel.addCSourceFile(ssfnCSourceFile);
    kernel.addIncludePath(uacpiIncludePath);
    kernel.addIncludePath(ssfnIncludePath);

    kernel.root_module.addImport("config", options.createModule());
    kernel.root_module.addImport("limine", limineZigModule);
    kernel.root_module.addImport("ssfn", ssfnModule);
    kernel.root_module.addImport("uacpi", uacpiModule);
    kernel.root_module.addImport("kernel", &kernel.root_module);
    return kernel;
}

fn initSSFN(b: *std.Build, optimize: std.builtin.OptimizeMode) void {
    const dep = b.dependency("ssfn", .{});
    ssfnCSourceFile = .{ .file = b.path("src/ssfn.c") };
    const translateC = b.addTranslateC(.{
        .link_libc = false,
        .optimize = optimize,
        .target = b.resolveTargetQuery(KERNEL_TARGET_QUERY),
        .root_source_file = dep.path("ssfn.h"),
    });
    ssfnModule = translateC.createModule();
    ssfnIncludePath = dep.path("");
}

fn initUACPI(b: *std.Build, optimize: std.builtin.OptimizeMode) void {
    const uacpi = b.dependency("uacpi", .{});
    const translateC = b.addTranslateC(.{
        .link_libc = false,
        .optimize = optimize,
        .target = b.resolveTargetQuery(KERNEL_TARGET_QUERY),
        .root_source_file = b.path("src/uacpi.h"),
    });
    uacpiIncludePath = uacpi.path("include");
    translateC.addIncludeDir(uacpiIncludePath.getPath(b)); // This is a hack. Specifically states that should only be called during make phase
    uacpiModule = translateC.createModule();
    uacpiCSourceFileOptions = .{
        .root = uacpi.path("source"),
        .files = &.{
            "default_handlers.c",
            "event.c",
            "interpreter.c",
            "io.c",
            "mutex.c",
            "namespace.c",
            "notify.c",
            "opcodes.c",
            "opregion.c",
            "osi.c",
            "registers.c",
            "resources.c",
            "shareable.c",
            "sleep.c",
            "stdlib.c",
            "tables.c",
            "types.c",
            "uacpi.c",
            "utilities.c",
        },
        .flags = &.{
            "-DUACPI_SIZED_FREES",
            "-DUACPI_DEFAULT_LOG_LEVEL=UACPI_LOG_TRACE",
        },
    };
}

fn initLimine(b: *std.Build) void {
    limineZigModule = b.dependency("limine_zig", .{}).module("limine");
    limineBinDep = b.dependency("limine_bin", .{});

    const buildLimineExe = b.addSystemCommand(&.{"make"});
    buildLimineExe.setCwd(limineBinDep.path(""));
    limineBuildExeStep = &buildLimineExe.step;
}

fn getModulesStep(b: *std.Build, iso_dir: *std.Build.Step.WriteFile, optimize: std.builtin.OptimizeMode) !void {
    const mod_root = try std.fs.cwd().openDir(MODULES_DIR, .{ .iterate = true });
    var iter = mod_root.iterate();
    while (try iter.next()) |entry| {
        const mainFile = b.pathJoin(&.{ MODULES_DIR, entry.name, "main.zig" });
        const exe = b.addExecutable(.{
            .name = try b.allocator.dupe(u8, entry.name),
            .root_source_file = b.path(mainFile),
            .target = b.resolveTargetQuery(MODULE_TARGET_QUERY),
            .optimize = optimize,
            .strip = true,
        });
        exe.root_module.addImport("module", &exe.root_module);
        _ = iso_dir.addCopyFile(exe.getEmittedBin(), b.pathJoin(&.{ ISO_MODULES_DIR, entry.name }));
        iso_dir.step.dependOn(&b.addInstallArtifact(exe, .{ .dest_sub_path = b.pathJoin(&.{ "modules", exe.name }) }).step);
    }
}
