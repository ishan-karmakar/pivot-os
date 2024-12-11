const std = @import("std");
const Step = std.Build.Step;
const Options = Step.Options;

const CPU_FEATURES_ADD = std.Target.x86.featureSet(&.{
    .soft_float,
});

const CPU_FEATURES_SUB = std.Target.x86.featureSet(&.{
    .mmx,
    .sse,
    .sse2,
    .avx,
    .avx2,
});

const CPU_FEATURES_QUERY = std.Target.Query{
    .cpu_arch = .x86_64,
    .abi = .none,
    .os_tag = .freestanding,
    .cpu_features_add = CPU_FEATURES_ADD,
    .cpu_features_sub = CPU_FEATURES_SUB,
};

const ISO_COPY: [1][]const u8 = .{"limine.conf"};

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
    "-serial",
    "stdio",
    "-no-reboot",
    "-no-shutdown",
    "-enable-kvm",
    "-cpu",
    "host,+tsc,+invtsc",
    "-bios",
    "OVMF.fd",
};

var limineZigModule: *std.Build.Module = undefined;
var limineBinDep: *std.Build.Dependency = undefined;
var flantermModule: *std.Build.Module = undefined;
var flantermCSourceFileOptions: std.Build.Module.AddCSourceFilesOptions = undefined;

pub fn build(b: *std.Build) void {
    const target = b.resolveTargetQuery(CPU_FEATURES_QUERY);
    const optimize = b.standardOptimizeOption(.{});
    initFlanterm(b, target, optimize);
    initLimine(b);

    createNoQEMUPipeline(b, target, optimize);
    createQEMUPipeline(b, target, optimize);
}

fn createNoQEMUPipeline(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) void {
    const options = b.addOptions();
    options.addOption(bool, "qemu", false);
    const kernel = getKernelStep(b, target, optimize, options); // Options implicitly added as dependency
    const iso_dir = getISODirStep(b, kernel); // Implicitly add kernel as dependency
    const iso_out = getXorrisoStep(b, iso_dir.getDirectory()); // Implictly added iso_dir as dependency

    b.getInstallStep().dependOn(&b.addInstallArtifact(kernel, .{}).step);
    b.getInstallStep().dependOn(&b.addInstallBinFile(iso_out, "os.iso").step);
}

