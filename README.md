# FL Studio RPC Tracker

A tool for tracking FL Studio project playback status, BPM, pitch, and other metrics in real-time.

## How It Works

The tracker works by reading FL Studio's memory in real-time to extract information about the current project:

- **Memory Scanning:** Scans FL Studio's engine module (`FLEngine_x64.dll`) for known patterns to locate BPM, pitch, metronome state, playback status, and song/pattern mode.  
- **Value Reading:** Reads values directly from FL Studio's process continuously.  
- **Discord RPC Update:** Updates your Discord profile with the current project details according to the template in `config.json`.  
- **Customizable Display:** Use placeholders (`{Project}`, `{BPM}`, `{Pitch}`, `{Metronome}`, `{Mode}`, `{Status}`) for dynamic, real-time presence information.  

## Supported Versions

This tool utilizes **AOB (Array of Bytes) scanning**, allowing it to dynamically locate memory addresses even after minor software updates. It has been officially tested and verified on the following versions of FL Studio:

| FL Studio Version | Build Number | Status |
| :--- | :--- | :--- |
| **25.2.3** | 5171 | ✅ Confirmed |
| **25.2.2** | 5154 | ✅ Confirmed |
| **25.1.6** | 4997 | ✅ Confirmed |
| **25.1.5** | 4976 | ✅ Confirmed |
| **25.1.4** | 4951 | ✅ Confirmed |
| **25.1.1** | 4879 | ✅ Confirmed |
| **24.2.2** | 4597 | ✅ Confirmed |
| **24.2.1** | 4526 | ✅ Confirmed |
| **24.2.0** | 4503 | ✅ Confirmed |
| **24.1.2** | 4430 | ✅ Confirmed |
| **21.2.3** | 4004 | ✅ Confirmed |
| **21.1.1** | 3750 | ✅ Confirmed |
| **21.0.3** | 3517 | ✅ Confirmed |
| **20.9.2** | 2963 | ✅ Confirmed |
| **20.8.4** | 2576 | ✅ Confirmed |
| **20.6.2** | 1549 | ✅ Confirmed |

> [!IMPORTANT]
> While the tool is designed to be version-independent, internal memory structures can change. If you encounter issues or incorrect data with a specific build, please **[create an issue](https://github.com/nuiiv/FL-Studio-Discord-RPC/issues)** in this repository.

## Configuration

All settings are stored in `config.json` in the project folder. Example:

```json
{
  "discordAppId": "YOUR_APP_ID",
  "largeImageKey": "flstudio",
  "refreshInterval": 2,
  "defaultProjectName": "Untitled",
  "playingImageKey": "play",
  "stoppedImageKey": "stop",
  "presence": {
    "details": "{Project} | BPM: {BPM} | Pitch: {Pitch} | Metronome: {Metronome} | Mode: {Mode}",
    "state": "Playback: {Status}"
  }
}
```

## Alternative Automatic RPC (DLL Proxy Method)

If you prefer not to run the tracker separately, you can enable Discord RPC automatically using the **`version.dll` proxy**:

1. **Download** the `version.dll` proxy file from the [releases page](https://github.com/nuiiv/FL-Studio-Discord-RPC/releases).  
2. **Copy** the `version.dll` file into the **same folder as `fl64.exe`** (FL Studio's main executable).  
3. **Launch FL Studio** normally. Discord RPC will automatically start and display your project details in real-time.

> [!IMPORTANT]
>
> This tool **reads memory from a running FL Studio process**.  
> Use at your own risk. The author is **not responsible for any crashes, data loss, or other issues** caused by using this tracker.

## Acknowledgements

[alessandromrc](https://github.com/alessandromrc) - Helped with RE fundamentals and discovered several memory patterns.

## Third-Party Libraries
This project is made possible by the following open-source libraries:

| Library | License | Usage |
| :--- | :--- | :--- |
| [nlohmann/json](https://github.com/nlohmann/json) | MIT | Configuration file parsing |
| [Discord-RPC](https://github.com/discord/discord-rpc) | MIT | Communication with Discord client |
| [MinHook](https://github.com/TsudaKageyu/minhook) | BSD-2-Clause | API Hooking for log interception |
