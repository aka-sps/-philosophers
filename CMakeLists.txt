cmake_minimum_required(VERSION 3.0)

project(philosophers)
add_executable(philosophers
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_compile_features(philosophers PUBLIC
    cxx_alias_templates
    cxx_auto_type
    cxx_binary_literals
    cxx_constexpr
    cxx_decltype
    cxx_delegating_constructors
    cxx_digit_separators
    cxx_final
    cxx_generalized_initializers
    cxx_lambdas
    cxx_lambda_init_captures
    cxx_nullptr
    cxx_override
    cxx_range_for
    cxx_right_angle_brackets
    cxx_rvalue_references
    cxx_static_assert
    cxx_trailing_return_types
    cxx_uniform_initialization
)
