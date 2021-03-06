SET(LIB_NAME dtEntityOSG)


INCLUDE_DIRECTORIES( 
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/
  ${OSG_INCLUDE_DIR}
)

SET(HEADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../include/${LIB_NAME})

SET(LIB_PUBLIC_HEADERS
  ${HEADER_PATH}/cameracomponent.h
  ${HEADER_PATH}/componentfactories.h
  ${HEADER_PATH}/export.h
  ${HEADER_PATH}/groupcomponent.h
  ${HEADER_PATH}/initosgviewer.h
  ${HEADER_PATH}/layerattachpointcomponent.h
  ${HEADER_PATH}/layercomponent.h
  ${HEADER_PATH}/lightcomponent.h
  ${HEADER_PATH}/matrixtransformcomponent.h
  ${HEADER_PATH}/nodecomponent.h
  ${HEADER_PATH}/nodemaskvisitor.h
  ${HEADER_PATH}/osganimationcomponent.h
  ${HEADER_PATH}/osgdebugdrawinterface.h
  ${HEADER_PATH}/osginputinterface.h
  ${HEADER_PATH}/osgsysteminterface.h
  ${HEADER_PATH}/osgwindowinterface.h
  ${HEADER_PATH}/pickshapecomponent.h
  ${HEADER_PATH}/positionattitudetransformcomponent.h
  ${HEADER_PATH}/shadercomponent.h
  ${HEADER_PATH}/shadowcomponent.h
  ${HEADER_PATH}/skyboxcomponent.h
  ${HEADER_PATH}/staticmeshcomponent.h
  ${HEADER_PATH}/textlabelcomponent.h
  ${HEADER_PATH}/texturelabelcomponent.h
  ${HEADER_PATH}/transformcomponent.h
)

SET(LIB_SOURCES
  componentfactories.cpp
  initosgviewer.cpp
)

SET(LIB_SOURCES_REPLACE  
  cameracomponent.cpp
  groupcomponent.cpp
  layerattachpointcomponent.cpp
  layercomponent.cpp
  lightcomponent.cpp
  matrixtransformcomponent.cpp
  nodecomponent.cpp
  osganimationcomponent.cpp
  osgdebugdrawinterface.cpp
  osginputinterface.cpp
  osgsysteminterface.cpp
  osgwindowinterface.cpp
  pickshapecomponent.cpp
  positionattitudetransformcomponent.cpp
  shadercomponent.cpp
  shadowcomponent.cpp
  skyboxcomponent.cpp
  staticmeshcomponent.cpp
  textlabelcomponent.cpp
  texturelabelcomponent.cpp  
  transformcomponent.cpp
)


SET(WRAPLIBRARIES dtEntity
                  ${OPENSCENEGRAPH_LIBRARIES}
                  ${OPENTHREADS_LIBRARIES}
)



REPLACE_SIDS_IN_SOURCE(LIB_SOURCES_OUT ${LIB_SOURCES_REPLACE})

ADD_LIBRARY(${LIB_NAME} ${DTENTITY_LIBS_DYNAMIC_OR_STATIC}
  ${LIB_PUBLIC_HEADERS}
  ${LIB_SOURCES}
  ${LIB_SOURCES_OUT}
  ${LIB_SOURCES_REPLACE}
)

TARGET_LINK_LIBRARIES( ${LIB_NAME} ${WRAPLIBRARIES})


SET_TARGET_PROPERTIES(${LIB_NAME}
	PROPERTIES DEFINE_SYMBOL DTENTITY_OSG_LIBRARY)
INCLUDE(ModuleInstall OPTIONAL)

IF (MSVC)
  ADD_DEFINITIONS(-DNODEFAULTLIB:LIBCMT)
  ADD_DEFINITIONS(-DNODEFAULTLIB:LIBCMTD)
ENDIF (MSVC)

SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}")


