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

const ISO_COPY = [_][]const u8{ "limine.conf", "font.sfn" };

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

const LWIP_SOURCES = &.{
    "core/init.c",
    "core/def.c",
    "core/dns.c",
    "core/inet_chksum.c",
    "core/ip.c",
    "core/mem.c",
    "core/memp.c",
    "core/netif.c",
    "core/pbuf.c",
    "core/raw.c",
    "core/stats.c",
    "core/sys.c",
    "core/altcp.c",
    "core/altcp_alloc.c",
    "core/altcp_tcp.c",
    "core/tcp.c",
    "core/tcp_in.c",
    "core/tcp_out.c",
    "core/timeouts.c",
    "core/udp.c",
    "core/ipv4/acd.c",
    "core/ipv4/autoip.c",
    "core/ipv4/dhcp.c",
    "core/ipv4/etharp.c",
    "core/ipv4/icmp.c",
    "core/ipv4/igmp.c",
    "core/ipv4/ip4_frag.c",
    "core/ipv4/ip4.c",
    "core/ipv4/ip4_addr.c",
    "netif/ethernet.c",
    "netif/bridgeif.c",
    "netif/bridgeif_fdb.c",
    // "api/api_lib.c",
    // "api/api_msg.c",
    // "api/err.c",
    // "api/if_api.c",
    // "api/netbuf.c",
    // "api/netdb.c",
    // "api/netifapi.c",
    // "api/sockets.c",
    "api/tcpip.c",
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
    b.installArtifact(kernel);
    addSSFN(kernel);
    addLimine(kernel);
    addUACPI(kernel);
    addLWIP(kernel);
    addPrintf(kernel);
    return kernel;
}

fn addSSFN(kernel: *Step.Compile) void {
    const dep = kernel.step.owner.dependency("ssfn", .{});
    kernel.root_module.addCSourceFile(.{ .file = kernel.step.owner.path("src/ssfn.c") });
    kernel.root_module.addImport("ssfn", kernel.step.owner.addTranslateC(.{
        .link_libc = false,
        .optimize = kernel.root_module.optimize.?,
        .target = kernel.root_module.resolved_target.?,
        .root_source_file = dep.path("ssfn.h"),
    }).createModule());
    kernel.root_module.addIncludePath(dep.path(""));
}

fn addUACPI(kernel: *Step.Compile) void {
    const uacpi = kernel.step.owner.dependency("uacpi", .{});
    kernel.root_module.addCSourceFiles(.{
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
    kernel.root_module.addIncludePath(uacpi.path("include"));
}

fn addLimine(kernel: *Step.Compile) void {
    const limineZigMod = kernel.step.owner.dependency("limine_zig", .{}).module("limine");
    kernel.root_module.addImport("limine", limineZigMod);
}

fn addLWIP(kernel: *Step.Compile) void {
    const dep = kernel.step.owner.dependency("lwip", .{});
    kernel.root_module.addCSourceFiles(.{
        .root = dep.path("src"),
        .files = LWIP_SOURCES,
        .flags = &.{},
    });
    const translateC = kernel.step.owner.addTranslateC(.{
        .link_libc = false,
        .optimize = kernel.root_module.optimize.?,
        .target = kernel.root_module.resolved_target.?,
        .root_source_file = kernel.step.owner.path("src/lwip.h"),
    });
    translateC.addIncludePath(dep.path("src/include"));
    translateC.addIncludePath(kernel.step.owner.path("src/lwip"));
    kernel.root_module.addImport("lwip", translateC.createModule());
    kernel.root_module.addIncludePath(dep.path("src/include"));
    kernel.root_module.addIncludePath(kernel.step.owner.path("src/lwip"));
}

fn addPrintf(kernel: *Step.Compile) void {
    const dep = kernel.step.owner.dependency("printf", .{});
    kernel.root_module.addCSourceFile(.{ .file = dep.path("printf.c") });
    const translateC = kernel.step.owner.addTranslateC(.{
        .link_libc = false,
        .optimize = kernel.root_module.optimize.?,
        .target = kernel.root_module.resolved_target.?,
        .root_source_file = dep.path("printf.h"),
    });
    kernel.root_module.addImport("printf", translateC.createModule());
}
