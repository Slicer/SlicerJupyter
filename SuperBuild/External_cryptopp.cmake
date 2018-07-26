
set(proj cryptopp)

# Set dependency list
set(${proj}_DEPENDS "")

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()

# Sanity checks
if(DEFINED cryptopp_DIR AND NOT EXISTS ${cryptopp_DIR})
  message(FATAL_ERROR "cryptopp_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/weidai11/cryptopp.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    #CRYPTOPP_5_6_5
    origin/master
    QUIET
    )

  set(cryptopp_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}-source)
  ExternalProject_Add(cryptopp-source
    GIT_REPOSITORY "${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}"
    SOURCE_DIR ${cryptopp_SOURCE_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_cmake_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/noloader/cryptopp-cmake.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_cmake_GIT_TAG
    77b9d530e9715a19843524229c8c7a90128fb3ab
    QUIET
    )

  set(crypto_cmake_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
  set(EP_INSTALL_DIR ${CMAKE_BINARY_DIR}/${proj}-install)

  set(cryptopp_disable_sse3 OFF)
  if(UNIX AND NOT APPLE)
    # Avoid the following error on "g++ (Ubuntu 5.2.1-22ubuntu2) 5.2.1 20151010"
    # /usr/lib/gcc/x86_64-linux-gnu/5/include/tmmintrin.h:136:1:
    #   error: inlining failed in call to always_inline ‘__m128i _mm_shuffle_epi8(__m128i, __m128i)’:
    #          target specific option mismatch _mm_shuffle_epi8 (__m128i __X, __m128i __Y)
    set(cryptopp_disable_sse3 ON)
  endif()

  set(cryptopp_disable_asm OFF)
  if (WIN32)
    # Avoid linker errors in Visual Studio
    set(cryptopp_disable_asm ON)
  endif()

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${${CMAKE_PROJECT_NAME}_${proj}_cmake_GIT_REPOSITORY}"
    GIT_TAG "${${CMAKE_PROJECT_NAME}_${proj}_cmake_GIT_TAG}"
    SOURCE_DIR ${crypto_cmake_SOURCE_DIR}
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
      -Dcryptopp_INSTALL_RUNTIME_DIR:STRING=${Slicer_INSTALL_THIRDPARTY_LIB_DIR}
      -Dcryptopp_INSTALL_LIBRARY_DIR:STRING=${Slicer_INSTALL_THIRDPARTY_LIB_DIR}
      -DCMAKE_INSTALL_PREFIX:PATH=${EP_INSTALL_DIR}
      # Options
      -DSRC_DIR:PATH=${cryptopp_SOURCE_DIR}
      -DBUILD_TESTING:BOOL=OFF
      -DBUILD_SHARED:BOOL=FALSE
      -DBUILD_STATIC:BOOL=TRUE
      -DDISABLE_SSSE3:BOOL=${cryptopp_disable_sse3}
      -DDISABLE_ASM:BOOL=${cryptopp_disable_asm}
      -Dcryptocpp_DISPLAY_CMAKE_SUPPORT_WARNING:BOOL=OFF
    DEPENDS
      cryptopp-source
      ${${proj}_DEPENDS}
    )
  set(${proj}_DIR ${EP_INSTALL_DIR}/lib/cmake/cryptopp/)

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDS})
endif()

mark_as_superbuild(${proj}_DIR:PATH)

ExternalProject_Message(${proj} "${proj}_DIR:${${proj}_DIR}")
