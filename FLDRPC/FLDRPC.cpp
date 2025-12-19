#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <fstream>
#include "./external/json/json.hpp"
#include <winver.h>
#include <unordered_map>

#pragma comment(lib, "Version.lib")
#include "./external/discord_rpc/include/discord_rpc.h"
#pragma comment(lib, "discord-rpc.lib")

using json = nlohmann::json;

// ---------------------- Config ----------------------
struct Config {
    std::string discordAppId;
    std::string largeImageKey;
    int refreshInterval = 2;
    std::string defaultProjectName = "Untitled";
    std::string playingImageKey = "play";
    std::string stoppedImageKey = "stop";
    std::string presenceDetails;
    std::string presenceState;
};

Config g_config;

bool LoadConfig(const std::string& path, Config& cfg) {
    cfg.discordAppId = "1451215754035855514";
    cfg.largeImageKey = "flstudio";
    cfg.refreshInterval = 2;
    cfg.defaultProjectName = "Untitled";
    cfg.playingImageKey = "play";
    cfg.stoppedImageKey = "stop";
    cfg.presenceDetails = "Project: {Project} | BPM: {BPM} | Pitch: {Pitch} | Metronome: {Metronome} | Mode: {Mode}";
    cfg.presenceState = "Playback: {Status}";

    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        json j;
        file >> j;

        cfg.discordAppId = j.value("discordAppId", cfg.discordAppId);
        cfg.largeImageKey = j.value("largeImageKey", cfg.largeImageKey);
        cfg.refreshInterval = j.value("refreshInterval", cfg.refreshInterval);
        cfg.defaultProjectName = j.value("defaultProjectName", cfg.defaultProjectName);
        cfg.playingImageKey = j.value("playingImageKey", cfg.playingImageKey);
        cfg.stoppedImageKey = j.value("stoppedImageKey", cfg.stoppedImageKey);

        if (j.contains("presence")) {
            cfg.presenceDetails = j["presence"].value("details", cfg.presenceDetails);
            cfg.presenceState = j["presence"].value("state", cfg.presenceState);
        }
    }
    catch (...) {
        std::cout << "JSON format error, using defaults.\n";
    }

    return true;
}

// ---------------------- Memory scanning ----------------------
enum DataType {
    TYPE_FLOAT,
    TYPE_INT,
    TYPE_SHORT,
    TYPE_BYTE,
    TYPE_POINTER_TO_INT,
    TYPE_POINTER_TO_SHORT,
    TYPE_POINTER_TO_BYTE
};

struct Signature {
	const char* pattern;
	int ripOffset;
	int instrLen;
	int postOffset;
	DataType type;
};

struct Pattern {
    std::string name;
	std::vector<Signature> signatures; // take into consideration older and newer versions
};

struct MemHandle {
    HANDLE proc{};
    uintptr_t addr{};

    MemHandle add(ptrdiff_t offset) const { return { proc, addr + offset }; }
    MemHandle sub(ptrdiff_t offset) const { return { proc, addr - offset }; }

    MemHandle rip(int instrLen) const {
        int32_t disp = 0;
        ReadProcessMemory(proc, (LPCVOID)addr, &disp, sizeof(disp), nullptr);
        return { proc, (addr - 2) + instrLen + disp };
    }

    template<typename T>
    T as() const {
        T val{};
        ReadProcessMemory(proc, (LPCVOID)addr, &val, sizeof(T), nullptr);
        return val;
    }

    template<typename T>
    T* as_ptr() const {
        T* val = nullptr;
        ReadProcessMemory(proc, (LPCVOID)addr, &val, sizeof(val), nullptr);
        return val;
    }
};

