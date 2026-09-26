// Portable hello world: builds with MSVC and MinGW (and any standard C++ compiler).
//   MSVC:   cl /EHsc /W4 hello.cpp
//   MinGW:  g++ -std=c++17 -Wall -Wextra -o hello.exe hello.cpp
#include <iostream>

int main()
{
    std::cout << "Hello, world!\n";
    return 0;
}
