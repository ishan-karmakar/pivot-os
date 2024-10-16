const std = @import("std");

pub fn build(b: *std.Build) void {
    const limine = b.dependency("limine", .{});
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86_64,
        .os_tag = .freestanding,
    });
    const optimize = b.standardOptimizeOption(.{});

    const kernel = b.addExecutable(.{
        .name = "pivot-os",
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    // kernel.setLinkerScript(b.path("linker.ld"));

    const wf = b.addWriteFiles();
    _ = wf.addCopyDirectory(
        limine.path(""),
        "",
        .{ .include_extensions = &.{
            "sys",
            "bin",
            "EFI",
        } },
    );

    _ = wf.addCopyFile(kernel.getEmittedBin(), "");

    // limine.conf + font

    const xorriso = b.addSystemCommand(&.{
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
    });
    xorriso.addDirectoryArg(wf.getDirectory());
    xorriso.addArg("-o");
    const iso_out = xorriso.addOutputFileArg("os.iso");
    const install_out = b.addInstallBinFile(iso_out, "os.iso");
    b.getInstallStep().dependOn(&install_out.step);
}
