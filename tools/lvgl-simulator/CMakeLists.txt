cmake_minimum_required(VERSION 3.18)
project(motonav_lvgl_simulator LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
find_package(SDL2 REQUIRED)

# Match firmware/build.py's LVGL 8.3.10 exactly. Pin the version on purpose.
FetchContent_Declare(lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG v8.3.10
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lvgl)

add_executable(motonav-lvgl-simulator main.cpp)
target_include_directories(motonav-lvgl-simulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE=1)
target_link_libraries(motonav-lvgl-simulator PRIVATE lvgl SDL2::SDL2)

if(APPLE)
  target_compile_definitions(motonav-lvgl-simulator PRIVATE SDL_MAIN_HANDLED)
endif()

enable_testing()
add_test(NAME lvgl_simulator_smoke COMMAND motonav-lvgl-simulator --smoke-test)
