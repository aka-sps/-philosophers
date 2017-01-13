cmake_minimum_required(VERSION 3.0)

project(philosophers)

find_package(Threads)

add_executable(philosophers
	${CMAKE_CURRENT_SOURCE_DIR}/philosophers.cpp
)
set_property(TARGET philosophers PROPERTY CXX_STANDARD 11)

target_compile_features(philosophers
	PUBLIC
	cxx_strong_enums
	cxx_decltype
	cxx_lambdas
	cxx_auto_type
	cxx_override
	cxx_nullptr
)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(philosophers PUBLIC -pthread)
endif()

if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(philosophers PUBLIC ${CMAKE_THREAD_LIBS_INIT})
endif()
