# V-term
A terminal emulator with support for VGA graphics

This is a personal project and work in progress (see TODO below)
# TODO
[ ] pty modes
    [x] Ensure the text conforms to resolution (+ mod window size)
    [x] Scrolling
    [x] Wrapping
    [ ] Escape codes
[ ] gfx modes
    [ ] shared process memory (`shm_open` or `mmap`) for vram (aka vram store in ram)
[ ] General
    [ ] Switching modes (discarding)
    [ ] Switching buffers (saving)
