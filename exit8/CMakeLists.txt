cmake_minimum_required(VERSION 3.10)

# 1. Ponle un nombre a tu proyecto
project(Entrega2_OpenGL)

# Actualizado a 14 para evitar tu error de compilación con las luces
set(CMAKE_CXX_STANDARD 14) 

include_directories(include)
find_package(OpenGL REQUIRED)

# 2. Tu ejecutable y archivos fuente
add_executable(exit8 src/Camera.cpp src/main.cpp src/glad.c)

# 3. Enlazado multiplataforma
if(WIN32)
    # Configuración para Windows (Visual Studio)
    target_link_libraries(exit8 OpenGL::GL glfw3)
else()
    # Configuración para Linux/macOS
    target_link_libraries(exit8 OpenGL::GL glfw dl)
endif()