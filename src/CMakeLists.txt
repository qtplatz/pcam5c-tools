# CMakeLists.txt

add_subdirectory( pcam5c )
add_subdirectory( drivers )

set ( DKMS_PACKAGES ${DKMS_PACKAGES} PARENT_SCOPE )
