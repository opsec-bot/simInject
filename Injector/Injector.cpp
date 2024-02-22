#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include "Helpers.hpp"
#include <filesystem>
#include <cstdlib>
#include <Shlobj.h>

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

namespace fs = std::filesystem;

const char *link = "https://discord.gg/T2dsd59B3H"; // discord invite here
const char *tempFileName = "fortniteLOL";           // change this on new discord invite

bool isFirstLaunch()
{
    return !fs::exists(fs::temp_directory_path() / tempFileName);
}

bool isAdministrator()
{
    return IsUserAnAdmin() != 0;
}

void createLaunchMarker()
{
    std::ofstream tempFile((fs::temp_directory_path() / tempFileName).string());
    tempFile.close();
}

bool injectDLL(DWORD processID, const std::string &dllPath)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProc == INVALID_HANDLE_VALUE)
    {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cout << "Couldn't create a handle to the process.\n"
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    LPVOID strAddy = VirtualAllocEx(hProc, NULL, dllPath.size() + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (strAddy == NULL)
    {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cout << "Couldn't call VirtualAllocEx." << std::endl;
        CloseHandle(hProc);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    if (!WriteProcessMemory(hProc, strAddy, dllPath.c_str(), dllPath.size() + 1, NULL))
    {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cout << "Couldn't call WriteProcessMemory." << std::endl;
        CloseHandle(hProc);
        VirtualFreeEx(hProc, strAddy, 0, MEM_RELEASE);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    LPVOID loadLibraryA = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

    HANDLE hThread = CreateRemoteThread(hProc, NULL, NULL, (LPTHREAD_START_ROUTINE)loadLibraryA, strAddy, NULL, NULL);
    if (hThread == INVALID_HANDLE_VALUE)
    {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cout << "Couldn't call CreateRemoteThread." << std::endl;
        CloseHandle(hProc);
        VirtualFreeEx(hProc, strAddy, 0, MEM_RELEASE);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    CloseHandle(hProc);
    VirtualFreeEx(hProc, strAddy, 0, MEM_RELEASE);

    return true;
}

int main()
{
    SetConsoleTitle("Injector x64 | By @opensec on Discord");

    fs::path executablePath = fs::canonical(fs::path(__FILE__));
    fs::path executableDirectory = executablePath.parent_path();

    consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!isAdministrator())
    {
        SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
        std::cerr << "Please re-launch with administrator permissions" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 1;
    }

    if (isFirstLaunch())
    {
        std::system(("start " + std::string(link)).c_str());
        createLaunchMarker();
    }

    SetConsoleColor(ConsoleColors::BlueFore | ConsoleColors::SilverFore);
    std::cout << "Searching for process..." << std::endl;

    DWORD rustPID = -1;
    do
    {
        rustPID = GetPID("RustClient.exe");
        Sleep(1000);
    } while (rustPID == -1);

    SetConsoleColor(ConsoleColors::BlueFore | ConsoleColors::SilverFore);

    std::string dllPath;
    for (const auto &entry : fs::directory_iterator(executableDirectory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".dll")
        {
            if (dllPath.empty() || entry.last_write_time() > fs::last_write_time(dllPath))
            {
                dllPath = entry.path().string();
            }
        }
    }

    if (!dllPath.empty())
    {
        system("cls");
        std::cout << "DLL found: " << dllPath << std::endl;

        if (injectDLL(rustPID, dllPath))
        {
            system("cls");
            SetConsoleColor(ConsoleColors::GreenFore | ConsoleColors::SilverFore);
            std::cout << "DLL successfully injected!" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        else
        {
            system("cls");
            SetConsoleColor(ConsoleColors::RedFore | ConsoleColors::SilverFore);
            std::cout << "Failed to inject DLL." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    else
    {
        std::cout << "No DLLs found in: " << executableDirectory << std::endl;
    }
    return 0;
}
