cmake_minimum_required(VERSION 3.10)

project(Entrega2_OpenGL)

set(CMAKE_CXX_STANDARD 14) 

include_directories(include)
find_package(OpenGL REQUIRED)

set(CODIGO src/glad.c
    src/Camera.cpp     
    src/GameLogic.cpp          
    src/Lighting.cpp
    src/Mesh.cpp
    src/Room.cpp
    src/RoomVariant.cpp)

add_executable(exit8 src/main.cpp ${CODIGO})

# 3. Enlazado multiplataforma
if(WIN32)
    # Configuración para Windows (Visual Studio)
    target_link_libraries(exit8 OpenGL::GL glfw3)
else()
    # Configuración para Linux/macOS
    target_link_libraries(exit8 OpenGL::GL glfw dl)
endif()