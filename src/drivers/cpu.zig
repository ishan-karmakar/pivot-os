pub const CPUIDResult = struct {
    eax: u32,
    ebx: u32,
    ecx: u32,
    edx: u32,
};

pub fn rdmsr(addr: u32) u64 {
    var low: u32 = undefined;
    var high: u32 = undefined;
    asm ("rdmsr"
        : [low] "={eax}" (low),
          [high] "={edx}" (high),
        : [addr] "{ecx}" (addr),
    );
    return low | (@as(u64, @intCast(high)) << 32);
}

pub fn wrmsr(addr: u32, val: u64) void {
    asm volatile ("wrmsr"
        :
        : [low] "{eax}" (@as(u32, @truncate(val))),
          [high] "{edx}" (@as(u32, @truncate(val >> 32))),
          [addr] "{ecx}" (addr),
    );
}

pub fn cpuid(level: u32, count: u32) CPUIDResult {
    var eax: u32 = undefined;
    var ebx: u32 = undefined;
    var ecx: u32 = undefined;
    var edx: u32 = undefined;
    asm ("cpuid"
        : [eax] "={eax}" (eax),
          [ebx] "={ebx}" (ebx),
          [ecx] "={ecx}" (ecx),
          [edx] "={edx}" (edx),
        : [level] "{eax}" (level),
          [count] "{ecx}" (count),
    );
    return .{
        .eax = eax,
        .ebx = ebx,
        .ecx = ecx,
        .edx = edx,
    };
}
