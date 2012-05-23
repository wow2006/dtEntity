IF(ENET_INCLUDE_DIR AND ENET_LIBRARIES)

  # in cache already
  SET(ENET_FOUND TRUE)

ELSE(ENET_INCLUDE_DIR AND ENET_LIBRARIES)

  INCLUDE(FindPackageHandleStandardArgs)
  INCLUDE(FindLibraryWithDebug)

  set(ENET_INC_PATHS
       /usr/include
       ${CMAKE_INCLUDE_PATH}
  )

  FIND_LIBRARY_WITH_DEBUG(ENET_LIBRARIES enet "")
  FIND_PATH(ENET_INCLUDE_DIR enet/enet.h PATHS ${ENET_INC_PATHS})
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(ENET DEFAULT_MSG ENET_LIBRARIES ENET_INCLUDE_DIR)

ENDIF(ENET_INCLUDE_DIR AND ENET_LIBRARIES)
