cmake_minimum_required(VERSION 3.5)
project(job_system LANGUAGES C)

option(JOBS_DEVELOPER_WARNINGS "Enable more warnings" OFF)

include(cmake/target_default_compile_warnings.cmake)
include(cmake/target_default_compile_options.cmake)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/${CMAKE_SYSTEM_NAME})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/${CMAKE_SYSTEM_NAME})

set(INC
        include/jobs.h
        include/private/work_stealing_queue.h
        include/private/api_defines.h
        include/private/api_types.h
        include/private/context.h
        include/private/jobs_api.h)
     
set(SRC
        src/work_stealing_queue.c
        src/context.c
        src/jobs_api.c)

add_library(jobs SHARED ${INC} ${SRC})
target_default_compile_options_c(jobs)

if(JOBS_DEVELOPER_WARNINGS)
target_default_compile_warnings_c(jobs)
endif()

target_include_directories(jobs PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

add_executable(main main.c)
target_link_libraries(main jobs)