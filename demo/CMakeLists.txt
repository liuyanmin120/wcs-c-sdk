cmake_minimum_required(VERSION 3.4)


#link_libraries(demo wcs)
#link_libraries(multipart wcs)
#link_libraries(wcsTest wcs)

link_directories(${LOCAL_PACKAGE_PATH}/build32/Debug)
MESSAGE(STATUS "lib path:" ${LOCAL_PACKAGE_PATH}/build32/Debug)

link_libraries(libcurl.lib
				pthreadVC2.lib
				libeay32.lib
				ssleay32.lib 
				wcs.lib)

#添加编译选项
add_definitions (-g)

#add_executable(demo demo.c)
#add_executable(multipart multipart.c)

add_executable(wcsTest test.c)
