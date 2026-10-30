# zenwave_dsp: header-only, JUCE-free DSP building blocks shared by every ZenWave plugin.
add_library(zenwave_dsp INTERFACE)
add_library(zenwave::dsp ALIAS zenwave_dsp)

target_include_directories(zenwave_dsp INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(zenwave_dsp INTERFACE cxx_std_20)
