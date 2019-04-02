cmake_minimum_required(VERSION 3.0)

project(philosophers)

add_executable(philosophers
	${CMAKE_CURRENT_SOURCE_DIR}/philosophers.cpp
)

set(CXX_USED_FEATURES
    cxx_strong_enums
    cxx_decltype
    cxx_lambdas
    cxx_auto_type
    cxx_override
    cxx_nullptr
)
target_compile_features(philosophers PUBLIC ${CXX_USED_FEATURES})
set_property(TARGET philosophers PROPERTY CXX_STANDARD 11)
