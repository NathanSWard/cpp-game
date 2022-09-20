# MIT License 
# Copyright (c) 2018-Today Michele Adduci <adduci@tutanota.com>
#
# Project-related instructions

# Activate Testing, if given
if(BUILD_TESTING)
  message(AUTHOR_WARNING "Building Tests.")
  enable_testing()
endif()

#####################################
# Define Targets
#####################################
add_subdirectory(nova)
add_subdirectory(game)

#####################################
# Define Install Targets
#####################################
install(TARGETS
  nova
  game
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)

install(
  FILES 
    ${CMAKE_SOURCE_DIR}/project/nova/include/nova/nova.hpp 
    ${CMAKE_BINARY_DIR}/exports/nova_export.h
  DESTINATION 
    include/nova)

install(FILES  
  ${CMAKE_SOURCE_DIR}/LICENSE
DESTINATION .)