if(WIN32)
    add_definitions(-DFCAppPoints)
endif(WIN32)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

set(Points_LIBS
    FreeCADApp
)

generate_from_xml(PointsPy)

SET(Points_SRCS
    AppPoints.cpp
    AppPointsPy.cpp
    FeaturePointsImportAscii.cpp
    FeaturePointsImportAscii.h
    Points.cpp
    Points.h
    PointsPy.xml
    PointsPyImp.cpp
    PointsAlgos.cpp
    PointsAlgos.h
    PointsFeature.cpp
    PointsFeature.h
    PointsGrid.cpp
    PointsGrid.h
    PreCompiled.cpp
    PreCompiled.h
    Properties.cpp
    Properties.h
    PropertyPointKernel.cpp
    PropertyPointKernel.h
)

add_library(Points SHARED ${Points_SRCS})

target_link_libraries(Points ${Points_LIBS})


fc_target_copy_resource(Points 
    ${CMAKE_SOURCE_DIR}/src/Mod/Points
    ${CMAKE_BINARY_DIR}/Mod/Points
    Init.py)

if(MSVC)
    set_target_properties(Points PROPERTIES SUFFIX ".pyd")
    set_target_properties(Points PROPERTIES DEBUG_OUTPUT_NAME "Points_d")
    set_target_properties(Points PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Points)
    set_target_properties(Points PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Points PROPERTIES SUFFIX ".pyd")
    set_target_properties(Points PROPERTIES DEBUG_OUTPUT_NAME "Points_d")
    set_target_properties(Points PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Points)
    set_target_properties(Points PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Points PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Points)
    set_target_properties(Points PROPERTIES PREFIX "")
    set_target_properties(Points PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Points DESTINATION lib)
