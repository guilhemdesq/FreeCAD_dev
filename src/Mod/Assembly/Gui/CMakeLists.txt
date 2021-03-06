if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Mod/Assembly/App
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
)

set(AssemblyGui_LIBS
    #${ODE_LIBRARIES}
    Assembly
    PartGui
    FreeCADGui
)

set(AssemblyGui_MOC_HDRS
    TaskAssemblyConstraints.h
    TaskDlgAssemblyConstraints.h
)
fc_wrap_cpp(AssemblyGui_MOC_SRCS ${AssemblyGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${AssemblyGui_MOC_SRCS})

qt4_add_resources(AssemblyGui_SRCS Resources/Assembly.qrc)

set(AssemblyGui_UIC_SRCS
    TaskAssemblyConstraints.ui
)
qt4_wrap_ui(AssemblyGui_UIC_HDRS ${AssemblyGui_UIC_SRCS})

SET(AssemblyGuiViewProvider_SRCS
    ViewProvider.cpp
    ViewProvider.h
    ViewProviderPart.cpp
    ViewProviderPart.h
    ViewProviderAssembly.cpp
    ViewProviderAssembly.h
    ViewProviderConstraint.cpp
    ViewProviderConstraint.h
    ViewProviderConstraintGroup.cpp
    ViewProviderConstraintGroup.h
    TaskDlgAssemblyConstraints.cpp
    TaskDlgAssemblyConstraints.h
    TaskAssemblyConstraints.h
    TaskAssemblyConstraints.cpp
)
SOURCE_GROUP("ViewProvider" FILES ${AssemblyGuiViewProvider_SRCS})

SET(AssemblyGuiModule_SRCS 
    AppAssemblyGui.cpp
    AppAssemblyGuiPy.cpp
    Command.cpp
    CommandConstraints.cpp
    Resources/Assembly.qrc
    qrc_Assembly.cxx
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)
SOURCE_GROUP("Module" FILES ${AssemblyGuiModule_SRCS})


SET(AssemblyGui_SRCS
    ${AssemblyGui_SRCS}
    ${AssemblyGuiViewProvider_SRCS}
    ${AssemblyGuiModule_SRCS}
    ${AssemblyGui_UIC_HDRS}
)

add_library(AssemblyGui SHARED ${AssemblyGui_SRCS})
target_link_libraries(AssemblyGui ${AssemblyGui_LIBS})


fc_target_copy_resource(AssemblyGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Assembly
    ${CMAKE_BINARY_DIR}/Mod/Assembly
    InitGui.py)

if(MSVC)
    set_target_properties(AssemblyGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(AssemblyGui PROPERTIES DEBUG_OUTPUT_NAME "AssemblyGui_d")
    set_target_properties(AssemblyGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Assembly)
    set_target_properties(AssemblyGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(AssemblyGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(AssemblyGui PROPERTIES DEBUG_OUTPUT_NAME "AssemblyGui_d")
    set_target_properties(AssemblyGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Assembly)
    set_target_properties(AssemblyGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(AssemblyGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Assembly)
    set_target_properties(AssemblyGui PROPERTIES PREFIX "")
    SET_TARGET_PROPERTIES(AssemblyGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS AssemblyGui DESTINATION lib)
