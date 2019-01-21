
#list(REMOVE_ITEM CMAKE_CXX_FLAGS "-fno-exceptions")
#set(CMAKE_CXX_FLAGS "-fexceptions ${CMAKE_CXX_FLAGS}")

set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)

add_clang_executable(binder
  binder.cpp
  binder.hpp

  context.hpp
  context.cpp

  class.hpp
  class.cpp

  config.hpp
  config.cpp

  enum.hpp
  enum.cpp

  function.hpp
  function.cpp

  type.hpp
  type.cpp

  util.hpp
  util.cpp

  reporter.hpp
  reporter.cpp

  fmt/format.cc
  fmt/posix.cc
  )

target_link_libraries(binder
  clangTooling
  clangBasic
  clangASTMatchers
  )

install(TARGETS binder
  COMPONENT binder
  RUNTIME DESTINATION bin
  )

add_custom_target(install-binder
  DEPENDS binder
  COMMAND "${CMAKE_COMMAND}"
          -DCMAKE_INSTALL_COMPONENT=binder
          -P "${CMAKE_BINARY_DIR}/cmake_install.cmake")

# Development library

set(EXPORT_HEADERS
  include/binder/stl_binders.hpp
  )

set_source_files_properties(${EXPORT_HEADERS} PROPERTIES HEADER_FILE_ONLY ON)

add_library(libbinder INTERFACE)

add_dependencies(libbinder binder)

target_include_directories(libbinder
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/binder>
    $<INSTALL_INTERFACE:include/binder>
  )

install(FILES ${EXPORT_HEADERS}
  COMPONENT libbinder
  DESTINATION include/binder
  )

install(TARGETS libbinder
  COMPONENT libbinder
  EXPORT libbinder-exports
  INCLUDES DESTINATION include/binder
  )

add_custom_target(install-libbinder
  DEPENDS binder
  COMMAND "${CMAKE_COMMAND}"
          -DCMAKE_INSTALL_COMPONENT=libbinder
          -P "${CMAKE_BINARY_DIR}/cmake_install.cmake")

# Make sure that our source directory is on the current cmake module path so that
# we can include cmake files from this directory.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

add_subdirectory(cmake/modules)
