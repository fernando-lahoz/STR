**gtk-monitor** requires GTK 4.14 or later and it's meant to be used in debian-based distros.

Compilation with **pkgconfig** and **gcc**:
```bash
cd gtk-monitor
gcc `pkg-config --cflags gtk4` gtk-monitor.c plot.c serial.c reader_thread.c writer_thread.c \
    -o gtk-monitor `pkg-config --libs gtk4` -O3

# Execute
./gtk-monitor
```
