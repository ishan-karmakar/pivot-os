struct Block {
    pub size: usize, // Always a power of two
    pub sibling: *mut Block
}

pub struct VirtualMemoryManager<const MB: usize = 0x1000>(*mut Block);

impl<const MB: usize> VirtualMemoryManager<MB> {
    pub fn new(mut start: usize, mut size: usize) -> Self {
        size = (size / MB) * MB;
        let root_block = start as *mut Block;
        let mut cur_block = root_block;
        loop {
            let block_size = 2_usize.pow(size.ilog2());
            size -= block_size as usize;
            if size == 0 {
                return Self(0 as *mut Block);
            }
            start += block_size;
        }
    }
}

pub(crate) fn init() {
    let vmm: VirtualMemoryManager<> = VirtualMemoryManager::new(0, 0x11000);
}