pub const CPUIDResult = struct {
    eax: u32,
    ebx: u32,
    ecx: u32,
    edx: u32,
};

pub const IRETStatus = packed struct {
    rip: u64,
    cs: u64,
    rflags: u64,
    rsp: u64,
    ss: u64,
};

pub const Status = packed struct {
    iret_status: IRETStatus,
    rax: u64,
    rbx: u64,
    rcx: u64,
    rdx: u64,
    rbp: u64,
    rsi: u64,
    rdi: u64,
    r8: u64,
    r9: u64,
    r10: u64,
    r11: u64,
    r12: u64,
    r13: u64,
    r14: u64,
    r15: u64,
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

pub inline fn set_kgs(a: u64) void {
    wrmsr(0xC0000102, a);
}

pub inline fn get_kgs() u64 {
    return rdmsr(0xC0000102);
}

pub inline fn set_cr3(pml4: usize) void {
    asm volatile ("mov %[pml4], %%cr3"
        :
        : [pml4] "r" (pml4),
    );
}