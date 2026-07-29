# Fooyin MDX Plugin

An input plugin for **Fooyin** that decodes Sharp X68000 `.mdx` chiptunes via `mdxmini`.

## Features

- Full playback of 8-channel YM2151 FM synth & MSM6258 ADPCM audio samples, with PCM8 support too!
- Support for switching between the cycle-accurate Nuked-OPM YM2151 emulation core and original MAME YM2151 emulator. 
- Automatic lookup of associated `.pdx` sample files in the song's directory, or specify a fallback directory. 
- Displays song titles correctly with automatic Shift-JIS title string decoding.
- Customisable gain, loop count, and output sample rate.
- Shouldn't crash on bad/broken MDX files!

## Building Instructions

1. Configure and build using Ninja:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

2. Install the plugin:
   ```bash
   mkdir -p ~/.local/lib/fooyin/plugins
   cp build/mdxinput/fyplugin_mdxinput.so ~/.local/lib/fooyin/plugins/
   ```

## License
GPLv2 or later.
