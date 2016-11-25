SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
add_subdirectory(src/OpenCVComponents)
add_subdirectory(src/OpenGLComponents)
ut_install_utql_patterns()