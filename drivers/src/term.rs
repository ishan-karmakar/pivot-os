use core::{fmt::Write, ptr::null_mut};

use flanterm_bindings::{flanterm_context, flanterm_fb_init, flanterm_write};
use limine::request::FramebufferRequest;
use spin::{Lazy, Mutex};

#[link_section = ".requests"]
static FRAMEBUFFER_REQUEST: FramebufferRequest = FramebufferRequest::new();

pub struct TermWriter(pub *mut flanterm_context);
unsafe impl Send for TermWriter {}

impl Write for TermWriter {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        // This is unsafe to convert &str to *const i8 but rn I don't have a better solution
        // I need a global allocator for CString but I want to print everything before initialization as well
        unsafe { flanterm_write(self.0, s as *const _ as *const i8, s.len()) };
        Ok(())
    }

    fn write_char(&mut self, c: char) -> core::fmt::Result {
        unsafe { flanterm_write(self.0, &c as *const _ as *const i8, 1) };
        Ok(())
    }
}

pub static TERM_WRITER: Lazy<Mutex<TermWriter>> = Lazy::new(|| {
    let fb = FRAMEBUFFER_REQUEST.get_response().unwrap().framebuffers().next().unwrap();
    Mutex::new(TermWriter(unsafe {
        flanterm_fb_init(
            None,
            None,
            fb.addr() as *mut _,
            fb.width() as usize,
            fb.height() as usize,
            fb.pitch() as usize,
            fb.red_mask_size(),
            fb.red_mask_shift(),
            fb.green_mask_size(),
            fb.green_mask_shift(),
            fb.blue_mask_size(),
            fb.blue_mask_shift(),
            null_mut(),
            null_mut(),
            null_mut(),
            null_mut(),
            null_mut(), // &mut 0x00FFFFFFu32 as *mut _,
            null_mut(),
            null_mut(),
            null_mut(),
            0, 0, 1, 0, 0, 0
        )
    }))
});