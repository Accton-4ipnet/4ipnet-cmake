cmake_minimum_required (VERSION 2.6)
link_libraries(curl)
project(4ipnet-cmake)
aux_source_directory(./src src)
add_executable(4ipnet-cmake ${src})

install(TARGETS 4ipnet-cmake
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static
        PUBLIC_HEADER DESTINATION include
        )
