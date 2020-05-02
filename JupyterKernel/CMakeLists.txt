
#-----------------------------------------------------------------------------
set(MODULE_NAME JupyterKernel)
set(MODULE_TITLE ${MODULE_NAME})

string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

find_package(xeus REQUIRED)
find_package(xeus-python REQUIRED)
find_package(pybind11 REQUIRED)

#-----------------------------------------------------------------------------
add_subdirectory(Logic)

#-----------------------------------------------------------------------------
set(MODULE_EXPORT_DIRECTIVE "Q_SLICER_QTMODULES_${MODULE_NAME_UPPER}_EXPORT")

# Current_{source,binary} and Slicer_{Libs,Base} already included
set(MODULE_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  ${pybind11_INCLUDE_DIR}
  )

set(MODULE_SRCS
  qSlicer${MODULE_NAME}Module.cxx
  qSlicer${MODULE_NAME}Module.h
  qSlicer${MODULE_NAME}ModuleWidget.cxx
  qSlicer${MODULE_NAME}ModuleWidget.h
  xSlicerInterpreter.cxx
  xSlicerInterpreter.h
  xSlicerServer.cxx
  xSlicerServer.h  
  )

set(MODULE_MOC_SRCS
  qSlicer${MODULE_NAME}Module.h
  qSlicer${MODULE_NAME}ModuleWidget.h
  )

set(MODULE_UI_SRCS
  Resources/UI/qSlicer${MODULE_NAME}ModuleWidget.ui
  )

set(MODULE_TARGET_LIBRARIES
  vtkSlicer${MODULE_NAME}ModuleLogic
  xeus
  xeus-python
  )

set(MODULE_RESOURCES
  Resources/qSlicer${MODULE_NAME}Module.qrc
  )

#-----------------------------------------------------------------------------
slicerMacroBuildLoadableModule(
  NAME ${MODULE_NAME}
  TITLE ${MODULE_TITLE}
  EXPORT_DIRECTIVE ${MODULE_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${MODULE_INCLUDE_DIRECTORIES}
  SRCS ${MODULE_SRCS}
  MOC_SRCS ${MODULE_MOC_SRCS}
  UI_SRCS ${MODULE_UI_SRCS}
  TARGET_LIBRARIES ${MODULE_TARGET_LIBRARIES}
  RESOURCES ${MODULE_RESOURCES}
  WITH_GENERIC_TESTS
  )

#-----------------------------------------------------------------------------
# Create kernel specification template

# Build tree
configure_file(
  Resources/kernel-template.json.in
  ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/${MODULE_NAME}/${Slicer_MAIN_PROJECT_APPLICATION_NAME}-${Slicer_VERSION}/kernel-template.json
  @ONLY
  )
configure_file(
  Resources/kernel-configure.py
  ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/${MODULE_NAME}/${Slicer_MAIN_PROJECT_APPLICATION_NAME}-${Slicer_VERSION}/kernel-configure.py
  @ONLY
  )
# Install tree
configure_file(
  Resources/kernel-template.json.in
  ${SlicerJupyter_BINARY_DIR}/kernel-template.json
  @ONLY
  )
install(
  FILES ${SlicerJupyter_BINARY_DIR}/kernel-template.json
  DESTINATION ${Slicer_INSTALL_QTLOADABLEMODULES_SHARE_DIR}/${MODULE_NAME}/${Slicer_MAIN_PROJECT_APPLICATION_NAME}-${Slicer_VERSION} COMPONENT Runtime
  )
install(
  FILES Resources/kernel-configure.py
  DESTINATION ${Slicer_INSTALL_QTLOADABLEMODULES_SHARE_DIR}/${MODULE_NAME}/${Slicer_MAIN_PROJECT_APPLICATION_NAME}-${Slicer_VERSION} COMPONENT Runtime
  )

foreach(res IN ITEMS 32 64)
  # Build tree
  configure_file(
    Resources/Icons/Slicer-${res}x${res}.png
    ${CMAKE_BINARY_DIR}/${Slicer_QTLOADABLEMODULES_SHARE_DIR}/${MODULE_NAME}/${Slicer_MAIN_PROJECT_APPLICATION_NAME}-${Slicer_VERSION}/logo-${res}x${res}.png
    COPYONLY
    )
  # Install tree
  install(
    FILES Resources/Icons/Slicer-${res}x${res}.png
    DESTINATION ${Slicer_INSTALL_QTLOADABLEMODULES_SHARE_DIR}/${MODULE_NAME}/${Slicer_MAIN_PROJECT_APPLICATION_NAME}-${Slicer_VERSION} COMPONENT Runtime
    )
endforeach()

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
