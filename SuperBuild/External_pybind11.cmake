
set(proj pybind11)

# Set dependency list
set(${proj}_DEPENDS "")

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()

# Sanity checks
if(DEFINED pybind11_DIR AND NOT EXISTS ${pybind11_DIR})
  message(FATAL_ERROR "pybind11_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/pybind/pybind11.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    "v2.2.4"
    QUIET
    )

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
  set(EP_INSTALL_DIR ${CMAKE_BINARY_DIR}/${proj}-install)

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}"
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    CMAKE_CACHE_ARGS
      # Compiler settings
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
      -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
      # Output directories
      #-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_BIN_DIR}
      #-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_LIB_DIR}
      #-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
      # Install directories
      #-Dpybind11_INSTALL_RUNTIME_DIR:STRING=${Slicer_INSTALL_THIRDPARTY_LIB_DIR}
      #-Dpybind11_INSTALL_LIBRARY_DIR:STRING=${Slicer_INSTALL_THIRDPARTY_LIB_DIR}
      # Options
      -DPYBIND11_MASTER_PROJECT:BOOL=ON
      -DPYBIND11_TEST:BOOL=OFF
      -DPYBIND11_INSTALL:BOOL=ON
      #-DPYBIND11_CMAKECONFIG_INSTALL_DIR:STRING="."
      -DCMAKE_INSTALL_PREFIX:PATH=${EP_INSTALL_DIR}

      -DPYTHON_EXECUTABLE:PATH=${PYTHON_EXECUTABLE}
      -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
      -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIR} 

    #INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDS}
    )
  set(${proj}_DIR "${EP_INSTALL_DIR}/share/cmake/pybind11")

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDS})
endif()

mark_as_superbuild(${proj}_DIR:PATH)

ExternalProject_Message(${proj} "${proj}_DIR:${${proj}_DIR}")
