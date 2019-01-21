# Generate a list of CMake library targets so that other CMake projects can
# link against them. LLVM calls its version of this file LLVMExports.cmake, but
# the usual CMake convention seems to be ${Project}Targets.cmake.

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(BINDER_INSTALL_PACKAGE_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/binder)
set(BINDER_INSTALL_BINDIR ${CMAKE_INSTALL_FULL_BINDIR})
set(BINDER_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/binder)
set(binder_cmake_builddir "${CMAKE_CURRENT_BINARY_DIR}/${BINDER_INSTALL_PACKAGE_DIR}")

export(EXPORT libbinder-exports
  FILE ${binder_cmake_builddir}/BinderTargets.cmake
  )

configure_package_config_file("BinderConfig.cmake.in"
  ${binder_cmake_builddir}/BinderConfig.cmake
  INSTALL_DESTINATION
    ${BINDER_INSTALL_PACKAGE_DIR}
  PATH_VARS
    BINDER_INSTALL_PACKAGE_DIR
    BINDER_INSTALL_BINDIR
    BINDER_INSTALL_INCLUDE_DIR
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

if (NOT LLVM_INSTALL_TOOLCHAIN_ONLY)
  set_property(GLOBAL PROPERTY CLANG_HAS_EXPORTS True)
  set_property(GLOBAL APPEND PROPERTY CLANG_EXPORTS libbinder)

  install(EXPORT libbinder-exports
    COMPONENT libbinder
    FILE BinderTargets.cmake
    DESTINATION ${BINDER_INSTALL_PACKAGE_DIR}
    )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDER_INSTALL_PACKAGE_DIR}/BinderConfig.cmake
    COMPONENT libbinder
    DESTINATION ${BINDER_INSTALL_PACKAGE_DIR})
endif()
