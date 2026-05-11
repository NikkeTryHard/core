# Native audio third-party sources

## SoundTouch

- Source: https://codeberg.org/soundtouch/soundtouch
- Imported revision: f738b1132ec1fd56efc90367898244cf52d9e6a5
- Version: 2.4.1
- License: LGPL-2.1-or-later; see `soundtouch/COPYING.TXT`.
- Local build: compiled as shared library `libsoundtouch.so`, then linked by `libsimpmusic_audio.so`.
- Local changes:
  - `source/RateTransposer.cpp`: added `<algorithm>` include for NDK libc++ `std::min/std::max` resolution.
  - CMake disables x86 SIMD optimizations with `SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS=1` for portable Android ABI builds.
- Used files are listed in `../../CMakeLists.txt`; extra upstream source files are retained for license/source correspondence.

## Airwindows Reverb

- Source: https://github.com/airwindows/airwindows/tree/master/plugins/LinuxVST/src/Reverb
- License: MIT. See `airwindows_reverb.cpp` header.
- Local changes: VST plugin wrapper removed; DSP ported into `AirwindowsReverb` for realtime Android PCM processing.
