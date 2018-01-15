set(the_description "The UbiTrack Visualization Render Components")
ut_add_component(utvisualizationopengl DEPS utcore utdataflow utvision utvisualization)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

SET(ASSIMP_DIR ${EXTERNAL_LIBRARIES_DIR}/assimp)
FIND_PACKAGE(Assimp)

ut_component_include_directories(${UBITRACK_CORE_DEPS_INCLUDE_DIR} ${OPENCV_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIR})
ut_glob_component_sources(HEADERS "*.h" SOURCES "*.cpp")
ut_create_single_component(${OPENGL_LIBRARIES} ${OpenCL_LIBRARY} ${ASSIMP_LIBRARIES})


set(UBITRACK_LIBRARY_NAME utvisualizationopengl)

# All cpp files for utvisualization components
FILE(GLOB UBITRACK_SRCS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
   "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(${UBITRACK_LIBRARY_NAME} SHARED ${UBITRACK_SRCS})

target_link_libraries(${UBITRACK_LIBRARY_NAME} ${CONAN_LIBS} ${UBITRACK_EXTRA_LIBS})

# Apply Target Properties
if(MSVC)
  if(CMAKE_CROSSCOMPILING)
    set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:secchk")
  endif()
  set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:libc /DEBUG")
endif()

foreach(_flag ${UBITRACK_COMPILE_FLAGS})
  set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES COMPILE_FLAGS "${_flag}")
endforeach()
foreach(_flag ${UBITRACK_LINK_FLAGS})
  set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES LINK_FLAGS "${_flag}")
endforeach()
foreach(_flag ${UBITRACK_LINK_FLAGS_DEBUG})
  set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES LINK_FLAGS_DEBUG "${_flag}")
endforeach()
foreach(_symb ${UBITRACK_DEFINES})
  set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES DEFINE_SYMBOL ${_symb})
endforeach()

# set compiler Definitions
set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES COMPILE_DEFINITIONS "${UBITRACK_COMPILE_DEFINITIONS}")

# set fPIC
set_property(TARGET ${UBITRACK_LIBRARY_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES
  OUTPUT_NAME "${UBITRACK_LIBRARY_NAME}${UBITRACK_DLLVERSION}"
  DEBUG_POSTFIX "${UBITRACK_DEBUG_POSTFIX}"
  ARCHIVE_OUTPUT_DIRECTORY ${UBITRACK_COMPONENT_INSTALL_PATH}
  )

# Installation of libraries, headers and documents
install(TARGETS ${UBITRACK_LIBRARY_NAME}
  RUNTIME DESTINATION ${UBITRACK_COMPONENT_BIN_INSTALL_PATH}
  LIBRARY DESTINATION ${UBITRACK_COMPONENT_INSTALL_PATH}
  ARCHIVE DESTINATION ${UBITRACK_COMPONENT_INSTALL_PATH}
  )
