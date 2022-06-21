cmake_minimum_required(VERSION 3.22)

#Initiating Project
project(brainfuck)


#Setting C++ Standard
set(CMAKE_CXX_STANDARD 20)

#Adding the files
add_executable(${PROJECT_NAME} "./src/main.cpp")