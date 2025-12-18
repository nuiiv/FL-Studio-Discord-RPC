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
| **24.2.1** | 4526 | ✅ Confirmed |
| **24.2.2** | 4597 | ✅ Confirmed |
| **25.2.2** | 5154 | ✅ Confirmed |

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

> [!IMPORTANT]
>
> This tool **reads memory from a running FL Studio process**.  
> Use at your own risk. The author is **not responsible for any crashes, data loss, or other issues** caused by using this tracker.