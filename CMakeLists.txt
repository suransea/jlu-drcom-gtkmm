cmake_minimum_required(VERSION 3.13)
project(jlu-drcom)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig)
find_package(Boost)
pkg_check_modules(GTKMM gtkmm-3.0)
link_directories(${GTKMM_LIBRARY_DIRS})
include_directories(
        ${GTKMM_INCLUDE_DIRS}
        ${BOOST_INCLUDE_DIRS}
        include
)

add_subdirectory(drcomclient)
aux_source_directory(. DIRSRCS)

add_executable(${PROJECT_NAME} ${DIRSRCS})
target_link_libraries(
        ${PROJECT_NAME}
        ${GTKMM_LIBRARIES}
        ${BOOST_LIBRARIES}
        boost_system
        pthread
        crypto
        drcomclient
)