# MIT License 
#
# nova target instructions

#####################################
# Define Target Information
#####################################
set(TARGET_NAME nova)
set(TARGET_INCLUDE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TARGET_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/nova.cpp)

#####################################
# Support IDE Visualization
#####################################
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TARGET_SOURCES})

#####################################
# Define Target according to options
#####################################
if(BUILD_SHARED_LIBS)
  add_library(${TARGET_NAME} SHARED ${TARGET_SOURCES})
else()
  add_library(${TARGET_NAME} STATIC ${TARGET_SOURCES})
endif()

#####################################
# Define Target Properties and Optional OpenSSL Dependency
#####################################

# generate an export header to expose symbols when using the library
generate_export_header(${TARGET_NAME} EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/${TARGET_NAME}_export.h)

# add target properties such as debug postfix, MSVC Runtime and library version
set_target_properties(${TARGET_NAME} PROPERTIES 
                                      DEBUG_POSTFIX "-d" 
                                      MSVC_RUNTIME_LIBRARY "${MSVC_RUNTIME_TYPE}" 
                                      VERSION ${CMAKE_PROJECT_VERSION}
                                      SOVERSION ${CMAKE_PROJECT_VERSION})

# set target compile options as defined in the cmake/compiler_options.cmake Module
target_compile_options(${TARGET_NAME} PRIVATE ${compiler_options})

# add compiler definition WITH_OPENSSL, if found
target_compile_definitions(${TARGET_NAME} PRIVATE ${compiler_definitions})

# set target link options as defined in the cmake/compiler_options.cmake Module
target_link_options(${TARGET_NAME} PRIVATE ${linker_flags})

# include all the necessary directories
target_include_directories(${TARGET_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/exports>
    ${Boost_INCLUDE_DIRS}
  PRIVATE 
    ${TARGET_INCLUDE_FOLDER}
    # Any other extra include required
)

target_link_libraries(${TARGET_NAME} PUBLIC EnTT::EnTT tl::optional tl::expected range-v3::range-v3 Boost::headers)

if(BUILD_TESTING)
  list(APPEND TEST_CASES
    type_map_test
    system_builder_test
    system_test
    scheduler_test
    graph_test
    app_test
    bitset_test
    reflection_test
    registry_test
  )
  foreach(TEST_CASE ${TEST_CASES})
    add_executable(${TEST_CASE} ${CMAKE_CURRENT_SOURCE_DIR}/test/${TEST_CASE}.cpp)
    target_link_libraries(${TEST_CASE} PRIVATE ${TARGET_NAME} doctest::doctest)
    target_compile_options(${TEST_CASE} PRIVATE ${compiler_options})
    target_compile_definitions(${TEST_CASE} PRIVATE ${compiler_definitions})
    target_link_options(${TEST_CASE} PRIVATE ${linker_flags})

    target_include_directories(${TEST_CASE}
      PUBLIC
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/exports>
      PRIVATE 
        ${TARGET_INCLUDE_FOLDER}
        # Any other extra include required
    )

    add_test(NAME ${TEST_CASE} COMMAND ${TEST_CASE})
  endforeach(TEST_CASE ${TEST_CASES})
endif()