
#usage:
#addTestTarget(`target` SOURCES source files TEST_LIBRARY gtest|gmock)

function(addTestTarget target)

    find_package(Threads)

    #get sources
    parseArguments(SOURCES TEST_LIBRARY ARGUMENTS ${ARGN})

    #add test executable
    add_executable(${target}_test ${SOURCES})

    #find which library should be used
    if("${TEST_LIBRARY}" STREQUAL "GTEST")

        find_package(GTest)
        include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})
        set(link_library ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY})

    elseif("${TEST_LIBRARY}" STREQUAL "GMOCK")

        find_package(GTest REQUIRED)
        find_package(GMock REQUIRED)
        include_directories(SYSTEM ${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS})
        set(link_library ${GMOCK_MAIN_LIBRARY} ${GMOCK_LIBRARY})

    else()
        message(FATAL_ERROR "For 'mode' argument use 'GTEST' or 'GMOCK'. Currently ${mode} was provided")
    endif()

    #link agains test library
    target_link_libraries(${target}_test ${link_library} ${CMAKE_THREAD_LIBS_INIT})

    #enable code coverage
    enableCodeCoverage(${target}_test)

    #add test
    add_test(${target}_test ${target}_test)

endfunction(addTestTarget)
