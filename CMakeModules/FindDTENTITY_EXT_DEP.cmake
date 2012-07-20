
INCLUDE(FindPackageHandleStandardArgs)

IF(DTENTITY_EXT_DEP_DIR)
  # in cache already
  SET(DTENTITY_EXT_DEP_FOUND TRUE)

ELSE(DTENTITY_EXT_DEP_DIR)
  SET(ENV_VIEWER_EXT_DEP $ENV{VIEWER_EXT_DEP})
  IF(NOT ENV_VIEWER_EXT_DEP EQUAL "")
  file(TO_CMAKE_PATH ENV_VIEWER_EXT_DEP EXTDEPPATH)
  ENDIF(NOT ENV_VIEWER_EXT_DEP EQUAL "")

  set(DTENTITY_EXT_DEP_PATHS ${EXTDEPPATH}
  "${CMAKE_SOURCE_DIR}/extdep"
    C:/dtEntity_ext_dep_vs_2008
  C:/dev/dtEntity_ext_dep_vs_2008
    d:/dev/dtEntity_ext_dep_vs_2008
  F:/dev/dtEntityextdep-vs10-x64
  )
  FIND_PATH(DTENTITY_EXT_DEP_DIR lib/osg.lib PATHS ${DTENTITY_EXT_DEP_PATHS})


  FIND_PACKAGE_HANDLE_STANDARD_ARGS(DTENTITY_EXT_DEP DEFAULT_MSG DTENTITY_EXT_DEP_DIR)
ENDIF(DTENTITY_EXT_DEP_DIR)

set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${DTENTITY_EXT_DEP_DIR}/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${DTENTITY_EXT_DEP_DIR}/lib)
