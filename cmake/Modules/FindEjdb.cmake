set(Ejdb_FOUND true)

if(Ejdb_FOUND)
    set(Ejdb_LIBRARIES ejdb)
    set(Ejdb_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/ejdb/dist/include")
    link_directories("${PROJECT_SOURCE_DIR}/ejdb/dist/lib")
endif()

mark_as_advanced(Ejdb_INCLUDE_DIRS Ejdb_LIBRARIES)
