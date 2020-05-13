cmake_minimum_required(VERSION 3.16)
project(HW1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(HW1
        BuiltinCommands.cpp
        BuiltinCommands.h
        Commands.cpp
        Commands.h
        Includes.h
        signals.cpp
        signals.h
        smash.cpp
        ExternalCommands.cpp ExternalCommands.h Smash.h main.cpp SpecialCommands.cpp SpecialCommands.h TimeOutCommand.cpp TimeOutCommand.h)
