cmake_minimum_required(VERSION 3.0)

project(philosophers
    VERSION 1.1)

enable_testing()

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

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

add_library(cxx-interface INTERFACE)
target_compile_features(cxx-interface INTERFACE
    cxx_strong_enums
    cxx_decltype
    cxx_lambdas
    cxx_auto_type
    cxx_override
    cxx_nullptr
)
if(Threads_FOUND)
    target_link_libraries(cxx-interface INTERFACE Threads::Threads)
elseif(CMAKE_CROSSCOMPILING AND CMAKE_HAVE_PTHREAD_H AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    target_compile_options(cxx-interface INTERFACE "-pthread")
    set_target_properties(cxx-interface PROPERTIES LINK_FLAGS "-pthread")
else()
    message(SEND_ERROR "Undefined Threads")
endif()

set(PHILOSOPHERS_STARVATION 1 CACHE BOOL "Define philosophers starvation")

add_executable(philosophers
    philosophers.cpp
)
target_compile_definitions(philosophers PUBLIC $<$<BOOL:${PHILOSOPHERS_STARVATION}>:PHILOSOPHERS_STARVATION>)
target_link_libraries(philosophers PRIVATE git-based-version cxx-interface)
