cmake_minimum_required(VERSION 3.22.1)

project(simpmusic_audio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(soundtouch SHARED
    src/main/cpp/soundtouch/source/AAFilter.cpp
    src/main/cpp/soundtouch/source/BPMDetect.cpp
    src/main/cpp/soundtouch/source/cpu_detect_x86.cpp
    src/main/cpp/soundtouch/source/FIFOSampleBuffer.cpp
    src/main/cpp/soundtouch/source/FIRFilter.cpp
    src/main/cpp/soundtouch/source/InterpolateCubic.cpp
    src/main/cpp/soundtouch/source/InterpolateLinear.cpp
    src/main/cpp/soundtouch/source/InterpolateShannon.cpp
    src/main/cpp/soundtouch/source/PeakFinder.cpp
    src/main/cpp/soundtouch/source/RateTransposer.cpp
    src/main/cpp/soundtouch/source/SoundTouch.cpp
    src/main/cpp/soundtouch/source/TDStretch.cpp
)

target_include_directories(soundtouch PUBLIC
    src/main/cpp/soundtouch/include
    src/main/cpp/soundtouch/source
)

target_compile_definitions(soundtouch PUBLIC
    SOUNDTOUCH_FLOAT_SAMPLES=1
    ST_NO_EXCEPTION_HANDLING=1
    SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS=1
)

target_compile_options(soundtouch PRIVATE
    -O3
    -ffast-math
)

add_library(simpmusic_audio SHARED
    src/main/cpp/native_audio.cpp
    src/main/cpp/airwindows_reverb.cpp
)

target_include_directories(simpmusic_audio PRIVATE
    src/main/cpp/soundtouch/include
)

target_link_libraries(simpmusic_audio
    soundtouch
    log
)
