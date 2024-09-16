use limine::{request::{HhdmRequest, MemoryMapRequest, PagingModeRequest}, BaseRevision};

#[used]
#[link_section = ".requests"]
static BASE_REVISION: BaseRevision = BaseRevision::new();

#[used]
#[link_section = ".requests"]
#[no_mangle]
static MMAP_REQUEST: MemoryMapRequest = MemoryMapRequest::new();

#[used]
#[link_section = ".requests"]
#[no_mangle]
static HHDM_REQUEST: HhdmRequest = HhdmRequest::new();

#[used]
#[link_section = ".requests"]
#[no_mangle]
static PAGING_REQUEST: PagingModeRequest = PagingModeRequest::new(); // Already set to default (LVL4)