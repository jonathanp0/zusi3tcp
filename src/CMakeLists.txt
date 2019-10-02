project(zusi3tcplib VERSION 0.1 LANGUAGES C CXX)

set ( SOURCES Zusi3TCP.cpp )
set ( HEADERS include/Zusi3TCP.h include/Zusi3TCPTypes.h include/Zusi3TCPData.h )


set ( CLI_SOURCES Zusi3TCPCli.cpp )
set ( CLI_HEADERS )


find_package( Boost REQUIRED system date_time regex)
find_package( Threads REQUIRED )


add_library(zusi3tcp  ${SOURCES} ${HEADERS} )
target_include_directories(zusi3tcp PUBLIC ${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
target_link_libraries(zusi3tcp PUBLIC ${CMAKE_THREAD_LIBS_INIT} Boost::system Boost::date_time Boost::regex)

add_executable(zusi3cli ${CLI_SOURCES} ${CLI_HEADES} )
target_link_libraries(zusi3cli zusi3tcp )

target_include_directories(zusi3tcp PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
	PRIVATE src)

target_include_directories(zusi3cli PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
	PRIVATE src)

install(TARGETS zusi3tcp EXPORT zusi3tcpConfig
	ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT zusi3tcpConfig DESTINATION share/zusi3tcp/cmake)

export(TARGETS zusi3tcp FILE zusi3tcpConfig.cmake)

