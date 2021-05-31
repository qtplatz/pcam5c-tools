#
cmake_minimum_required ( VERSION 3.7 )  # for GREATER_EQUAL
cmake_policy( SET CMP0020 NEW )
cmake_policy( SET CMP0042 NEW )
cmake_policy( SET CMP0053 NEW )

project ( pcam5c-tools )

set ( CMAKE_EXPORT_COMPILE_COMMANDS ON )
set ( CMAKE_INSTALL_PREFIX /opt/pcam5c )

if ( CMAKE_CROSSCOMPILING )
  message( STATUS "------------- CROSSCOMPILING -------------" )
endif()

###################### tools and kernel-drivers  #########################
add_definitions( -DBOOST_BIND_GLOBAL_PLACEHOLDERS)
find_package( Boost 1.75 REQUIRED COMPONENTS date_time regex filesystem thread system program_options serialization iostreams )

add_subdirectory ( src )

if ( CMAKE_CROSSCOMPILING )
endif( CMAKE_CROSSCOMPILING )
