cmake_minimum_required(VERSION 3.22)

project(ZenWaveLabs VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Universal binaries on macOS (Apple Silicon + Intel)
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

option(ZENWAVE_BUILD_TESTS "Build DSP unit tests" ON)
option(ZENWAVE_COPY_PLUGINS "Copy built plugins into the user plugin folders" OFF)

# ---------------------------------------------------------------------------
# JUCE (fetched automatically; pin to a release tag)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG        8.0.4
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Shared company settings
# ---------------------------------------------------------------------------
set(ZENWAVE_COMPANY_NAME      "ZenWave Labs")
set(ZENWAVE_COMPANY_WEBSITE   "https://zenwavelabs.com")
set(ZENWAVE_COMPANY_EMAIL     "support@zenwavelabs.com")
set(ZENWAVE_MANUFACTURER_CODE "Zwav")   # exactly 4 chars, first uppercase
set(ZENWAVE_BUNDLE_PREFIX     "com.zenwavelabs")

add_subdirectory(libs/zenwave_dsp)
add_subdirectory(libs/zenwave_ui)
add_subdirectory(plugins/drift)
add_subdirectory(plugins/umbra)
add_subdirectory(plugins/core)
add_subdirectory(plugins/pulse)
option(ZENWAVE_BUILD_SNAPSHOT "Build headless editor screenshot tools" OFF)
option(ZENWAVE_BUILD_QA "Build headless QA sweep tools" OFF)
if(ZENWAVE_BUILD_QA)
  add_subdirectory(tools/qa)
endif()
if(ZENWAVE_BUILD_SNAPSHOT)
  add_subdirectory(tools/snapshot)
endif()

if(ZENWAVE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
