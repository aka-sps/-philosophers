cmake_minimum_required(VERSION 3.0)

project(philosophers)
add_executable(philosophers
    ${CMAKE_CURRENT_SOURCE_DIR}/philosophers.cpp
)

target_compile_features(philosophers PUBLIC
    cxx_strong_enums
    cxx_right_angle_brackets
    cxx_uniform_initialization
    cxx_range_for
    cxx_decltype
    cxx_lambdas
    cxx_auto_type
    cxx_generalized_initializers
    cxx_override
    cxx_nullptr
)
