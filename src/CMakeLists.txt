# Find the OpenCV package
find_package(OpenCV REQUIRED)
find_package(SDL2 REQUIRED)

add_executable(nesquick utils.cpp lstdebugger.cpp ppu.cpp cpu.cpp cpumem.cpp audio.cpp main.cpp)

target_link_libraries(nesquick ${OpenCV_LIBS} SDL2::SDL2)

# Include the OpenCV headers
target_include_directories(nesquick PRIVATE ${OpenCV_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
