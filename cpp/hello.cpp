// Portable hello world + process list: builds with MSVC and MinGW (and g++/clang++ on Linux).
//   MSVC:   cl /EHsc /W4 /std:c++17 hello.cpp
//   MinGW:  g++ -std=c++17 -Wall -Wextra -o hello.exe hello.cpp
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#else
#include <filesystem>
#include <fstream>
#endif

struct ProcessInfo { unsigned long pid; std::string name; };

// Standard C++ has no process API, so each platform uses its native one.
std::vector<ProcessInfo> listProcesses()
{
    std::vector<ProcessInfo> result;
#ifdef _WIN32
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return result;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    for (BOOL ok = Process32FirstW(snap, &entry); ok; ok = Process32NextW(snap, &entry)) {
        char name[MAX_PATH * 3] = {};  // UTF-16 -> UTF-8 so output works regardless of UNICODE setting
        WideCharToMultiByte(CP_UTF8, 0, entry.szExeFile, -1, name, sizeof(name), nullptr, nullptr);
        result.push_back({entry.th32ProcessID, name});
    }
    CloseHandle(snap);
#else
    std::error_code ec;
    for (const auto& dir : std::filesystem::directory_iterator("/proc", ec)) {
        const std::string pid = dir.path().filename().string();
        if (pid.find_first_not_of("0123456789") != std::string::npos) continue;
        std::string name;
        std::ifstream(dir.path() / "comm") >> name;  // process may exit mid-scan; name stays empty
        result.push_back({std::stoul(pid), name});
    }
#endif
    return result;
}

int main()
{
    std::cout << "Hello, world!\n\nRunning processes:\n";
    for (const auto& p : listProcesses())
        std::cout << "  " << p.pid << "\t" << p.name << "\n";
    return 0;
}
