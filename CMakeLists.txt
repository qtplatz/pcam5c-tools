#
cmake_minimum_required ( VERSION 3.7 )  # Xilinx Vitis (2020.2) contains 3.3.2, which does not support C++17
cmake_policy( SET CMP0020 NEW )
cmake_policy( SET CMP0042 NEW )
cmake_policy( SET CMP0053 NEW )

project ( pcam5c-tools )

set ( CMAKE_EXPORT_COMPILE_COMMANDS ON )
set ( CMAKE_INSTALL_PREFIX /usr/local )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" )
include (version)

if ( CMAKE_CROSSCOMPILING )
  message( STATUS "------------- CROSSCOMPILING -------------" )
endif()

list ( APPEND CMAKE_INSTALL_RPATH ${CPACK_INSTALL_PREFIX}/lib )
set ( CPACK_SET_DESTDIR OFF )         # override qtplatz-config.cmake
set ( CPACK_INSTALL_PREFIX "/" )      # override qtplatz-config.cmake
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/lib ) # static libs, dll's import lib/exp
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/lib ) # plugins
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/bin ) # .exe | .dll
set ( CPACK_PACKAGING_INSTALL_PREFIX "/" ) # <-- must be root for "/lib/module/4.4.52/..."

include( version )
set ( CMAKE_CXX_STANDARD 17 )
message( STATUS "## version: " "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}(${VERSION_HASH})" )

###################### tools and kernel-drivers  #########################
add_definitions( -DBOOST_BIND_GLOBAL_PLACEHOLDERS)
find_package( Boost 1.75 REQUIRED COMPONENTS
  date_time
  filesystem
  json
  program_options
  regex
  system
  thread
  )

set ( DKMS_PACKAGES ) # This variable will be filled via drivers/dkms/dkms.cmake
set ( KERNELRELEASE "xlnx" )

add_subdirectory ( src )

include( CPackComponent )
cpack_add_component( pcam5c  DISPLAY_NAME "pcam5c tools" GROUP tools )
cpack_add_component_group( tools )

set( CPACK_DEB_COMPONENT_INSTALL 1 )
set( CPACK_PACKAGE_NAME pcam5c )
set( CPACK_PACKAGE_VENDOR "MS-Cheminformatics LLC" )
set( CPACK_PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK} )
set( CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR} )
set( CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR} )
set( CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH} )
set( CPACK_GENERATOR "DEB" )
set( CPACK_PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK} )
set( CPACK_DEBIAN_PACKAGE_MAINTAINER "T. Hondo <toshi.hondo@qtplatz.com>" )
set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE armhf )

###############################################
# dkms PACKAGES ---------
message( STATUS "### top level DKMS_PACKAGES=${DKMS_PACKAGES}" )

foreach( i ${DKMS_PACKAGES} )
  string( TOUPPER ${i} MODNAME )
  set ( CPACK_DEBIAN_${MODNAME}_PACKAGE_CONTROL_EXTRA
    "${CMAKE_BINARY_DIR}/src/drivers/${i}/dkms/postinst;${CMAKE_BINARY_DIR}/src/drivers/${i}/dkms/prerm" )
  set ( CPACK_DEBIAN_${MODNAME}_PACKAGE_DEPENDS dkms )
  message( STATUS "DKMS_PACKAGE: ${i} ${MODULE}" )
endforeach()
#---------------------------------

include( CPack )
