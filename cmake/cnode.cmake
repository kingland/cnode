set(cnode_sources		
	src/cnode.cc
	src/cnode_buffer.cc
	src/cnode_v8.cc
	src/csmalloc.cc
	src/cstring_bytes.cc
	src/cutil.cc
	src/cnode_js.cc
	src/cnode_watchdog.cc
	src/cnode_contextify.cc
	src/cnode_uv.cc
	src/casync_wrap.cc
	src/cares_wrap.cc
	src/chandle_wrap.cc
	src/cfsevent_wrap.cc
	src/cnode_constants.cc
	src/cnode_counters.cc
	src/cnode_statwatcher.cc
	src/cnode_file.cc
	src/cnode_os.cc
	src/ctime_wrap.cc
	src/cpipe_wrap.cc
	src/ctcp_wrap.cc
	src/cudp_wrap.cc
	src/cstream_wrap.cc
	src/ctty_wrap.cc
	src/cuv.cc
	src/cnode_crypto_bio.cc
	src/cnode_crypto_clienthello.cc
	src/cnode_crypto.cc
	src/ctls_wrap.cc
)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
set(CMAKE_C_FLAGS_DEBUG "-O0 -Wall -g   $ENV{CFLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -g  $ENV{CXXFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
else()
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG $ENV{CFLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE " -O3 -DNDEBUG $ENV{CXXFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
endif()

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -lv8 -lwinmm ")

#set(CMAKE_LIBRARY_PATH "C:/TDM-GCC-64/lib")

#set(CCFLAGS "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_C_FLAGS}")
#set(CCFLAGS "${CMAKE_CXX_FLAGS_RELEASE} ")



include_directories(
src
)

add_library(cnode ${cnode_sources})
 
#add_executable(v8 ${v8_sources})
target_link_libraries(cnode)
#find_library(LIBV8_LIBRARY NAMES v8)