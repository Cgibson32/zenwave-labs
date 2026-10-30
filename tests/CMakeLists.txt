add_executable(zenwave_dsp_tests dsp_tests.cpp)
target_link_libraries(zenwave_dsp_tests PRIVATE zenwave::dsp)
add_test(NAME zenwave_dsp_tests COMMAND zenwave_dsp_tests)
