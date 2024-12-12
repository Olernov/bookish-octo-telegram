project(chat)
cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++17 -Wall")

find_package(Boost 1.66 COMPONENTS system REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries (${PROJECT_NAME}
    ${Boost_SYSTEM_LIBRARY} ${Boost_ASIO_LIBRARY}
    )

install (
        FILES "${PROJECT_NAME}"
        DESTINATION "/usr/bin"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                    GROUP_EXECUTE GROUP_READ
                    WORLD_EXECUTE WORLD_READ
)
