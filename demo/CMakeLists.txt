cmake_minimum_required(VERSION 2.8)

#link_libraries(demo wcs)
#link_libraries(multipart wcs)
link_libraries(wcsTest wcs)

#添加编译选项
add_definitions (-g)

#add_executable(demo demo.c)
#add_executable(multipart multipart.c)

add_executable(wcsTest test.c)
