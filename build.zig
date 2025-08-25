const std = @import("std");
const Step = std.Build.Step;

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

const ISO_COPY = [_][]const u8{ "limine.conf", "u_vga16.sfn" };

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
    "-enable-kvm",
    "-cpu",
    "host,+invtsc",
};

const UACPI_SOURCES = &.{
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
};

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    createNoQEMUPipeline(b, optimize);
    createQEMUPipeline(b, optimize);
}

fn createQEMUPipeline(b: *std.Build, optimize: std.builtin.OptimizeMode) void {
    const options = b.addOptions();
    options.addOption(bool, "qemu", true);
    const kernel = createKernelStep(b, options, optimize);
    const iso_out = createISOStep(kernel);
    createQEMUStep(b, iso_out);
}

fn createNoQEMUPipeline(b: *std.Build, optimize: std.builtin.OptimizeMode) void {
    const options = b.addOptions();
    options.addOption(bool, "qemu", false);
    const kernel = createKernelStep(b, options, optimize);
    const iso_out = createISOStep(kernel);
    b.getInstallStep().dependOn(&b.addInstallBinFile(iso_out, "os.iso").step);
}

fn createQEMUStep(b: *std.Build, iso_out: std.Build.LazyPath) void {
    const qemu = b.addSystemCommand(&QEMU_ARGS);
    qemu.addArg("-cdrom");
    qemu.addFileArg(iso_out);
    const run_step = b.step("run", "Run with QEMU emulator");
    run_step.dependOn(&qemu.step);
}

fn createISOStep(kernel: *Step.Compile) std.Build.LazyPath {
    const limineBinDep = kernel.step.owner.dependency("limine_bin", .{});
    const wf = kernel.step.owner.addWriteFiles();
    _ = wf.addCopyDirectory(
        limineBinDep.path(""),
        "",
        .{ .include_extensions = &.{ "sys", "bin" } },
    );
    _ = wf.addCopyDirectory(
        limineBinDep.path(""),
        "EFI/BOOT",
        .{ .include_extensions = &.{"EFI"} },
    );
    _ = wf.addCopyFile(kernel.getEmittedBin(), "pivot-os");
    for (ISO_COPY) |f| _ = wf.addCopyFile(kernel.step.owner.path(f), f);

    const xorriso = kernel.step.owner.addSystemCommand(&XORRISO_ARGS);
    xorriso.addDirectoryArg(wf.getDirectory());
    xorriso.addArg("-o");
    const iso_out = xorriso.addOutputFileArg("os.iso");

    const makeLimineStep = kernel.step.owner.addSystemCommand(&.{"make"});
    const limineBiosStep = kernel.step.owner.addSystemCommand(&.{ "./limine", "bios-install" });
    limineBiosStep.step.dependOn(&makeLimineStep.step);
    makeLimineStep.setCwd(limineBinDep.path(""));
    limineBiosStep.setCwd(limineBinDep.path(""));
    limineBiosStep.addFileArg(iso_out);
    iso_out.addStepDependencies(&limineBiosStep.step);
    return iso_out;
}

fn createKernelStep(b: *std.Build, options: *Step.Options, optimize: std.builtin.OptimizeMode) *Step.Compile {
    const kernel = b.addExecutable(.{
        .name = "pivot-os",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = b.resolveTargetQuery(KERNEL_TARGET_QUERY),
            .optimize = optimize,
            .code_model = .kernel,
        }),
        // There seems to be a bug in the custom x86 backend where soft-float causes a segfault
        // Until that is fixed we are sticking with the LLVM backend
        .use_llvm = true,
    });

    kernel.setLinkerScript(b.path("linker.ld"));
    kernel.root_module.addImport("config", options.createModule());
    addSSFN(kernel);
    addLimine(kernel);
    addUACPI(kernel);
    return kernel;
}

fn addSSFN(kernel: *Step.Compile) void {
    const dep = kernel.step.owner.dependency("ssfn", .{});
    kernel.addCSourceFile(.{ .file = kernel.step.owner.path("src/ssfn.c") });
    kernel.root_module.addImport("ssfn", kernel.step.owner.addTranslateC(.{
        .link_libc = false,
        .optimize = kernel.root_module.optimize.?,
        .target = kernel.root_module.resolved_target.?,
        .root_source_file = dep.path("ssfn.h"),
    }).createModule());
    kernel.addIncludePath(dep.path(""));
}

fn addUACPI(kernel: *Step.Compile) void {
    const uacpi = kernel.step.owner.dependency("uacpi", .{});
    kernel.addCSourceFiles(.{
        .root = uacpi.path("source"),
        .files = UACPI_SOURCES,
        .flags = &.{
            "-DUACPI_SIZED_FREES",
            "-DUACPI_DEFAULT_LOG_LEVEL=UACPI_LOG_INFO",
        },
    });
    const translateC = kernel.step.owner.addTranslateC(.{
        .link_libc = false,
        .optimize = kernel.root_module.optimize.?,
        .target = kernel.root_module.resolved_target.?,
        .root_source_file = kernel.step.owner.path("src/uacpi.h"),
    });
    translateC.addIncludePath(uacpi.path("include"));
    kernel.root_module.addImport("uacpi", translateC.createModule());
    kernel.addIncludePath(uacpi.path("include"));
}

fn addLimine(kernel: *Step.Compile) void {
    const limineZigMod = kernel.step.owner.dependency("limine_zig", .{
        .api_revision = 3,
        .allow_deprecated = false,
        .no_pointers = false,
    }).module("limine");
    kernel.root_module.addImport("limine", limineZigMod);
}
