
set( QTPLATZ_SOURCE_DIR ${CMAKE_SOURCE_DIR}/../qtplatz )

add_custom_command(
  OUTPUT ${QTPLATZ_SOURCE_DIR}/bootstrap.sh
  COMMAND git clone http://github.com/qtplatz/qtplatz.git ${QTPLATZ_SORUCE_DIR}
  )

add_custom_command(
  OUTPUT ${QTPLATZ_BUILD_DIR}/qtplatz-config.cmake
  COMMAND ${TOOLS}/run-bootstrap.sh ${QTPLATZ_SOURCE_DIR}
  DEPENDS ${QTPLATZ_SOURCE_DIR}/bootstrap.sh
  )

add_custom_target(
  qtplatz
  COMMAND make package
  DEPENDS ${QTPLATZ_BUILD_DIR}/qtplatz-config.cmake
  WORKING_DIRECTORY ${QTPLATZ_BUILD_DIR}
  )
