set(Ejdb_FOUND true)

if(Ejdb_FOUND)
  set(Ejdb_LIBRARIES ejdb)
endif()

mark_as_advanced(Ejdb_INCLUDE_DIRS Ejdb_LIBRARIES)
