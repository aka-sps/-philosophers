cmake_minimum_required(VERSION 3.0)

project(philosophers
    VERSION 1.1)

find_package(Git)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_CURRENT_SOURCE_DIR} describe --dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE
	OUTPUT_STRIP_TRAILING_WHITESPACE
	)
    message(STATUS "${GIT_DESCRIBE}")
else(Git_FOUND)
    set(GIT_DESCRIBE "${PROJECT_VERSION}" CACHE STRING "User-defined version string")
endif(Git_FOUND)

add_library(git-based-version INTERFACE)
target_compile_definitions(git-based-version INTERFACE GIT_DESCRIBE="${GIT_DESCRIBE}")

set(CXX_USED_FEATURES
    cxx_strong_enums
    cxx_decltype
    cxx_lambdas
    cxx_auto_type
    cxx_override
    cxx_nullptr
)

add_library(cxx-interface INTERFACE)
target_compile_features(cxx-interface INTERFACE ${CXX_USED_FEATURES})
#set_property(TARGET cxx-interface PROPERTY CXX_STANDARD 11)

set(PHILOSOPHERS_STARVATION 1 CACHE BOOL "Define philosophers starvation")

add_executable(philosophers
    philosophers.cpp
)
target_compile_definitions(philosophers PUBLIC $<$<BOOL:${PHILOSOPHERS_STARVATION}>:PHILOSOPHERS_STARVATION>)
target_link_libraries(philosophers PRIVATE git-based-version cxx-interface)
