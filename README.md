# V-term
A terminal emulator with support for VGA graphics (see gfx modes below).

This is a personal project and work in progress (see TODO below)
# TODO
- [ ] pty modes
    - [x] Ensure the text conforms to resolution (+ mod window size)
    - [x] Scrolling
    - [x] Wrapping
    - [ ] Escape codes (See [here](https://www.xfree86.org/current/ctlseqs.html) and [here](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html))
        - [ ] Colors
            - [ ] 3 bit
            - [ ] 8 bit
            - [ ] Full color
- [ ] gfx modes (see [here](https://prirai.github.io/blogs/ansi-esc/#screen-modes))
    - [ ] shared process memory (`shm_open` or `mmap`) for vram (aka vram store in ram)
- [ ] General (done using custom escape codes)
    - [ ] Switching modes (discarding all elements in current buffer)
    - [ ] Switching buffers (change `current_buffer`)
- [ ] Other todos for future
    - [ ] Improve performance
        - [ ] Call `DrawText` once per frame (custom `DrawText` function to account for color both bg and fg)
        - [ ] Multithreading ?
