# The mmClock (artz fork)

This fork fixes the clock freezing at the top of the hour, corrects the pin definitions to match this build's wiring, and stops the clock running an hour fast during daylight saving time. The changes were tested on the clock in October 2026. The original project README follows below.

## Summary of changes

| Area | File | Change |
|---|---|---|
| Freeze (root cause) | `Clock/DFRobotDFPlayerMini.cpp` | `sendStack()` can no longer wait forever for an ACK |
| Freeze | `Clock/ClockMP3.cpp` | `Play()` drains unread DFPlayer replies; `WaitForIt()` and `PlayTrackAndWait()` time out |
| Freeze | `Clock/Clock.ino` | `connectWiFi()` no longer blocks the whole clock during a Wi-Fi outage |
| Pins | `Clock/common.h` | LED chip select `D9` → `D2`; MP3 serial pins corrected to `D3`/`D4` |
| Display | `Clock/DFRobot_HT1632C.cpp` | Bit-banged writes slowed to within the HT1632C's timing spec |
| Display | `Clock/Clock.ino` | New `refreshDisplayHW()` re-enables the panel every 2 minutes |
| Time | `Clock/common.h` | `TIMEZONE` `-4` → `-5` (DST was being counted twice) |
| Build | `Clock/ClockMP3.h` | `Play()` declared `void` to match its definition, so the sketch compiles |

## 1. The freeze at the top of the hour

### Symptom

The clock ran normally for hours, then froze, usually at the top of the hour. The display stopped, the buttons did nothing and the web page stopped loading. Nothing was printed on serial and the board did not reboot. Only unplugging it brought it back.

### Root cause

Everything in this sketch runs as cooperative `Thread`s inside the single Arduino `loop()`, so any call that never returns stops the whole clock.

At minute 0, `handleAlarm()` calls `Play(1,3)` for the hourly chime. That is the only time the firmware talks to the MP3 module unattended.

In the DFPlayer library, every command starts by waiting for the ACK of the previous one:

```cpp
while (_isSending) { delay(0); waitAvailable(); }
```

The only way out is `waitAvailable()` timing out. But `waitAvailable()` returns immediately, without ever checking its timeout, whenever `available()` reports the `_isAvailable` flag, and that flag is cleared only by `readType()` / `read()`. `Play()` sends a command and never reads anything back. So once the module's "play finished" message had set `_isAvailable` while `_isSending` was still set, the next command spun in that loop forever. Because it calls `delay(0)`, which yields to FreeRTOS, the task watchdog never fired either. No crash, no reboot, just a stopped clock.

### How it was confirmed

