set(proj python-packages)

# Set dependency list
set(${proj}_DEPENDENCIES "")

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  ExternalProject_FindPythonPackage(
    MODULE_NAME "jedi"
    REQUIRED
    )
endif()

if(NOT DEFINED ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  set(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} ${Slicer_USE_SYSTEM_python})
endif()

if(NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_jedi_VERSION
    "0.18.0"
    QUIET
    )

  # argon2-cffi package is needed by JupyterLab.
  # It does not have binary wheels, therefore we need build it manually.
  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_argon2_cffi_VERSION
    "20.1.0"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_xeus_python_shell_VERSION
    "0.2.0"
    QUIET
    )

  # Alternative python prefix for installing extension python packages
  set(python_packages_DIR "${CMAKE_BINARY_DIR}/python-packages-install")
  file(TO_NATIVE_PATH ${python_packages_DIR} python_packages_DIR_NATIVE_DIR)

  set(python_sitepackages_DIR "${CMAKE_BINARY_DIR}/python-packages-install/${PYTHON_SITE_PACKAGES_SUBDIR}")
  file(TO_NATIVE_PATH ${python_sitepackages_DIR} python_sitepackages_DIR_NATIVE_DIR)


  set(_no_binary "")

  # Install jedi and requirements
  # note: --force-reinstall ensures the python dependency is installed within
  #       this library's prefix for packaging.
  set(_install_jedi COMMAND ${CMAKE_COMMAND}
      -E env
        PYTHONNOUSERSITE=1
        CC=${CMAKE_C_COMPILER}
        PYTHONPATH=${python_sitepackages_DIR}
        ${wrapper_script} ${PYTHON_EXECUTABLE} -m pip install
          jedi==${${CMAKE_PROJECT_NAME}_jedi_VERSION}
          argon2-cffi==${${CMAKE_PROJECT_NAME}_argon2_cffi_VERSION}
          xeus-python-shell==${${CMAKE_PROJECT_NAME}_xeus_python_shell_VERSION}
          ${_no_binary}
          --prefix ${python_packages_DIR_NATIVE_DIR}
          --force-reinstall
          --no-warn-script-location
    )

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    SOURCE_DIR ${proj}
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    DOWNLOAD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E  echo_append ""
    ${_install_jedi}
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  ExternalProject_GenerateProjectDescription_Step(${proj}
    VERSION ${${CMAKE_PROJECT_NAME}_${proj}_VERSION}
    LICENSE_FILES
      "https://raw.githubusercontent.com/davidhalter/jedi/master/LICENSE.txt"
    )

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree
  set(${proj}_PYTHONPATH_LAUNCHER_BUILD
    ${python_packages_DIR}/${PYTHON_STDLIB_SUBDIR}
    ${python_packages_DIR}/${PYTHON_STDLIB_SUBDIR}/lib-dynload
    ${python_packages_DIR}/${PYTHON_SITE_PACKAGES_SUBDIR}
    )
  mark_as_superbuild(
    VARS ${proj}_PYTHONPATH_LAUNCHER_BUILD
    LABELS "PYTHONPATH_LAUNCHER_BUILD"
    )

  mark_as_superbuild(python_packages_DIR:PATH)

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()
