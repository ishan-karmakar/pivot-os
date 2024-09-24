use core::{alloc::{GlobalAlloc, Layout}, ptr::{null_mut, NonNull}};

use rlsf::Tlsf;
use spin::Mutex;

use crate::vmm::KVMM;

// use crate::vmm::KVMM;

const HEAP_SIZE: usize = 0x1000 * 4;

struct GlobalTlsf(pub Mutex<Tlsf<'static, u16, u16, 12, 16>>);

unsafe impl GlobalAlloc for GlobalTlsf {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        self.0.lock().allocate(layout).map(NonNull::as_ptr).unwrap_or(null_mut())
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        self.0.lock().deallocate(NonNull::new_unchecked(ptr), layout.align())
    }

    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        let layout = Layout::from_size_align_unchecked(new_size, layout.align());
        self.0.lock().reallocate(NonNull::new_unchecked(ptr), layout).map(NonNull::as_ptr).unwrap_or(null_mut())
    }
}

#[global_allocator]
static HEAP: GlobalTlsf = GlobalTlsf(Mutex::new(Tlsf::new()));

pub fn init() {
    let pool = KVMM.lock().allocate(HEAP_SIZE).unwrap();
    unsafe { HEAP.0.lock().insert_free_block_ptr(pool) }.unwrap();
    log::info!("Initialized kernel heap (TLSF)");
}