fn createQEMUPipeline(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) void {
    const options = b.addOptions();
    options.addOption(bool, "qemu", true);
    const kernel = getKernelStep(b, target, optimize, options);
    const iso_dir = getISODirStep(b, kernel);
    const iso_out = getXorrisoStep(b, iso_dir.getDirectory());

    const qemu = b.addSystemCommand(&QEMU_ARGS);
    qemu.addArg("-cdrom");
    qemu.addFileArg(iso_out);
    const run_step = b.step("run", "Run with QEMU emulator");
    run_step.dependOn(&qemu.step);

    run_step.dependOn(&b.addInstallArtifact(kernel, .{ .dest_sub_path = "pivot-os-qemu" }).step);
    run_step.dependOn(&b.addInstallBinFile(iso_out, "os-qemu.iso").step);
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

fn getKernelStep(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode, options: *Options) *Step.Compile {
    const kernel = b.addExecutable(.{
        .name = "pivot-os",
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
        .code_model = .kernel,
        .linkage = .static,
    });
    kernel.want_lto = false;

    kernel.root_module.addImport("config", options.createModule());
    kernel.root_module.addImport("limine", limineZigModule);
    kernel.root_module.addImport("flanterm", flantermModule);
    kernel.addCSourceFiles(flantermCSourceFileOptions);
    return kernel;
}

fn initFlanterm(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) void {
    const flanterm = b.dependency("flanterm", .{});
    const translateC = b.addTranslateC(.{
        .link_libc = false,
        .optimize = optimize,
        .target = target,
        .root_source_file = flanterm.path("backends/fb.h"),
    });
    flantermModule = translateC.createModule();
    flantermCSourceFileOptions = .{
        .root = flanterm.path(""),
        .files = &.{ "flanterm.c", "backends/fb.c" },
        .flags = &.{"-DFLANTERM_FB_DISABLE_BUMP_ALLOC"},
    };
}

fn initLimine(b: *std.Build) void {
    limineZigModule = b.dependency("limine_zig", .{}).module("limine");
    limineBinDep = b.dependency("limine_bin", .{});
}

// const ISO_OUT = "os.iso";
// const KERNEL_NAME = "pivot-os";
// const ISO_COPY: [1][]const u8 = .{
//     "limine.conf",
// };
// const XORRISO_ARGS = .{
//     "xorriso",
//     "-as",
//     "mkisofs",
//     "-b",
//     "limine-bios-cd.bin",
//     "-no-emul-boot",
//     "-boot-load-size",
//     "4",
//     "-boot-info-table",
//     "--efi-boot",
//     "limine-uefi-cd.bin",
//     "-efi-boot-part",
//     "--efi-boot-image",
//     "--protective-msdos-label",
// };
// const QEMU_ARGS = .{
//     "qemu-system-x86_64",
//     "-m",
//     "128M",
//     "-smp",
//     "2",
//     "-serial",
//     "stdio",
//     "-no-reboot",
//     "-no-shutdown",
//     "-enable-kvm",
//     "-cpu",
//     "host,+tsc,+invtsc",
//     "-bios",
//     "OVMF.fd",
// };

// pub fn build(b: *std.Build) void {
//     const options = b.addOptions();

//     const kernel = b.addExecutable(.{
//         .name = KERNEL_NAME,
//         .root_source_file = b.path("src/main.zig"),
//         .target = target,
//         .optimize = optimize,
//         .code_model = .kernel,
//         .linkage = .static,
//     });
//     kernel.want_lto = false;
//     kernel.setLinkerScript(b.path("linker.ld"));
//     const limineModule = b.dependency("limine_zig", .{}).module("limine");

//     // const uacpi = b.dependency("uacpi", .{});
//     kernel.root_module.addImport("limine", limineModule);
//     kernel.root_module.addImport("kernel", &kernel.root_module);
//     kernel.root_module.addImport("config", options.createModule());

//     // kernel.addCSourceFiles(.{
//     //     .root = uacpi.path("source"),
//     //     .files = &.{
//     //         "default_handlers.c",
//     //         "event.c",
//     //         "interpreter.c",
//     //         "io.c",
//     //         "mutex.c",
//     //         "namespace.c",
//     //         "notify.c",
//     //         "opcodes.c",
//     //         "opregion.c",
//     //         "osi.c",
//     //         "registers.c",
//     //         "resources.c",
//     //         "shareable.c",
//     //         "sleep.c",
//     //         "stdlib.c",
//     //         "tables.c",
//     //         "types.c",
//     //         "uacpi.c",
//     //         "utilities.c",
//     //     },
//     //     .flags = &.{ "-DUACPI_SIZED_FREES", "-DUACPI_KERNEL_INITIALIZATION" },
//     // });
//     // kernel.addIncludePath(uacpi.path("include"));

//     // const uacpiTranslate = b.addTranslateC(.{
//     //     .link_libc = false,
//     //     .target = target,
//     //     .optimize = optimize,
//     //     .root_source_file = b.path("src/drivers/uacpi.h"),
//     // });
//     // uacpiTranslate.addIncludeDir(uacpi.path("include").getPath(b));
//     // kernel.root_module.addImport("uacpi", uacpiTranslate.addModule("uacpi"));

//     b.installArtifact(kernel); // Installing kernel so we can examine it for debugging

//     const wf = createISODir(b, kernel);
//     const iso = runXorriso(b, wf.getDirectory());
//     const non_qemu_config_step = BuildQEMUConfigStep.create(b, options, false);
//     b.getInstallStep().dependOn(&iso.step);
//     b.getInstallStep().dependOn(&non_qemu_config_step.step);
//     const qemu_run = b.addSystemCommand(&QEMU_ARGS);
//     qemu_run.addArg("-cdrom");
//     qemu_run.addFileArg(iso.source);
//     qemu_run.step.dependOn(&iso.step);
//     const qemu_step = b.step("run", "Run the kernel with QEMU");
//     qemu_step.dependOn(&qemu_run.step);
//     var qemu_config_step = BuildQEMUConfigStep.create(b, options, true);
//     qemu_step.dependOn(&qemu_config_step.step);

//     b.installDirectory(.{
//         .source_dir = kernel.getEmittedDocs(),
//         .install_dir = .prefix,
//         .install_subdir = "docs",
//     });
// }

// fn createISODir(b: *std.Build, kernel: *std.Build.Step.Compile) *std.Build.Step.WriteFile {
//     const limine = b.dependency("limine_c", .{});
//     const wf = b.addWriteFiles();
//     _ = wf.addCopyDirectory(
//         limine.path(""),
//         "",
//         .{ .include_extensions = &.{
//             "sys",
//             "bin",
//         } },
//     );
//     _ = wf.addCopyDirectory(
//         limine.path(""),
//         "EFI/BOOT",
//         .{ .include_extensions = &.{"EFI"} },
//     );

//     _ = wf.addCopyFile(kernel.getEmittedBin(), KERNEL_NAME);

//     for (ISO_COPY) |f| {
//         _ = wf.addCopyFile(b.path(f), f);
//     }
//     return wf;
// }

// fn runXorriso(b: *std.Build, iso_dir: std.Build.LazyPath) *std.Build.Step.InstallFile {
//     const xorriso = b.addSystemCommand(&XORRISO_ARGS);
//     xorriso.addDirectoryArg(iso_dir);
//     xorriso.addArg("-o");
//     return b.addInstallBinFile(xorriso.addOutputFileArg(ISO_OUT), ISO_OUT);
// }

// const BuildQEMUConfigStep = struct {
//     step: Step,
//     options: *Options,
//     qemu: bool,

//     pub fn create(owner: *std.Build, options: *Options, qemu: bool) *@This() {
//         const qemu_config = owner.allocator.create(@This()) catch @panic("OOM");
//         qemu_config.* = .{
//             .step = Step.init(.{
//                 .id = .custom,
//                 .name = "build qemu config",
//                 .owner = owner,
//                 .makeFn = make,
//             }),
//             .options = options,
//             .qemu = qemu,
//         };
//         return qemu_config;
//     }

//     fn make(step: *Step, prog_node: std.Progress.Node) !void {
//         const base: *@This() = @fieldParentPtr("step", step);
//         base.options.addOption(bool, "qemu", base.qemu);
//         prog_node.completeOne();
//     }
// };
