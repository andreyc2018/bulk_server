cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)

if ($ENV{TRAVIS_BUILD_NUMBER})
    set(BUILD_NUM $ENV{TRAVIS_BUILD_NUMBER})
else()
    set(BUILD_NUM 99)
endif()

set(homework_VERSION_MAJOR 12)
set(homework_VERSION_MINOR 0)
set(homework_VERSION_BUILD ${BUILD_NUM})

project(bulk_server VERSION
        ${homework_VERSION_MAJOR}.${homework_VERSION_MINOR}.${homework_VERSION_BUILD})

find_package(Threads REQUIRED)

configure_file(version.h.in version.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include
        ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googlemock/include
        ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include)

set(HEADER_FILES
        include/async.h
        include/async_internal.h
        include/asyncqueue.h
        include/command.h
        include/counters.h
        include/interpreter.h
        include/listeners.h
        include/logger.h
        include/message.h
        include/messagequeue.h
        include/parser.h
        include/parserstate.h
        include/preprocessor.h
        include/processor.h
        include/reporters.h
        include/singleton.h
        include/writerfactory.h
        include/writers.h
        include/xray.h)

    add_library(server STATIC
        src/async.cpp
        src/async_internal.cpp
        src/interpreter.cpp
        src/logger.cpp
        src/parser.cpp
        src/parserstate.cpp
        src/preprocessor.cpp
        src/processor.cpp
        ${HEADER_FILES})

add_executable(bulk_server
    src/main.cpp
    include/async.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h)

add_executable(test_version src/test_async.cpp)

target_compile_options(server PRIVATE -Wpedantic -Wall -Wextra)
target_compile_options(bulk_server PRIVATE -Wpedantic -Wall -Wextra)
target_compile_options(test_version PRIVATE -Wpedantic -Wall -Wextra)

target_link_libraries(bulk_server server
        Threads::Threads)
target_link_libraries(test_version server gmock_main
        Threads::Threads)

install(TARGETS bulk_server RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_CONTACT "sysdos286@yahoo.com")
set(CPACK_PACKAGE_VERSION_MAJOR ${homework_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${homework_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${homework_VERSION_BUILD})

include(CPack)

enable_testing()

add_test(NAME test_version COMMAND test_version)

add_subdirectory(example)

if(EXISTS ${CMAKE_SOURCE_DIR}/sandbox/CMakeLists.txt)
    add_subdirectory(sandbox EXCLUDE_FROM_ALL)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/googletest/CMakeLists.txt)
    # Replace install() to do-nothing macro.
    macro(install)
    endmacro()
    # Include subproject (or any other CMake code) with "disabled" install().
    message(STATUS "Adding Google Test framework.")
    add_subdirectory(googletest)
    # Restore original install() behavior.
    macro(install)
        _install(${ARGN})
    endmacro()
endif()

message(STATUS "Homework version
    ${homework_VERSION_MAJOR}.${homework_VERSION_MINOR}.${homework_VERSION_BUILD}")

find_package(Doxygen)
if (DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/README.md
            ${CMAKE_CURRENT_BINARY_DIR}/docs/README.md COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/hw_assignment.md
            ${CMAKE_CURRENT_BINARY_DIR}/docs/hw_assignment.md COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/_config.yml
            ${CMAKE_CURRENT_BINARY_DIR}/docs/_config.yml COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/parser_state_machine.uml
            ${CMAKE_CURRENT_BINARY_DIR}/docs/parser_state_machine.uml COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/parser_state_machine.png
            ${CMAKE_CURRENT_BINARY_DIR}/docs/html/parser_state_machine.png COPYONLY)

    # note the option ALL which allows to build the docs together with the application
    #add_custom_target(doc_doxygen ALL
    add_custom_target(doc_doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
