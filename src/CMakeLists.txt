set(CMAKE_INCLUDE_CURRENT_DIR ON)
IF(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
ELSE()
  set(CMAKE_CXX_FLAGS "-std=gnu++0x")
ENDIF()

IF(APPLE)
  set(additional_includes
    /usr/include/tidy
    /usr/include/libxml2
    /usr/local/opt/icu4c/include
  )
  
  set(additional_lib_searchpath
    /usr/local/opt/icu4c/lib
  )
ELSE()
  set(additional_includes
    /usr/include/libxml2
  )
ENDIF()

# icu4c

find_path(ICU4C_INCLUDE_DIR
  NAMES unicode/utf8.h
  PATHS ${additional_includes}
)
find_library(ICU4C_LIBRARY
  NAMES icuuc
  PATHS ${additional_lib_searchpath}
)

if(NOT ICU4C_INCLUDE_DIR OR NOT ICU4C_LIBRARY)
  message(FATAL_ERROR "ERROR: Could not find icu4c")
else()
  message(STATUS "Found icu4c")
endif()

# leveldb

find_path(LEVELDB_INCLUDE_DIR
  NAMES leveldb/db.h
  PATHS ${additional_includes}
)
find_library(LEVELDB_LIBRARY
  NAMES leveldb
  PATHS ${additional_lib_searchpath}
)

if(NOT LEVELDB_INCLUDE_DIR OR NOT LEVELDB_LIBRARY)
  message(FATAL_ERROR "ERROR: Could not find leveldb")
else()
  message(STATUS "Found leveldb")
endif()


file(GLOB_RECURSE
    source_files
    *.h
    *.c
    *.cpp
)

# SET_SOURCE_FILES_PROPERTIES(
#   ${source_files}
#     PROPERTIES LANGUAGE C
# )

file(COPY
lidx.h
DESTINATION
${CMAKE_CURRENT_BINARY_DIR}/include/kvdb
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/include"
  ${additional_includes}
)

add_library (lidx
    lidx-encode.cpp
    lidx-icu-utils.c
    lidx.cpp
)
