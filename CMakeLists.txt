cmake_minimum_required(VERSION 3.13)
project(jlu-drcom)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
pkg_check_modules(GTKMM gtkmm-3.0)
pkg_check_modules(OpenSSL openssl)

link_directories(
        ${GTKMM_LIBRARY_DIRS}
        ${OpenSSL_LIBRARY_DIRS}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${GTKMM_INCLUDE_DIRS}
        ${OpenSSL_INCLUDE_DIRS}
        .
)

add_subdirectory(drcomclient)
aux_source_directory(. SRCS)
add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(
        ${PROJECT_NAME}
        ${GTKMM_LIBRARIES}
        ${OpenSSL_LIBRARIES}
        Boost::boost
        spdlog::spdlog_header_only
        nlohmann_json::nlohmann_json
        drcomclient
)