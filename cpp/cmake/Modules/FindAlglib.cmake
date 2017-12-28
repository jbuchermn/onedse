#
# Find alglib in dependencies folder. Variables set:
#  Alglib_INCLUDE_DIR 
#  Alglib_LIBRARIES   
#  Alglib_FOUND       

if(Alglib_INCLUDE_DIR)
  set (Alglib_FIND_QUIETLY TRUE)
endif(Alglib_INCLUDE_DIR)

find_path(    Alglib_INCLUDE_DIR  NAMES alglib/ap.h PATHS "${CMAKE_SOURCE_DIR}/dependencies" PATH_SUFFIXES alglib)
find_library( Alglib_LIBRARIES    NAMES alglib      PATHS "${CMAKE_SOURCE_DIR}/dependencies" PATH_SUFFIXES alglib)

# handle the QUIETLY and REQUIRED arguments and set Alglib_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Alglib DEFAULT_MSG Alglib_LIBRARIES Alglib_INCLUDE_DIR)

mark_as_advanced(Alglib_LIBRARIES Alglib_INCLUDE_DIR)
