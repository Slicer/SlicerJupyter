
set(proj xeus)

# Set dependency list
set(${proj}_DEPENDS nlohmann_json xtl ZeroMQ cppzmq)

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()

# Sanity checks
if(DEFINED xeus_DIR AND NOT EXISTS ${xeus_DIR})
  message(FATAL_ERROR "xeus_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/QuantStack/xeus.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    "7062ef5e561098c0797a1534e89bbaef9563c03d"  # 0.24.1 + "not -> !" fix
    QUIET
    )

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
 
  set(EXTERNAL_PROJECT_CMAKE_CACHE_ARGS)
  if(UNIX)
    list(APPEND EXTERNAL_PROJECT_CMAKE_CACHE_ARGS
      -DOPENSSL_SSL_LIBRARY:FILEPATH=${OPENSSL_SSL_LIBRARY}
      -DOPENSSL_CRYPTO_LIBRARY:FILEPATH=${OPENSSL_CRYPTO_LIBRARY}
      )
  else()
    list(APPEND EXTERNAL_PROJECT_CMAKE_CACHE_ARGS
      -DLIB_EAY_DEBUG:FILEPATH=${LIB_EAY_DEBUG}
      -DLIB_EAY_RELEASE:FILEPATH=${LIB_EAY_RELEASE}
      -DSSL_EAY_DEBUG:FILEPATH=${SSL_EAY_DEBUG}
      -DSSL_EAY_RELEASE:FILEPATH=${SSL_EAY_RELEASE}
      )
  endif()

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
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_BIN_DIR}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_LIB_DIR}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
      # Install directories
      -Dxeus_INSTALL_RUNTIME_DIR:STRING=${Slicer_INSTALL_THIRDPARTY_LIB_DIR}
      -Dxeus_INSTALL_LIBRARY_DIR:STRING=${Slicer_INSTALL_THIRDPARTY_LIB_DIR}
      -DCMAKE_INSTALL_LIBDIR:STRING=${Slicer_INSTALL_THIRDPARTY_LIB_DIR} # Skip default initialization by GNUInstallDirs CMake module
      # Options
      -DBUILD_TESTING:BOOL=OFF
      # Depdendencies
      -Dnlohmann_json_DIR:PATH=${nlohmann_json_DIR}
      -Dxtl_DIR:PATH=${xtl_DIR}
      -DZeroMQ_DIR:PATH=${ZeroMQ_DIR}
      -Dcppzmq_DIR:PATH=${cppzmq_DIR}
      -DOPENSSL_INCLUDE_DIR:PATH=${OPENSSL_INCLUDE_DIR}
      ${EXTERNAL_PROJECT_CMAKE_CACHE_ARGS}
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDS}
    )
  set(${proj}_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

  if(APPLE)
     ExternalProject_Add_Step(${proj} fix_rpath
       COMMAND install_name_tool -id ${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_LIB_DIR}/libxeus.1.dylib ${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_LIB_DIR}/libxeus.1.dylib
       DEPENDEES install
       )
  endif()

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDS})
endif()

mark_as_superbuild(${proj}_DIR:PATH)

ExternalProject_Message(${proj} "${proj}_DIR:${${proj}_DIR}")