std::vector<Pattern> PatternList = {
    {
        "Current BPM",
        {
            { "F3 0F 11 05 ? ? ? ? F3 0F 2A 05 ? ? ? ?", 4, 6, 0, TYPE_FLOAT } // looks to be the same for every version
        }
    },
{
        "Pattern/Song Toggle",
        {
            { "48 8B 05 ? ? ? ? 83 38 ? 75 ? 48 8D 8D 90 02 00 00", 3, 6, 0, TYPE_POINTER_TO_INT }, // 24.1.2.4430
            { "48 8B 05 ? ? ? ? 83 38 ? 75 ? E8 ? ? ? ? 48 8D 88 48 1C 02 00", 3, 6, 0, TYPE_POINTER_TO_INT },

            { "48 8B 05 ? ? ? ? 83 38 ? 0F 85 ? ? ? ? B1 ?", 3, 6, 0, TYPE_POINTER_TO_INT }, // 20.6.2.1549
        }
    },
{
        "Master Pitch",
        {
            { "48 8B 05 ? ? ? ? 03 18 48 8B 4D 20", 3, 6, 0, TYPE_POINTER_TO_SHORT },
            { "48 8B 05 ? ? ? ? F3 0F 2A 00 48 8B 45 20", 3, 6, 0, TYPE_POINTER_TO_SHORT },
            { "48 8B 05 ? ? ? ? 8B 8D B8 05 00 00 89 08 33 C0", 3, 6, 0, TYPE_POINTER_TO_SHORT },
            { "48 8B 05 ? ? ? ? 8B 00 89 85 B8 05 00 00 8B 85 C0 05 00 00 A9 ? ? ? ? 74 ? 48 8B 8D 90 03 00 00", 3, 6, 0, TYPE_POINTER_TO_SHORT },

            { "48 8B 05 ? ? ? ? 48 0F B7 00 66 89 85 08 02 00 00", 3, 6, 0, TYPE_POINTER_TO_SHORT }, // 24.2.2.4597
            { "48 8B 05 ? ? ? ? 48 8B 8D 40 0B 00 00 48 8B 89 C0 44 00 00 48 0F BF 89 2C 00 10 00", 3, 6, 0, TYPE_POINTER_TO_SHORT }, // 25.2.2.5154
			{ "48 8B 05 ? ? ? ? 48 0F B7 00 66 89 85 D8 04 00 00", 3, 6, 0, TYPE_POINTER_TO_SHORT }, // 21.0.3.3517
            { "48 8B 05 ? ? ? ? 48 8B 8D 90 09 00 00 48 8B 89 00 04 00 00 48 0F BF 89 2C 00 10 00", 3, 6, 0, TYPE_POINTER_TO_SHORT }, // 21.1.1.3750
            { "48 8B 05 ? ? ? ? 48 8B 8D C0 09 00 00 48 8B 89 20 48 00 00 48 0F BF 89 2C 00 10 00", 3, 6, 0, TYPE_POINTER_TO_SHORT }, // 24.1.2.4430
            { "48 8B 05 ? ? ? ? 48 8B 8D 50 0B 00 00 48 8B 89 50 48 00 00 48 0F BF 89 2C 00 10 00", 3, 6, 0, TYPE_POINTER_TO_SHORT }, // 25.1.1.4879
            { "48 8B 05 ? ? ? ? 48 8B 8D 40 0B 00 00 48 8B 89 40 48 00 00 48 0F BF 89 2C 00 10 00", 3, 6, 0, TYPE_POINTER_TO_SHORT }, // 25.1.5.4976

            { "48 8B 05 ? ? ? ? 48 8B 8D ? ? ? ? 48 8B 89 ? ? ? ? 48 0F BF 89", 3, 6, 0, TYPE_POINTER_TO_SHORT }, // 20.6.2.1549
        }
    },
{
        "Metronome Toggle",
        {
            { "48 8B 05 ? ? ? ? 48 0F B6 10 E8 ? ? ? ? 48 8B 05 ? ? ? ? 48 8B 00 48 8B 88 F8 0C 00 00", 3, 6, 0, TYPE_POINTER_TO_BYTE }, // 21.0.3.3517
            { "48 8B 0D ? ? ? ? 48 0F B6 09 88 88 88 09 00 00", 3, 6, 0, TYPE_POINTER_TO_BYTE }, // 24.2.2.4597

            { "48 8B 05 ? ? ? ? ? ? ? 0F 84 ? ? ? ? 48 8B 05 ? ? ? ? ? ? 48 8B 05", 3, 6, 0, TYPE_POINTER_TO_BYTE }, // 20.6.2.1549
        }
    },
{
        "Playing Status",
        {
            { "48 8B 0D ? ? ? ? 83 39 ? 0F 94 C1 85 C0", 3, 6, 0, TYPE_POINTER_TO_INT }, // 21.0.3.3517
            { "83 3D ? ? ? ? ? 74 ? 83 3D ? ? ? ? ? 7F", 2, 7, 0, TYPE_BYTE }, // versions 24 and up

            { "48 8B 05 ? ? ? ? ? ? ? 0F 85 ? ? ? ? 48 8B 05 ? ? ? ? ? ? 48 8B 0D", 3, 6, 0, TYPE_POINTER_TO_INT }, // 20.6.2.1549

        }
    }
};

