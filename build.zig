const std = @import("std");

const ISO_OUT = "os.iso";
const KERNEL_NAME = "pivot-os";
const ISO_COPY: [1][]const u8 = .{
    "limine.conf",
};
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
    "-bios",
    "OVMF.fd",
};

pub fn build(b: *std.Build) void {
    var cpu_features_add = std.Target.Cpu.Feature.Set.empty;
    var cpu_features_sub = std.Target.Cpu.Feature.Set.empty;
    cpu_features_add.addFeature(@intFromEnum(std.Target.x86.Feature.soft_float));
    cpu_features_sub.addFeature(@intFromEnum(std.Target.x86.Feature.mmx));
    cpu_features_sub.addFeature(@intFromEnum(std.Target.x86.Feature.sse));
    cpu_features_sub.addFeature(@intFromEnum(std.Target.x86.Feature.sse2));
    cpu_features_sub.addFeature(@intFromEnum(std.Target.x86.Feature.avx));
    cpu_features_sub.addFeature(@intFromEnum(std.Target.x86.Feature.avx2));

    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86_64,
        .os_tag = .freestanding,
        .abi = .none,
        .cpu_features_add = cpu_features_add,
        .cpu_features_sub = cpu_features_sub,
    });
    const optimize = b.standardOptimizeOption(.{});
    const debug = b.option(bool, "debug", "Run OS in GDB debugging mode") orelse false;
    const options = b.addOptions();
    options.addOption(bool, "debug", debug);

    const kernel = b.addExecutable(.{
        .name = KERNEL_NAME,
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
        .code_model = .kernel,
        .linkage = .static,
    });
    kernel.want_lto = false;
    kernel.root_module.addImport("config", options.createModule());
    kernel.setLinkerScript(b.path("linker.ld"));
    const limineModule = b.dependency("limine_zig", .{}).module("limine");

    const flanterm = b.dependency("flanterm", .{});
    const uacpi = b.dependency("uacpi", .{});
    kernel.root_module.addImport("limine", limineModule);
    kernel.root_module.addImport("kernel", &kernel.root_module);

    kernel.addCSourceFiles(.{
        .root = flanterm.path(""),
        .files = &.{ "flanterm.c", "backends/fb.c" },
    });

    kernel.addCSourceFiles(.{
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
        .flags = &.{ "-DUACPI_SIZED_FREES", "-DUACPI_KERNEL_INITIALIZATION" },
    });
    kernel.addIncludePath(uacpi.path("include"));

    const flantermTranslate = b.addTranslateC(.{
        .root_source_file = flanterm.path("backends/fb.h"),
        .link_libc = false,
        .target = target,
        .optimize = optimize,
    });
    kernel.root_module.addImport("flanterm", flantermTranslate.addModule("flanterm"));

    const uacpiTranslate = b.addTranslateC(.{
        .link_libc = false,
        .target = target,
        .optimize = optimize,
        .root_source_file = b.path("src/drivers/uacpi.h"),
    });
    uacpiTranslate.addIncludeDir(uacpi.path("include").getPath(b));
    kernel.root_module.addImport("uacpi", uacpiTranslate.addModule("uacpi"));

    b.installArtifact(kernel); // Installing kernel so we can examine it for debugging

    const wf = createISODir(b, kernel);
    const iso = runXorriso(b, wf.getDirectory());
    b.getInstallStep().dependOn(&iso.step);
    const qemu_run = b.addSystemCommand(&QEMU_ARGS);
    if (debug) qemu_run.addArg("-s");
    qemu_run.addArg("-cdrom");
    qemu_run.addFileArg(iso.source);
    qemu_run.step.dependOn(b.getInstallStep());
    const qemu_step = b.step("run", "Run the kernel with QEMU");
    qemu_step.dependOn(&qemu_run.step);

    b.installDirectory(.{
        .source_dir = kernel.getEmittedDocs(),
        .install_dir = .prefix,
        .install_subdir = "docs",
    });
}

fn createISODir(b: *std.Build, kernel: *std.Build.Step.Compile) *std.Build.Step.WriteFile {
    const limine = b.dependency("limine_c", .{});
    const wf = b.addWriteFiles();
    _ = wf.addCopyDirectory(
        limine.path(""),
        "",
        .{ .include_extensions = &.{
            "sys",
            "bin",
        } },
    );
    _ = wf.addCopyDirectory(
        limine.path(""),
        "EFI/BOOT",
        .{ .include_extensions = &.{"EFI"} },
    );

    _ = wf.addCopyFile(kernel.getEmittedBin(), KERNEL_NAME);

    for (ISO_COPY) |f| {
        _ = wf.addCopyFile(b.path(f), f);
    }
    return wf;
}

fn runXorriso(b: *std.Build, iso_dir: std.Build.LazyPath) *std.Build.Step.InstallFile {
    const xorriso = b.addSystemCommand(&XORRISO_ARGS);
    xorriso.addDirectoryArg(iso_dir);
    xorriso.addArg("-o");
    return b.addInstallBinFile(xorriso.addOutputFileArg(ISO_OUT), ISO_OUT);
}
