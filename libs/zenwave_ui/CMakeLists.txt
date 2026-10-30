# zenwave_ui: shared look-and-feel and widgets for every ZenWave plugin (header-only, needs JUCE).
add_library(zenwave_ui INTERFACE)
add_library(zenwave::ui ALIAS zenwave_ui)
target_include_directories(zenwave_ui INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