DWORD GetProcId(const wchar_t* name) {
    PROCESSENTRY32W pe{ sizeof(pe) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (!_wcsicmp(pe.szExeFile, name)) {
                CloseHandle(snap);
                return pe.th32ProcessID;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return 0;
}

uintptr_t GetModuleBase(DWORD pid, const wchar_t* modName, DWORD& modSize) {
    MODULEENTRY32W me{ sizeof(me) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (Module32FirstW(snap, &me)) {
        do {
            if (!_wcsicmp(me.szModule, modName)) {
                modSize = me.modBaseSize;
                CloseHandle(snap);
                return reinterpret_cast<uintptr_t>(me.modBaseAddr);
            }
        } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
    return 0;
}

std::vector<int> PatternToBytes(const char* pattern) {
    std::vector<int> bytes;
    char* start = const_cast<char*>(pattern);
    char* end = start + strlen(pattern);
    for (char* cur = start; cur < end; ++cur) {
        if (*cur == '?') {
            bytes.push_back(-1);
            if (*(cur + 1) == '?') ++cur;
        }
        else if (isxdigit(*cur)) {
            bytes.push_back(strtoul(cur, &cur, 16));
            --cur;
        }
    }
    return bytes;
}

uintptr_t FindPattern(HANDLE proc, uintptr_t base, DWORD size, const char* sig) {
    auto pattern = PatternToBytes(sig);
    std::vector<BYTE> buffer(size);
    if (!ReadProcessMemory(proc, (LPCVOID)base, buffer.data(), size, nullptr)) return 0;

    for (size_t i = 0; i < size - pattern.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < pattern.size(); j++) {
            if (pattern[j] != -1 && buffer[i + j] != pattern[j]) {
                found = false;
                break;
            }
        }
        if (found) return base + i;
    }
    return 0;
}

std::wstring GetProcessProductVersion(DWORD pid) {
    wchar_t path[MAX_PATH];
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return L"Unknown";
    DWORD size = MAX_PATH;
    QueryFullProcessImageNameW(hProc, 0, path, &size);
    CloseHandle(hProc);

    DWORD dummy;
    DWORD verSize = GetFileVersionInfoSizeW(path, &dummy);
    if (!verSize) return L"Unknown";
    std::vector<BYTE> data(verSize);
    GetFileVersionInfoW(path, 0, verSize, data.data());

    VS_FIXEDFILEINFO* info = nullptr;
    UINT len = 0;
    VerQueryValueW(data.data(), L"\\", (LPVOID*)&info, &len);

    return std::to_wstring(HIWORD(info->dwProductVersionMS)) + L"." +
        std::to_wstring(LOWORD(info->dwProductVersionMS)) + L"." +
        std::to_wstring(HIWORD(info->dwProductVersionLS)) + L"." +
        std::to_wstring(LOWORD(info->dwProductVersionLS));
}

std::string GetFLStudioProjectName() {
    HWND hwnd = FindWindow(L"TFruityLoopsMainForm", nullptr);
    if (!hwnd) return g_config.defaultProjectName;

    wchar_t title[512] = {};
    GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));

    std::wstring wtitle(title);
    std::string stitle(wtitle.begin(), wtitle.end());

    size_t pos = stitle.rfind('-');
    if (pos != std::string::npos) {
        std::string proj = stitle.substr(0, pos);
        proj.erase(proj.find_last_not_of(" \t") + 1);
        return proj.empty() ? g_config.defaultProjectName : proj;
    }

    return g_config.defaultProjectName;
}


// ---------------------- Global state ----------------------
std::string g_projectName;
std::string g_playState;
float g_bpm;
int g_masterPitch;
bool g_metronome;
bool g_songMode;
int64_t g_startTimestamp;
std::string g_flVersion;

// ---------------------- Helpers ----------------------
std::string ReplacePlaceholders(const std::string& text) {
    std::string result = text;
    auto replace = [&](const std::string& key, const std::string& value) {
        size_t pos = 0;
        while ((pos = result.find(key, pos)) != std::string::npos) {
            result.replace(pos, key.length(), value);
            pos += value.length();
        }
        };

    replace("{Project}", g_projectName);
    replace("{BPM}", std::to_string((int)g_bpm));
    replace("{Pitch}", std::to_string(g_masterPitch));
    replace("{Metronome}", g_metronome ? "ON" : "OFF");
    replace("{Mode}", g_songMode ? "Song" : "Pattern");
    replace("{Status}", g_playState);

    return result;
}

// ---------------------- Discord RPC ----------------------
void InitDiscordRPC() {
    DiscordEventHandlers handlers{};
    Discord_Initialize(g_config.discordAppId.c_str(), &handlers, 1, nullptr);
    g_startTimestamp = time(nullptr);
}

void ShutdownDiscordRPC() {
    Discord_Shutdown();
}

void UpdateDiscordRPC() {
    std::string detailsStr = ReplacePlaceholders(g_config.presenceDetails);
    std::string stateStr = ReplacePlaceholders(g_config.presenceState);

    DiscordRichPresence rpc{};
    rpc.startTimestamp = g_startTimestamp;
    rpc.largeImageKey = g_config.largeImageKey.c_str();
    rpc.largeImageText = g_flVersion.c_str();
    rpc.details = detailsStr.c_str();
    rpc.state = stateStr.c_str();
    rpc.smallImageKey = (g_playState == "Playing") ? g_config.playingImageKey.c_str() : g_config.stoppedImageKey.c_str();

    Discord_UpdatePresence(&rpc);
    std::cout << "[Discord Update] " << rpc.details << " | " << rpc.state << std::endl;
}

// ---------------------- Main ----------------------
int main() {
    if (!LoadConfig("config.json", g_config))
        std::cout << "Failed to load config.json, using defaults.\n";

    const wchar_t* PROCESS_NAME = L"FL64.exe";
    const wchar_t* MODULE_NAME = L"FLEngine_x64.dll";

    DWORD pid = GetProcId(PROCESS_NAME);
    if (!pid) {
        std::cout << "Error: FL Studio (FL64.exe) not found. Please open FL Studio first.\n";
        system("pause");
        return 1;
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        std::cout << "Error: Could not open process. Try running as Administrator.\n";
        system("pause");
        return 1;
    }
    DWORD modSize = 0;
    uintptr_t modBase = GetModuleBase(pid, MODULE_NAME, modSize);
    if (!modBase) {
        std::cout << "Error: Could not find FLEngine_x64.dll.\n";
        system("pause");
        return 1;
    }

    std::wcout << L"--- Target: " << PROCESS_NAME
        << L" (Version: " << GetProcessProductVersion(pid) << L") ---\n\n";

    g_projectName = GetFLStudioProjectName();
    std::wstring wVer = GetProcessProductVersion(pid);
    std::string version(wVer.begin(), wVer.end());
    g_flVersion = "FL Studio v" + version;
    InitDiscordRPC();

    std::unordered_map<std::string, bool> printedPatterns;
    bool initialCheckDone = false;

    while (true) {
        for (const auto& p : PatternList) {
            uintptr_t hit = 0;
            const Signature* matchedSig = nullptr;

            for (const auto& sig : p.signatures) {
                hit = FindPattern(hProc, modBase, modSize, sig.pattern);
                if (hit) {
                    matchedSig = &sig;
                    break;
                }
            }
            if (!hit || !matchedSig) continue;

            MemHandle ptr{ hProc, hit };
            MemHandle resolved = ptr.add(matchedSig->ripOffset).rip(matchedSig->instrLen).add(matchedSig->postOffset);

            if (!printedPatterns[p.name]) {
                std::cout << "[Memory] Pattern: " << p.name
                    << " | Address: 0x" << std::hex << resolved.addr
                    << " | Module+Offset: 0x" << (resolved.addr - modBase) << std::dec << "\n";
                printedPatterns[p.name] = true;
            }

            if (p.name == "Current BPM") {
                g_bpm = resolved.as<float>();
            }
            else if (p.name == "Pattern/Song Toggle") {
                int val = 0;
                if (matchedSig->type == TYPE_POINTER_TO_INT) {
                    auto ptrVal = resolved.as_ptr<int>();
                    if (ptrVal) ReadProcessMemory(hProc, ptrVal, &val, sizeof(val), nullptr);
                }
                g_songMode = (val == 1);
            }
            else if (p.name == "Master Pitch") {
                int16_t raw = 0;
                if (matchedSig->type == TYPE_POINTER_TO_SHORT) {
                    auto ptrVal = resolved.as_ptr<int16_t>();
                    if (ptrVal) ReadProcessMemory(hProc, ptrVal, &raw, sizeof(raw), nullptr);
                }
                g_masterPitch = static_cast<int>(raw);
            }
            else if (p.name == "Metronome Toggle") {
                unsigned char val = 0;
                if (matchedSig->type == TYPE_POINTER_TO_BYTE) {
                    auto ptrVal = resolved.as_ptr<unsigned char>();
                    if (ptrVal) ReadProcessMemory(hProc, ptrVal, &val, sizeof(val), nullptr);
                }
                g_metronome = (val != 0);
            }
            else if (p.name == "Playing Status") {
                uint8_t val = 0;
                if (matchedSig->type == TYPE_BYTE) {
                    val = resolved.as<uint8_t>();
                }
                else if (matchedSig->type == TYPE_POINTER_TO_INT) {
                    int* ptrVal = resolved.as_ptr<int>();
                    if (ptrVal) ReadProcessMemory(hProc, ptrVal, &val, sizeof(val), nullptr);
                }
                g_playState = (val == 1) ? "Playing" : "Stopped";
            }
        }

        if (!initialCheckDone) {
            for (const auto& p : PatternList) {
                if (printedPatterns.find(p.name) == printedPatterns.end()) {
                    std::cout << "WARNING: Pattern '" << p.name << "' was not found. Some features may not work.\n";
                }
            }
            initialCheckDone = true;
        }

        g_projectName = GetFLStudioProjectName();

        UpdateDiscordRPC();
        Discord_RunCallbacks();

        std::this_thread::sleep_for(std::chrono::seconds(g_config.refreshInterval));

        if (GetProcId(PROCESS_NAME) == 0) {
            break;
        }
    }

    ShutdownDiscordRPC();
    return 0;
}
