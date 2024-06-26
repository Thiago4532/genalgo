project(glad C)

add_library(glad STATIC src/glad.c)
target_include_directories(glad PUBLIC include/)

# vim: et ts=8 sts=2 sw=2
