# MIT License 
# Copyright (c) 2018-Today Michele Adduci <adduci@tutanota.com>
#
# Dependencies

# Required for Testing
if(BUILD_TESTING)
	find_package(doctest CONFIG REQUIRED)
endif()

# List Dependencies
find_package(EnTT CONFIG REQUIRED)
find_package(tl-optional CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(SFML CONFIG REQUIRED)