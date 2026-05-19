cmake_minimum_required(VERSION 3.10.0)
project(translucent VERSION 0.1.0 LANGUAGES C)

subdirs(external resources)

add_executable(translucent-sdl3 main.c)

target_link_libraries(
    translucent-sdl3
    translucent-resources
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf)