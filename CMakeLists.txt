cmake_minimum_required(VERSION 3.8)

project(SlicerJupyter)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://github.com/Slicer/SlicerJupyter")
set(EXTENSION_CATEGORY "Developer Tools")
set(EXTENSION_CONTRIBUTORS "Andras Lasso (PerkLab), Jean-Christophe Fillion-Robin (Kitware)")
set(EXTENSION_DESCRIPTION "This extension provides a Jupyter kernel, which allows running Jupyter notebooks in 3D Slicer.")
set(EXTENSION_ICONURL "https://raw.githubusercontent.com/Slicer/SlicerJupyter/master/SlicerJupyterLogo.png")
set(EXTENSION_SCREENSHOTURLS "https://raw.githubusercontent.com/NA-MIC/ProjectWeek/master/PW28_2018_GranCanaria/Projects/SlicerJupyter/NotebookOnly.png https://raw.githubusercontent.com/NA-MIC/ProjectWeek/master/PW28_2018_GranCanaria/Projects/SlicerJupyter/NotebookSideBySide.png https://raw.githubusercontent.com/NA-MIC/ProjectWeek/master/PW28_2018_GranCanaria/Projects/SlicerJupyter/JupyterLab.png")
set(EXTENSION_DEPENDS "NA") # Specified as a space separated string, a list or 'NA' if any
set(EXTENSION_BUILD_SUBDIRECTORY inner-build)

set(SUPERBUILD_TOPLEVEL_PROJECT inner)

#-----------------------------------------------------------------------------
# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})
mark_as_superbuild(Slicer_DIR)

find_package(Git REQUIRED)
mark_as_superbuild(GIT_EXECUTABLE)

#-----------------------------------------------------------------------------
# SuperBuild setup
option(${EXTENSION_NAME}_SUPERBUILD "Build ${EXTENSION_NAME} and the projects it depends on." ON)
mark_as_advanced(${EXTENSION_NAME}_SUPERBUILD)
if(${EXTENSION_NAME}_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(JupyterKernel)
add_subdirectory(JupyterNotebooks)

#-----------------------------------------------------------------------------
install(CODE "message(\"Installing python packages.\")")
install(DIRECTORY "${python_packages_DIR}/"
        DESTINATION ${Slicer_INSTALL_ROOT}${Slicer_BUNDLE_EXTENSIONS_LOCATION}
        COMPONENT RuntimeLibraries)

#-----------------------------------------------------------------------------
set(EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS)
list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${xeus_DIR};xeus;ALL;/") # xeus has no runtime install component
list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${xeus-python_DIR};xeus-python;ALL;/")
list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${ZeroMQ_DIR};ZeroMQ;Runtime;/") # this is not needed on Windows, but maybe needed on other platforms
list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${ZeroMQ_DIR};ZeroMQ;Unspecified;/") # on Windows, libzmq-v140-mt-4_2_5.dll installed by this
set(${EXTENSION_NAME}_CPACK_INSTALL_CMAKE_PROJECTS "${EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS}" CACHE STRING "List of external projects to install" FORCE)

#-----------------------------------------------------------------------------
list(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${EXTENSION_NAME};ALL;/")
list(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${${EXTENSION_NAME}_CPACK_INSTALL_CMAKE_PROJECTS}")
include(${Slicer_EXTENSION_GENERATE_CONFIG})
include(${Slicer_EXTENSION_CPACK})
