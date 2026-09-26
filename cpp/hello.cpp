// Portable hello world + process list (pid, parent pid, threads, name): builds with MSVC and MinGW (and g++/clang++ on Linux).
//   MSVC:   cl /EHsc /W4 /std:c++17 hello.cpp
//   MinGW:  g++ -std=c++17 -Wall -Wextra -o hello.exe hello.cpp
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#else
#include <filesystem>
#include <sstream>
#endif

struct ProcessInfo { unsigned long pid, ppid, threads; std::string name; };

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
        result.push_back({entry.th32ProcessID, entry.th32ParentProcessID, entry.cntThreads, name});
    }
    CloseHandle(snap);
#else
    std::error_code ec;
    for (const auto& dir : std::filesystem::directory_iterator("/proc", ec)) {
        const std::string pid = dir.path().filename().string();
        if (pid.find_first_not_of("0123456789") != std::string::npos) continue;
        // /proc/<pid>/stat: "pid (name) state ppid ... num_threads(field 20) ..."
        // The name may contain spaces or ')', so split on the *last* ')'.
        std::ifstream file(dir.path() / "stat");
        std::string stat((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        const auto open = stat.find('('), close = stat.rfind(')');
        if (open == std::string::npos || close == std::string::npos) continue;  // process exited mid-scan
        std::istringstream rest(stat.substr(close + 1));
        std::string state, skip;
        ProcessInfo info{std::stoul(pid), 0, 0, stat.substr(open + 1, close - open - 1)};
        rest >> state >> info.ppid;
        for (int field = 5; field < 20; ++field) rest >> skip;
        rest >> info.threads;
        result.push_back(info);
    }
#endif
    return result;
}

// Read a whole file into a byte buffer. Standard C++ only, so it behaves
// identically under MSVC, MinGW and g++/clang++. Opened in binary mode so no
// newline translation happens; the buffer holds the file's exact bytes.
std::vector<std::byte> readFileBytes(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};  // missing/unreadable file -> empty buffer

    const std::streamsize size = file.tellg();
    if (size <= 0) return {};  // empty file, or tellg() failed
    file.seekg(0);

    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(bytes.data()), size);
    bytes.resize(static_cast<std::size_t>(file.gcount()));  // trust bytes actually read
    return bytes;
}

int main(int argc, char** argv)
{
    std::cout << "Hello, world!\n\nRunning processes:\n  PID\tPPID\tTHREADS\tNAME\n";
    for (const auto& p : listProcesses())
        std::cout << "  " << p.pid << "\t" << p.ppid << "\t" << p.threads << "\t" << p.name << "\n";

    if (argc > 1) {
        const auto bytes = readFileBytes(argv[1]);
        std::cout << "\nRead " << bytes.size() << " bytes from " << argv[1] << "\n";
    }
    return 0;
}
