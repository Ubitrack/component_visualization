# - try to find glut library and include files
#  Freeglut_INCLUDE_DIR, where to find GL/glut.h, etc.
#  Freeglut_LIBRARIES, the libraries to link against
#  Freeglut_FOUND, If false, do not try to use Freeglut.
# Also defined, but not for general use are:
#  Freeglut_glut_LIBRARY = the full path to the glut library.
#  Freeglut_Xmu_LIBRARY  = the full path to the Xmu library.
#  Freeglut_Xi_LIBRARY   = the full path to the Xi Library.

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

IF (WIN32)
  FIND_PATH( Freeglut_INCLUDE_DIR NAMES GL/freeglut.h 
    PATHS  
	${Freeglut_ROOT_PATH}/include 	
	${FREEGLUT_ROOT_DIR}/include
	)
  FIND_LIBRARY( Freeglut_glut_LIBRARY NAMES glut glut32 freeglut freeglut_static
    PATHS
    ${OPENGL_LIBRARY_DIR}
    ${Freeglut_ROOT_PATH}/Release
	${FREEGLUT_ROOT_DIR}/lib
    )
ELSE (WIN32)
  
  IF (APPLE)
    # These values for Apple could probably do with improvement.
    #FIND_PATH( Freeglut_INCLUDE_DIR freeglut.h
    #  /System/Library/Frameworks/Freeglut.framework/Versions/A/Headers
    #  ${OPENGL_LIBRARY_DIR}
    #  )
    #SET(Freeglut_glut_LIBRARY "-framework Freeglut" CACHE STRING "Freeglut library for OSX") 
    #SET(Freeglut_cocoa_LIBRARY "-framework Cocoa" CACHE STRING "Cocoa framework for OSX")
    
    FIND_PATH( Freeglut_INCLUDE_DIR GL/freeglut.h
      /opt/local/include
      /usr/local/include
      )
  
    FIND_LIBRARY( Freeglut_glut_LIBRARY glut
      /opt/local/lib
      /usr/local/lib
      )
    
    FIND_LIBRARY( Freeglut_Xi_LIBRARY Xi
      /opt/local/lib
      /usr/local/lib
      )
    
    FIND_LIBRARY( Freeglut_Xmu_LIBRARY Xmu
      /opt/local/lib
      /usr/local/lib
      )


  ELSE (APPLE)
    
    FIND_PATH( Freeglut_INCLUDE_DIR GL/freeglut.h
      /usr/include
      /usr/local/include
      )
  
    FIND_LIBRARY( Freeglut_glut_LIBRARY glut
      /usr/lib
      /usr/local/lib
      )
    
    FIND_LIBRARY( Freeglut_Xi_LIBRARY Xi
      /usr/lib
      /usr/local/lib
      )
    
    FIND_LIBRARY( Freeglut_Xmu_LIBRARY Xmu
      /usr/lib
      /usr/local/lib
      )
    
  ENDIF (APPLE)
  
ENDIF (WIN32)

SET( Freeglut_FOUND "NO" )
IF(Freeglut_INCLUDE_DIR)
  IF(Freeglut_glut_LIBRARY)
    # Is -lXi and -lXmu required on all platforms that have it?
    # If not, we need some way to figure out what platform we are on.
    SET( Freeglut_LIBRARIES
      ${Freeglut_glut_LIBRARY}
      ${Freeglut_Xmu_LIBRARY}
      ${Freeglut_Xi_LIBRARY} 
      ${Freeglut_cocoa_LIBRARY}
      )
    SET( Freeglut_FOUND "YES" )
    
  ENDIF(Freeglut_glut_LIBRARY)
ENDIF(Freeglut_INCLUDE_DIR)

MARK_AS_ADVANCED(
  Freeglut_INCLUDE_DIR
  Freeglut_glut_LIBRARY
  Freeglut_Xmu_LIBRARY
  Freeglut_Xi_LIBRARY
  )
