#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# ZLib: the vectors are made with Python's zlib at build time
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ZLib/Vectors.hpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/ZLib/gen-vectors.py
            ${CMAKE_CURRENT_BINARY_DIR}/ZLib/Vectors.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ZLib/gen-vectors.py
    COMMENT "Generating the ZLib test vectors"
)
ctos_kernel_test(ZLibTest
    SOURCES ZLib/ZLibTest.cpp ${CMAKE_CURRENT_BINARY_DIR}/ZLib/Vectors.hpp
    KERNEL Library/ZLib.cpp
)
target_include_directories(ZLibTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/ZLib)
