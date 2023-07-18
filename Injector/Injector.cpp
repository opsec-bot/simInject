#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include "Helpers.hpp"

HANDLE consoleHandle;
#define SetConsoleColor(col) SetConsoleTextAttribute(consoleHandle, col);

enum ConsoleColors
{
    BlackFore = 0,
    MaroonFore = FOREGROUND_RED,
    GreenFore = FOREGROUND_GREEN,
    NavyFore = FOREGROUND_BLUE,
    TealFore = FOREGROUND_GREEN | FOREGROUND_BLUE,
    OliveFore = FOREGROUND_RED | FOREGROUND_GREEN,
    PurpleFore = FOREGROUND_RED | FOREGROUND_BLUE,
    GrayFore = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    SilverFore = FOREGROUND_INTENSITY,
    RedFore = FOREGROUND_INTENSITY | FOREGROUND_RED,
    LimeFore = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    BlueFore = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
    AquaFore = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
    YellowFore = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    FuchsiaFore = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
    WhiteFore = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
};

int main()
{
    SetConsoleTitle("simHook injector x64 | @simsec");
    consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Waiting for game start
    SetConsoleColor(ConsoleColors::BlueFore | ConsoleColors::SilverFore);
    std::cout << "[simInject] Searching for process..." << std::endl;
    DWORD gmodPID = -1;
    do {
        gmodPID = GetPID("gmod.exe");
        Sleep(1000);
    } while (gmodPID == -1);

    // Process found
    SetConsoleColor(ConsoleColors::BlueFore | ConsoleColors::SilverFore);
    std::cout << "[simInject] Process found!" << std::endl;

    // Ask for DLL path
    std::string dllPath;
    std::cout << "[simInject] Drag & drop .dll here: ";
    std::getline(std::cin, dllPath);

    // Check if the DLL file exists and can be opened
    std::fstream fileStream(dllPath);
    if (!fileStream.is_open())
    {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cout << "[simInject] The specified DLL file could not be opened." << std::endl;
        getchar();
        return 1;
    }
    fileStream.close();

    // Convert the string to a C-style char array
    const char* path = dllPath.c_str();

    // Open the target process
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, gmodPID);
    if (hProc == INVALID_HANDLE_VALUE)
    {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cout << "[simInject] Couldn't create a handle to Garry's Mod.\n" << std::endl;
        getchar();
        return 1;
    }

    // Allocate memory in the target process for the DLL path
    LPVOID strAddy = VirtualAllocEx(hProc, NULL, strlen(path), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (strAddy == NULL) {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cout << "[simInject] Couldn't call VirtualAllocEx." << std::endl;
        getchar();
        CloseHandle(hProc);
        return 1;
    }

    // Write the DLL path to the allocated memory in the target process
    if (!WriteProcessMemory(hProc, strAddy, path, strlen(path), NULL))
    {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cout << "[simInject] Couldn't call WriteProcessMemory." << std::endl;
        getchar();
        CloseHandle(hProc);
        VirtualFreeEx(hProc, strAddy, 0, MEM_RELEASE);
        return 1;
    }

    // Get the address of the LoadLibraryA function in kernel32.dll
    LPVOID loadLibraryA = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

    // Create a remote thread in the target process to call LoadLibraryA with the DLL path
    HANDLE hThread = CreateRemoteThread(hProc, NULL, NULL, (LPTHREAD_START_ROUTINE)loadLibraryA, strAddy, NULL, NULL);
    if (hThread == INVALID_HANDLE_VALUE)
    {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cout << "[simInject] Couldn't call CreateRemoteThread." << std::endl;
        getchar();
        CloseHandle(hProc);
        VirtualFreeEx(hProc, strAddy, 0, MEM_RELEASE);
        return 1;
    }

    // Wait for the remote thread to finish
    WaitForSingleObject(hThread, INFINITE);

    // Cleanup
    CloseHandle(hThread);
    CloseHandle(hProc);
    VirtualFreeEx(hProc, strAddy, 0, MEM_RELEASE);

    // Injection successful
    SetConsoleColor(ConsoleColors::GreenFore | ConsoleColors::SilverFore);
    std::cout << "[simInject] Successfully injected into task!" << std::endl;
    getchar();
    return 0;
}
