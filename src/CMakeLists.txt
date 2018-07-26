cmake_minimum_required(VERSION 3.7)
project(redbase)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES pf_manager.h common_thing.h pf_manager.cpp pf_buffer.h pf_common.h pf_hashtable.cpp pf_hashtable.h pf_buffer.cpp pf_filehandle.cpp pf_filehandle.h pf_pagehandle.cpp pf_pagehandle.h rm_manager.cpp rm_manager.h pf_error.h pf_error.cpp rm_error.cpp rm_error.h rm_commom.h rm_rid.h rm_record.cpp rm_record.h rm_filescan.cpp rm_filescan.h rm_filehandle.cpp rm_filehandle.h rm_pagehdr.h bitmap.cpp bitmap.h ix_manager.cpp ix_manager.h ix_indexhandle.cpp ix_indexhandle.h ix_common.h btree_node.cpp btree_node.h ix_error.cpp ix_error.h ix_indexscan.cpp ix_indexscan.h comp.cpp comp.h sm_manager.cpp sm_manager.h sm_error.cpp sm_error.h data_attr.h ql_manager.cpp ql_manager.h parse.h ql_error.cpp ql_error.h printer.cpp printer.h )
add_executable(dbcreate ${SOURCE_FILES} dbcreate.cpp )
add_executable(dbdestroy ${SOURCE_FILES} dbdestroy.cpp)
add_executable(redbase ${SOURCE_FILES} redbase.cpp RBparse.cpp RBparse.h)