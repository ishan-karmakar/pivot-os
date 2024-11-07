const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.acpi);
const std = @import("std");

pub fn init() void {
    if (uacpi.uacpi_initialize(0) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_initialize failed");
    }
    if (uacpi.uacpi_namespace_load() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_namespace_load failed");
    }
    if (uacpi.uacpi_namespace_initialize() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_namespace_initialize failed");
    }
    if (uacpi.uacpi_finalize_gpe_initialization() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_finalize_gpe_initialization failed");
    }
    uacpi.uacpi_namespace_for_each_node_depth_first(uacpi.uacpi_namespace_root(), acpi_init_device, null);
}

pub fn shutdown() void {
    if (uacpi.uacpi_prepare_for_sleep_state(uacpi.UACPI_SLEEP_STATE_S5) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_prepare_for_sleep_state failed");
    }

    asm volatile ("cli");
    if (uacpi.uacpi_enter_sleep_state(uacpi.UACPI_SLEEP_STATE_S5) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_enter_sleep_state failed");
    }
    unreachable;
}

fn acpi_init_device(_: ?*anyopaque, node: ?*uacpi.uacpi_namespace_node) callconv(.C) uacpi.uacpi_ns_iteration_decision {
    const path: []const u8 = std.mem.span(uacpi.uacpi_namespace_node_generate_absolute_path(node));
    var info: *uacpi.uacpi_namespace_node_info = undefined;
    if (uacpi.uacpi_get_namespace_node_info(node, @ptrCast(&info)) != uacpi.UACPI_STATUS_OK) {
        log.err("Unable to retrieve node {s} information", .{path});
        uacpi.uacpi_free_namespace_node_info(info);
        return uacpi.UACPI_NS_ITERATION_DECISION_CONTINUE;
    }
    log.info("{s}, type: {}", .{ path, info.type });

    // // if (info.type != uacpi.UACPI_OBJECT_DEVICE) {
    // //     uacpi.uacpi_free_namespace_node_info(info);
    // //     return uacpi.UACPI_NS_ITERATION_DECISION_CONTINUE;
    // // }

    // if (info.flags & uacpi.UACPI_NS_NODE_INFO_HAS_HID == 0) {
    //     log.warn("Node {s} does not contain an HID, skipping - {}", .{ path, info.flags });
    //     uacpi.uacpi_free_namespace_node_info(info);
    //     return uacpi.UACPI_NS_ITERATION_DECISION_CONTINUE;
    // }

    // log.info("{s}", .{info.hid.value[0..info.hid.size]});

    // uacpi.uacpi_free_namespace_node_info(info);
    return uacpi.UACPI_NS_ITERATION_DECISION_CONTINUE;
}
