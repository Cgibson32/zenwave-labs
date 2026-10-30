juce_add_plugin(Core
  PRODUCT_NAME                "Core"
  COMPANY_NAME                "${ZENWAVE_COMPANY_NAME}"
  COMPANY_WEBSITE             "${ZENWAVE_COMPANY_WEBSITE}"
  COMPANY_EMAIL               "${ZENWAVE_COMPANY_EMAIL}"
  BUNDLE_ID                   "${ZENWAVE_BUNDLE_PREFIX}.core"
  PLUGIN_MANUFACTURER_CODE    ${ZENWAVE_MANUFACTURER_CODE}
  PLUGIN_CODE                 Zcor
  FORMATS                     VST3 AU Standalone
  VST3_CATEGORIES             Fx Distortion Dynamics
  AU_MAIN_TYPE                kAudioUnitType_Effect
  IS_SYNTH                    FALSE
  NEEDS_MIDI_INPUT            FALSE
  NEEDS_MIDI_OUTPUT           FALSE
  IS_MIDI_EFFECT              FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD     ${ZENWAVE_COPY_PLUGINS}
  VERSION                     ${PROJECT_VERSION}
)

juce_generate_juce_header(Core)

target_sources(Core PRIVATE
  src/PluginProcessor.cpp
  src/PluginEditor.cpp
  src/Presets.cpp
)

target_compile_definitions(Core PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0
  JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(Core
  PRIVATE
    zenwave::dsp
    zenwave::ui
    juce::juce_audio_utils
    juce::juce_dsp
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
