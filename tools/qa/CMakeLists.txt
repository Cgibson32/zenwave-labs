# Headless QA: preset + parameter sweep for each plugin (run in CI on Linux).
foreach(P Drift Umbra Core)
  string(TOLOWER ${P} plow)
  juce_add_console_app(${P}QA PRODUCT_NAME "${P}QA")
  juce_generate_juce_header(${P}QA)
  target_sources(${P}QA PRIVATE qa.cpp
    ${CMAKE_SOURCE_DIR}/plugins/${plow}/src/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/plugins/${plow}/src/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/plugins/${plow}/src/Presets.cpp)
  target_include_directories(${P}QA PRIVATE ${CMAKE_SOURCE_DIR}/plugins/${plow}/src)
  target_compile_definitions(${P}QA PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0 JUCE_MODAL_LOOPS_PERMITTED=1
    JucePlugin_Name="${P}" JUCE_STANDALONE_APPLICATION=0 JucePlugin_Build_VST3=0 JucePlugin_Build_AU=0
    JucePlugin_Build_Standalone=0 JucePlugin_Build_VST=0 JucePlugin_Build_AAX=0 JucePlugin_Build_Unity=0
    JucePlugin_Build_LV2=0 JucePlugin_Build_AUv3=0 JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_EditorRequiresKeyboardFocus=0
    JucePlugin_ManufacturerCode=0x5a776176 JucePlugin_PluginCode=0x5a647266 JucePlugin_Manufacturer="ZenWave Labs"
    JucePlugin_VersionString="0.1.0" JucePlugin_Version=0x10000 JucePlugin_Desc="${P}" JucePlugin_VSTUniqueID=0
    JucePlugin_VSTCategory=kPlugCategEffect JucePlugin_AUMainType=0 JucePlugin_AUSubType=0 JucePlugin_AUExportPrefix=x
    JucePlugin_AUExportPrefixQuoted="x" JucePlugin_AUManufacturerCode=0 JucePlugin_CFBundleIdentifier=x
    JucePlugin_MaxNumInputChannels=2 JucePlugin_MaxNumOutputChannels=2 JucePlugin_PreferredChannelConfigurations=
    JucePlugin_ManufacturerWebsite="" JucePlugin_ManufacturerEmail="")
  target_link_libraries(${P}QA PRIVATE zenwave::dsp zenwave::ui juce::juce_audio_utils juce::juce_dsp
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endforeach()
