
include(${CMAKE_SOURCE_DIR}/cmake/functions.cmake)

find_package(Qt6 REQUIRED COMPONENTS Gui Sql Test)

add_definitions(-DSTATIC_PLUGINS)  # build in all plugins

addTestTarget(database_backends
                SOURCES
                    # engines
                    backends/sql_backends/sqlite_backend/backend.cpp
                    #backends/sql_backends/mysql_backend/backend.cpp
                    #backends/sql_backends/mysql_backend/mysql_server.cpp
                    # memory backend linked

                    # other sql stuff
                    backends/sql_backends/generic_sql_query_constructor.cpp
                    backends/sql_backends/group_operator.cpp
                    backends/sql_backends/people_information_accessor.cpp
                    backends/sql_backends/photo_change_log_operator.cpp
                    backends/sql_backends/photo_operator.cpp
                    backends/sql_backends/sql_filter_query_generator.cpp
                    backends/sql_backends/sql_query_executor.cpp
                    backends/sql_backends/query_structs.cpp
                    backends/sql_backends/sql_backend.cpp
                    backends/sql_backends/table_definition.cpp
                    backends/sql_backends/tables.cpp
                    backends/sql_backends/transaction.cpp

                    # sql tests:
                    unit_tests_for_backends/common.hpp
                    unit_tests_for_backends/general_flags_tests.cpp
                    unit_tests_for_backends/groups_tests.cpp
                    unit_tests_for_backends/people_tests.cpp
                    unit_tests_for_backends/photo_operator_tests.cpp
                    unit_tests_for_backends/photos_change_log_tests.cpp
                    unit_tests_for_backends/photos_tests.cpp
                    unit_tests_for_backends/tags_tests.cpp
                    unit_tests_for_backends/thumbnails_tests.cpp
                    unit_tests_for_backends/transaction_accumulations_tests.cpp

                    # dependencies
                    implementation/apeople_information_accessor.cpp
                    implementation/aphoto_change_log_operator.cpp
                    implementation/notifications_accumulator.cpp

                    # main()
                    unit_tests_for_backends/main.cpp

                LIBRARIES
                    core
                    database
                    database_memory_backend
                    plugins
                    sample_dbs
                    system
                    Qt::Core
                    Qt::Gui
                    Qt::Sql
                    Qt::Test
                    GTest::gtest
                    GTest::gmock

                INCLUDES
                    ${CMAKE_SOURCE_DIR}/src
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}/backends/sql_backends
                    ${CMAKE_CURRENT_BINARY_DIR}/backends/sql_backends/sqlite_backend
                    ${CMAKE_CURRENT_BINARY_DIR}/backends/sql_backends/mysql_backend

                DEFINITIONS
                    STATIC_PLUGINS
                    DATABASE_MYSQL_BACKEND_STATIC_DEFINE    # disable visibility mechanisms to prevent inconsistent dll linkage warnings 
                    DATABASE_SQLITE_BACKEND_STATIC_DEFINE   # disable visibility mechanisms to prevent inconsistent dll linkage warnings 
                    DATABASE_STATIC_DEFINE                  # disable visibility mechanisms to prevent inconsistent dll linkage warnings 
)