- While frozen, the clock still answered ping and accepted TCP connections (the ESP32's network stack runs on its own task), but never served the web page. So it was a blocked `loop()`, not a crash or a Wi-Fi drop.
- With `MP3_DEBUG` enabled, the serial trace stopped at exactly the line this explanation predicts.
- With the fixes in place, the clock ran straight through the top-of-hour chime that used to freeze it.

### Fixes

- **`DFRobotDFPlayerMini::sendStack()`**: the ACK wait now gives up after twice the library's timeout (about 1 second) and clears `_isSending`. This is the actual fix.
- **`Play()`**: drains any unread replies from the module before sending, so the flags can't get stuck in the first place.
- **`WaitForIt()`**: had no timeout and no yield. It now gives up after 5 seconds.
- **`PlayTrackAndWait()`**: both waits on the BUSY pin were endless. It now allows 3 seconds for a track to start and 60 seconds for it to finish.
- **`connectWiFi()`**: this thread runs every 10 seconds and used to block until Wi-Fi came back, freezing the clock for the whole length of any Wi-Fi outage. It now waits at most 5 seconds and tries again on its next run.

## 2. Pin corrections

The pins in `common.h` did not match how this clock is wired:

| Define | Was | Now | Meaning |
|---|---|---|---|
| `FBD_CS` | `D9` | **`D2`** | LED matrix chip select |
| `SERIAL2_RXPIN` | `D3` | **`D4`** | MP3 module's RX (the ESP32 transmits on this pin) |
| `SERIAL2_TXPIN` | `D2` | **`D3`** | MP3 module's TX (the ESP32 listens on this pin) |

The `SERIAL2_*` names are from the **MP3 module's** point of view. `SetupMP3()` calls `Serial2.begin(9600, SERIAL_8N1, SERIAL2_TXPIN, SERIAL2_RXPIN)`, and that function takes the ESP32's *receive* pin first.

With chip select on the wrong pin, the panel never received a single command. The HT1632C keeps showing its last frame until something writes to it, so after flashing firmware with the wrong pin the display looks frozen, and after the next power cycle it is blank.

### Current pin map

| Function | FireBeetle pin | GPIO |
|---|---|---|
| LED matrix DATA | D8 | 5 |
| LED matrix CS | D2 | 25 |
| LED matrix WR | D7 | 13 |
| MP3 module RX (ESP32 → module) | D4 | 27 |
| MP3 module TX (module → ESP32) | D3 | 26 |
| MP3 module BUSY | — | 4 |
| Button 1: speak the time | — | 19 |
| Button 2: change display mode | — | 21 |
| Button 3: change brightness | — | 22 |

The upstream MickMake project uses different pins (LED on `D6`/`D5`/`D7`, MP3 on GPIO 16/17), so don't mix its `common.h` with this one.

## 3. Display reliability

- **Bit timing.** The HT1632C driver toggled its pins with no delays at all, so on a 240 MHz ESP32 each data bit was latched within nanoseconds of being set, well outside the chip's timing spec at 3.3 V. Writes now pause 4 µs around each clock edge and chip-select change (`HT_DELAY_US` at the top of `DFRobot_HT1632C.cpp`). A full screen update takes about 3 ms.
- **Periodic re-enable.** The panel's enable commands were only ever sent once, in `setup()`. `refreshDisplayHW()` now re-sends them every 2 minutes (from `handleNTP()`), so a missed start-up can't leave the panel dark until the next reboot. A brightness of 0 still means "display off".

## 4. Time zone

`getDSTOffset()` already adds an hour during daylight saving time, so `TIMEZONE` must be the *standard-time* offset. It was set to `-4` (Eastern Daylight Time), which counted DST twice and ran the clock an hour fast all summer. It is now `-5`. For other time zones, use the standard-time offset, for example `-8` for US Pacific.

## Building

- **Board package:** DFRobot ESP32, version 0.2.1. In Arduino, select **FireBeetle ESP32** (`DFRobot:esp32:esp32`) with **Flash Mode: DIO** and the default partition scheme. This is the core the clock's original firmware was built with.
- **Libraries:** Time (tested with 1.6.1), ArduinoThread (2.1.1), ArduinoJson 7.x (7.4.3).
- **Before uploading,** fill in `WIFI_SSID` and `WIFI_PASSWD` in `Clock/common.h`. **Don't commit them.** They are blank in this repository on purpose.
- **Debug output:** uncomment the `#define`s in `Clock/Debug.h`. `MP3_DEBUG` is the useful one for anything sound-related.

## Known issues (not changed here)

- **DST start and end dates are wrong.** `isDST()` doesn't actually work out the second Sunday in March or the first Sunday in November. With `TIMEZONE -5` it always switches on **17 March** and **10 November**, so the clock is an hour off for a few days around each change (in 2026, an hour fast from 1 to 9 November).
- **`handleRoot()` prints the whole settings page to serial** on every page load. It's a leftover debug line that pauses the clock for about half a second each time.
- **The hourly chime plays `01/003.mp3`**, but `SDcard/01/` in this repository only contains `001.mp3` and `002.mp3`. Make sure the file exists on your card.
- **Compiler warning:** ArduinoJson 7 reports `containsKey()` in `process()` as deprecated. It's harmless.

---

# The mmClock
Source code for my talking alarm clock. This alarm clock will:
- [x] Sync time from an NTP server.
- [x] Sync events from Google calendar.
- [x] Speak the time/date in many formats.
- [x] Annoy you with the alarm of your choice.

Features I'll be adding:
- [ ] Better security, (currently it's open).
- [ ] Select clock display style from Google calendar.
- [ ] Select MP3 alarm file to play from Google calendar.
- [ ] Select alarm volume level from Google calendar.
- [ ] Multiple calendar syncing.
- [ ] MQTT connectivity.
- [ ] Better web interface.

Check out [my tutorial video](https://www.youtube.com/watch?v=IoX6t03ULnc) on how I made it.
Also check out [my website](https://www.mickmake.com/archives/3375) for further details. 


## Support :+1:
If you want to support me, then head on over to [my Patreon page](http://patreon.com/MickMake).


## License
This source code is covered under the GPL!


## Source code - Clock/
Source code is under the [Clock/](Clock/) directory.
All the important defines are in the [common.h](Clock/common.h) file.
Make sure you update these to the correct settings.

## MP3 files - SDcard/
Copy all the files under [SDcard/](SDcard/) to an SD card. You can create your own, but make sure that you keep the same order as define in [ClockMP3defs.h](Clock/ClockMP3defs.h).

## Other libraries
To use: Download the [FireBeetle Covers-24X8 LED Matrix files](https://github.com/Chocho2017/FireBeetleLEDMatrix), unzip the file and copy the two files:

	DFRobot_HT1632C.cpp
	DFRobot_HT1632C.h

to the [Clock/](Clock/) directory.





