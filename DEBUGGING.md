# XMMS Debugging Setup

This document describes how to set up and use debugging for the XMMS project.

## Prerequisites

- GDB (GNU Debugger) - ✅ Installed at `/usr/bin/gdb` (version 16.3)
- VS Code with C/C++ extension
- Build dependencies (GTK+2, GLib2, etc.)
- GTK2 Debug Symbols - ✅ Installed and configured

## GTK2 Debug Symbols Setup

The following debug packages have been installed for comprehensive GTK2 debugging:

### Installed Debug Packages
- `libgtk-2_0-0-debuginfo` - Core GTK2 library debug symbols
- `gtk2-devel-debuginfo` - GTK2 development tools debug symbols
- `glib2-devel-debuginfo` - GLib2 library debug symbols
- `libgdk_pixbuf-2_0-0-debuginfo` - GDK-PixBuf debug symbols
- `libpango-1_0-0-debuginfo` - Pango text rendering debug symbols
- `libcairo-gobject2-debuginfo` - Cairo graphics debug symbols

### Debug File Locations
- **Debug symbols**: `/usr/lib/debug/usr/lib64/`
- **Source code**: `/usr/src/debug/gtk-2.24.33/` and `/usr/src/debug/glib-2.86.1/`

### GDB Configuration
VS Code debug configurations are set up to automatically:
- Load debug symbols from `/usr/lib/debug`
- Find source code in `/usr/src/debug`
- Enable step-through debugging of GTK2 functions

This allows you to:
- Step into GTK2 function calls (gtk_widget_*, g_*, etc.)
- View GTK2 source code during debugging
- Get meaningful stack traces with symbol names
- Inspect GTK2 data structures with proper type information

## Quick Start

### 1. Build for Debugging

Choose one of these options:

**Option A: Quick debug build (preserves current configuration)**
```bash
# Run the VS Code task "build-debug" or use terminal:
make clean && make CFLAGS="-g3 -O0 -Wall -Wpointer-arith -finline-functions" -j4
```

**Option B: Reconfigure for optimal debugging**
```bash
# Run the VS Code task "reconfigure-debug" or use terminal:
make distclean
CFLAGS="-g3 -O0" ./configure --enable-debug --prefix=/usr/local
make -j4
```

### 2. Debug with VS Code

1. Open the Run and Debug view (Ctrl+Shift+D)
2. Select one of the debug configurations:
   - **"Debug XMMS"** - Launch XMMS with debugger attached
   - **"Debug XMMS (with args)"** - Launch with command line arguments
   - **"Attach to XMMS"** - Attach to already running XMMS process

3. Set breakpoints by clicking in the left margin of source files
4. Press F5 to start debugging

### 3. Debug with GDB (Command Line)

```bash
# Start GDB with XMMS
gdb ./xmms/xmms

# Common GDB commands:
(gdb) run                    # Start the program
(gdb) break main            # Set breakpoint at main function
(gdb) break filename.c:123  # Set breakpoint at specific line
(gdb) continue              # Continue execution
(gdb) step                  # Step into functions
(gdb) next                  # Step over functions
(gdb) print variable_name   # Print variable value
(gdb) backtrace            # Show call stack
(gdb) info locals          # Show local variables
(gdb) quit                 # Exit GDB
```

## Available VS Code Tasks

- **build-debug**: Clean and build with debug flags (-g3 -O0)
- **build-release**: Build with default optimization
- **clean**: Clean build artifacts
- **reconfigure-debug**: Reconfigure project for debugging
- **install**: Install XMMS to system
- **run-xmms**: Build and run XMMS without debugger

Access tasks via: `Ctrl+Shift+P` → "Tasks: Run Task"

## Debug Configurations Explained

### Debug XMMS
- Builds the project with debug flags if needed
- Launches XMMS in the debugger
- Stops at main() if "stopAtEntry" is enabled
- Best for general debugging

### Debug XMMS (with args)
- Same as above but prompts for command line arguments
- Useful for debugging file loading: `filename.mp3`
- Or testing specific features: `--help`, `--version`

### Attach to XMMS
- Attaches debugger to already running XMMS process
- Useful for debugging issues that occur during runtime
- Select the XMMS process from the list when prompted

## Common Debugging Scenarios

### 1. Debug Startup Issues
1. Set breakpoint at `main()` function in `xmms/main.c`
2. Use "Debug XMMS" configuration
3. Step through initialization code

