Multithreading
- One queue sorted by quantum (highest quantum first)
- [Kernel Task, Kernel Task 2, User task 1, User task 2]

Init Scheduler:
- Create failsafe thread
- Create thread for kernel function

Create thread (priority, function start, stack pointer):
- Allocate data for EF and thread structure
- Save current EF
- Replace RSP, RIP, etc. in EF
- Insert thread into thread list

Next Thread Algorithm (given active thread):
start = active thread->next
if active thread has ended && active thread == last thread: // Making sure that active thread WAS running
    delete active thread
    active thread = NULL
    last thread = NULL

cur = start
while true:
    if cur is valid thread:
        if cur == last thread:
            return NULL
        set active thread to cur
        set last thread to cur
        return cur
    else if cur->next == start:
        if failsafe thread == last thread:
            return NULL
        set last thread to cur
        return failsafe thread
return NULL

APIC Periodic IRQ:
Increment apic_ticks
Send APIC EOI
Get next thread
If next thread != NULL:
    POP return address
    POP code segment
    POP rflags
    POP stack pointer
    POP stack segment
    Save EF
    Load new EF
    PUSH return address
    PUSH code segment
    PUSH rflags
    PUSH stack pointer
    PUSH stack segment
    return
else:
    Restore any changed registers
    return
