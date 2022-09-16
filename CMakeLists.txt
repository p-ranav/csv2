cmake_minimum_required(VERSION 3.8)

if(DEFINED PROJECT_NAME)
  set(CSV2_SUBPROJECT ON)
endif()

project(csv2 VERSION 1.8.0 LANGUAGES CXX)
  
if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    conan_basic_setup()
endif()

option(CSV2_BUILD_TESTS "Build csv2 tests + enable CTest")
option(CSV2_SAMPLES "Build csv2 samples")
option(CSV2_DEMO "Build csv2 demo" OFF)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

add_library(csv2 INTERFACE)
add_library(csv2::csv2 ALIAS csv2)

target_compile_features(csv2 INTERFACE cxx_std_11)
target_include_directories(csv2 INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

if(CSV2_BUILD_TESTS)
  add_subdirectory(test)
endif()

if(NOT CSV2_SUBPROJECT)
  configure_package_config_file(csv2Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/csv2Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/csv2)
  write_basic_package_version_file(csv2ConfigVersion.cmake
                                   COMPATIBILITY AnyNewerVersion)

  configure_file(csv2.pc.in csv2.pc @ONLY)

  install(TARGETS csv2 EXPORT csv2Targets)
  install(EXPORT csv2Targets
          FILE csv2Targets.cmake
          NAMESPACE csv2::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/csv2)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/csv2Config.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/csv2ConfigVersion.cmake
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/csv2)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/csv2.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/csv2
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          USE_SOURCE_PERMISSIONS
          PATTERN "*.hpp")
  install(FILES LICENSE LICENSE.mio
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/csv2)

  if(EXISTS "${PROJECT_SOURCE_DIR}/.gitignore")
    # Simple glob to regex conversion (.gitignore => CPACK_SOURCE_IGNORE_FILES)
    file(READ ".gitignore" DOT_GITIGNORE)
    string(REPLACE ";" "RANDOMSEQUENCE" DOT_GITIGNORE "${DOT_GITIGNORE}")
    string(REPLACE "\n" ";" DOT_GITIGNORE "${DOT_GITIGNORE}")
    string(REPLACE "RANDOMSEQUENCE" "\\;" DOT_GITIGNORE "${DOT_GITIGNORE}")
    foreach(IGNORE_LINE ${DOT_GITIGNORE})
      if(NOT IGNORE_LINE OR IGNORE_LINE MATCHES "^#")
        continue()
      endif()
      string(REPLACE "\\" "\\\\" IGNORE_LINE "${IGNORE_LINE}")
      string(REPLACE "." "\\\\." IGNORE_LINE "${IGNORE_LINE}")
      string(REPLACE "*" ".*" IGNORE_LINE "${IGNORE_LINE}")
      string(REPLACE "+" "\\\\+" IGNORE_LINE "${IGNORE_LINE}")
      list(APPEND CPACK_SOURCE_IGNORE_FILES "${IGNORE_LINE}")
    endforeach()
  endif()

  # extra ignored files
  list(APPEND CPACK_SOURCE_IGNORE_FILES
    .editorconfig
    .git
    .gitignore
    .travis.yml
    .appveyor.yml
  )
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
  set(CPACK_GENERATOR "TGZ;TXZ")
  set(CPACK_SOURCE_GENERATOR "TGZ;TXZ")
  include(CPack)
endif()
