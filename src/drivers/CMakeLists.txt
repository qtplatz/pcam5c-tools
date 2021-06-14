cmake_minimum_required( VERSION 3.7 )
# subdir
find_path( DKMS NAMES postinst.in prerm.in Makefile.in  PATHS "${CMAKE_CURRENT_SOURCE_DIR}/dkms" NO_CMAKE_FIND_ROOT_PATH )

add_subdirectory( dgmod )
add_subdirectory( csi2rx )
add_subdirectory( vdma )
add_subdirectory( ov5640-gpio )

set ( DKMS_PACKAGES ${DKMS_PACKAGES} PARENT_SCOPE )
