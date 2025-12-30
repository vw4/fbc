cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_ASM_COMPILER avr-gcc)

set(CMAKE_AR avr-ar)
set(CMAKE_OBJCOPY avr-objcopy)
set(CMAKE_OBJDUMP avr-objdump)
set(CMAKE_SIZE avr-size)

# Prevent host include/library lookup
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

project(fbc C)

set(MCU attiny25)
set(F_CPU 1000000UL)

if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
endif()


add_executable(${PROJECT_NAME}.elf
        src/main.c
        src/io.c
        src/io.h
        src/gamma.c
        src/gamma.h
        src/channel.c
        src/channel.h
        src/events.c
        src/events.h
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE F_CPU=${F_CPU})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${PROJECT_NAME}.elf PRIVATE DEBUG=1 DBG_ENABLED=1)
else()
        target_compile_definitions(${PROJECT_NAME}.elf PRIVATE DEBUG=0 DBG_ENABLED=0)
endif()

target_compile_options(${PROJECT_NAME}.elf PRIVATE
        -mmcu=${MCU}
        -Os
        -ffunction-sections -fdata-sections
        -Wall -Wextra
        -gdwarf-2
)
target_link_options(${PROJECT_NAME}.elf PRIVATE
        -mmcu=${MCU}
        -Wl,--gc-sections
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND avr-objcopy ${PROJECT_NAME}.elf -O ihex ${PROJECT_NAME}.hex -R .eeprom
        COMMAND avr-objcopy ${PROJECT_NAME}.elf -O binary ${PROJECT_NAME}.bin -R .eeprom
        COMMAND avr-size --format=berkeley --target=ihex ${PROJECT_NAME}.elf
        BYPRODUCTS ${PROJECT_NAME}.hex ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_SIZE} --format=berkeley --target=ihex --totals $<TARGET_FILE:${PROJECT_NAME}.elf>
)
