set(the_description "The UbiTrack Visualization Render Components")
ut_add_component(utvisualizationopengl DEPS utcore utdataflow utvision utvisualization)


SET(ASSIMP_ROOT_DIR ${EXTERNAL_LIBRARIES_DIR}/assimp)
FIND_PACKAGE(Assimp)

ut_component_include_directories(${UBITRACK_CORE_DEPS_INCLUDE_DIR} ${OPENCV_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIR})
ut_glob_component_sources(HEADERS "*.h" SOURCES "*.cpp")
ut_create_single_component(${OPENGL_LIBRARIES} ${OpenCL_LIBRARY} ${ASSIMP_LIBRARIES})