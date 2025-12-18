# FL Studio RPC Tracker

A tool for tracking FL Studio project playback status, BPM, pitch, and other metrics in real-time.

## How It Works

The tracker works by reading FL Studio's memory in real-time to extract information about the current project:

- **Memory Scanning:** Scans FL Studio's engine module (`FLEngine_x64.dll`) for known patterns to locate BPM, pitch, metronome state, playback status, and song/pattern mode.  
- **Value Reading:** Reads values directly from FL Studio's process continuously.  
- **Discord RPC Update:** Updates your Discord profile with the current project details according to the template in `config.json`.  
- **Customizable Display:** Use placeholders (`{Project}`, `{BPM}`, `{Pitch}`, `{Metronome}`, `{Mode}`, `{Status}`) for dynamic, real-time presence information.  

## Supported Version

This tool has been **tested on FL Studio version 24.2.2 build 4597 and version 24.2.1 build 4526 (Should still work on similar versions due to AOB scanning)**.  
It may **not work properly on other versions**. If you encounter issues with other versions, feel free to **create an issue** in this repository.

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