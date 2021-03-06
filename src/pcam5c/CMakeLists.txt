#
project ( pcam5c )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}" )
set ( CMAKE_CXX_STANDARD 17 )

find_package( Boost )
if ( Boost_FOUND )
  message( STATUS "# Boost        : " ${Boost_VERSION} "\t" ${Boost_INCLUDE_DIRS} )
  find_package( Boost COMPONENTS program_options system filesystem json )
  add_definitions( -DHAVE_BOOST=1 )
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  )

add_executable( ${PROJECT_NAME}
  main.cpp
  gpio.cpp
  gpio.hpp
  pcam5c.cpp
  i2c.cpp
  i2c.hpp
  ov5640.cpp
  ov5640.hpp
  uio.cpp
  uio.hpp
  csi2rx.cpp
  csi2rx.hpp
  d_phyrx.cpp
  d_phyrx.hpp
  )

target_link_libraries( ${PROJECT_NAME} LINK_PUBLIC
  ${Boost_LIBRARIES}
  )

install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT tools )
