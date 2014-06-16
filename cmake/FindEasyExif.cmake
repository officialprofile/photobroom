

#rules for finding EasyExif
#based on http://www.vtk.org/Wiki/CMake:How_To_Find_Libraries

find_path(EASYEXIF_INCLUDE_DIR easyexif/exif.h
          PATHS ${CMAKE_BINARY_DIR}/3rd_party/root/include)

find_library(EASYEXIF_LIBRARY NAMES easyexif
             PATHS ${CMAKE_BINARY_DIR}/3rd_party/root/lib)

set(EASYEXIF_LIBRARIES ${EASYEXIF_LIBRARY} )
set(EASYEXIF_INCLUDE_DIRS ${EASYEXIF_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EasyExif DEFAULT_MSG EASYEXIF_LIBRARY EASYEXIF_INCLUDE_DIR)

mark_as_advanced(EASYEXIF_INCLUDE_DIR EASYEXIF_LIBRARY)