### 2. Debug Plugin Loading
1. Set breakpoints in plugin loading functions
2. Look for functions containing "plugin" in their names
3. Use "Debug XMMS" and let it load completely

### 3. Debug Audio Playback
1. Load an audio file in XMMS first
2. Set breakpoints in audio output plugins (`Output/` directory)
3. Use "Attach to XMMS" to debug running instance

### 4. Debug UI Issues
1. Set breakpoints in GTK event handlers
2. Look for functions with "callback" or "event" in names
3. Debug user interactions step by step

## Debugging Tips

### Setting Breakpoints
- Click in the left margin of source files in VS Code
- Or use GDB: `break function_name` or `break file.c:line_number`
- Conditional breakpoints: Right-click breakpoint → "Edit Breakpoint"

### Inspecting Variables
- Hover over variables in VS Code to see values
- Use the Variables panel in Debug view
- In GDB: `print variable_name` or `p var`

### Call Stack
- View in VS Code's Call Stack panel
- In GDB: `backtrace` or `bt`

### Memory Issues
```bash
# Run with Valgrind for memory debugging
valgrind --tool=memcheck --leak-check=full ./xmms/xmms
```

## Project Structure for Debugging

Key directories to understand:
- `xmms/` - Main application code
- `libxmms/` - Core library functions
- `Input/` - File format plugins (MP3, WAV, etc.)
- `Output/` - Audio output plugins (OSS, ALSA, etc.)
- `Effect/` - Audio effect plugins
- `Visualization/` - Visual effect plugins

## Troubleshooting

### "No debugging symbols found"
- Ensure you built with debug flags: `-g3 -O0`
- Run `file ./xmms/.libs/xmms` to verify debug info is present
- Look for "not stripped" in the output

### "Cannot find source file"
- Check that source paths in VS Code match actual file locations
- Verify `"cwd"` setting in launch configuration

### "Error while loading shared libraries"
**This is the most common issue with XMMS debugging.**

The error usually looks like:
```
error while loading shared libraries: libxmms.so.1: cannot open shared object file: No such file or directory
```

**Solution**: The debug configurations are already set up with the correct `LD_LIBRARY_PATH`. If you still encounter this:

1. **For VS Code debugging**: Use the "Debug XMMS" configuration which sets:
   ```
   LD_LIBRARY_PATH=${workspaceFolder}/libxmms/.libs:/usr/local/lib64:${env:LD_LIBRARY_PATH}
   ```

2. **For command line debugging**:
   ```bash
   cd /home/isac/repos/xmms-revived
   export LD_LIBRARY_PATH="$(pwd)/libxmms/.libs:/usr/local/lib64:$LD_LIBRARY_PATH"
   gdb xmms/.libs/xmms
   ```

3. **For running without debugger**:
   ```bash
   # Use the wrapper script (recommended)
   ./xmms/xmms
   
   # Or set library path manually
   LD_LIBRARY_PATH="$(pwd)/libxmms/.libs:/usr/local/lib64:$LD_LIBRARY_PATH" ./xmms/.libs/xmms
   ```

### X11/Display Issues
XMMS is a GUI application requiring X11. If you get display errors:

- Ensure `DISPLAY` environment variable is set: `echo $DISPLAY`
- Test X11 connection: `xeyes` or `xclock`
- For remote debugging, use X11 forwarding: `ssh -X`
- The debug configurations automatically inherit your `DISPLAY` setting

### GDB crashes or hangs
- Try building with `-g3` instead of `-g`
- Disable compiler optimizations completely: `-O0`
- Check for threading issues with `info threads`
- Use `stopAtEntry: true` in launch.json to pause at program start

### Plugin debugging
- Plugins are shared libraries (.so files)
- They're loaded dynamically at runtime
- Set breakpoints after plugin loading completes
- Use `info sharedlibrary` in GDB to see loaded plugins

## Building with Different Debug Levels

```bash
# Minimal debug info (fastest compile)
CFLAGS="-g -O0" make

# More debug info (recommended)
CFLAGS="-g3 -O0" make

# Maximum debug info with extra checks
CFLAGS="-g3 -O0 -DDEBUG -D_DEBUG -fsanitize=address" make
```

## Additional Tools

### Static Analysis
```bash
# Use cppcheck for static analysis
cppcheck --enable=all --inconclusive --std=c99 xmms/
```

### Profiling
```bash
# Build with profiling support
CFLAGS="-g -pg" make
./xmms/xmms
gprof ./xmms/xmms gmon.out > analysis.txt
```