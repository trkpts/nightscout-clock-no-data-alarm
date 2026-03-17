# Nightscout Clock - No Data Alarm Implementation

This implementation adds an alarm feature that triggers when glucose data hasn't been received for a configurable time period.

## Features

- ✅ Configurable time threshold (5-60 minutes)
- ✅ Enable/disable toggle in web GUI
- ✅ Snooze functionality
- ✅ Silence intervals (night/day/none)
- ✅ Custom RTTTL melodies
- ✅ Automatic alarm clearing when data returns
- ✅ Distinctive alarm sound (different from glucose alarms)
- ✅ Visual "No Data" message when snoozed

## Files Modified/Created

### Core Files to Modify:

1. **src/Settings.h** - Add 5 new alarm settings fields
2. **src/SettingsManager.cpp** - Load/save the new settings
3. **src/BGAlarmManager.cpp** - Handle no-data alarm logic
4. **src/globals.h** - Add default alarm melody

### Optional Files:

5. **data/index.html** - Web UI controls (if you have access)

## Installation Steps

### Step 1: Update Settings.h

Add these 5 fields to the `Settings` class (after the existing alarm settings):

```cpp
// No Data Alarm Settings
bool alarm_no_data_enabled;
int alarm_no_data_minutes;
int alarm_no_data_snooze_minutes;
String alarm_no_data_silence_interval;
String alarm_no_data_melody;
```

### Step 2: Update SettingsManager.cpp

#### In `loadSettingsFromFile()`:

Add after the existing alarm loading code:

```cpp
// No Data Alarm
settings.alarm_no_data_enabled = (*doc)["alarm_no_data_enabled"].as<bool>();
settings.alarm_no_data_minutes = (*doc)["alarm_no_data_minutes"].as<int>();
if (settings.alarm_no_data_minutes < 5) settings.alarm_no_data_minutes = 10;
if (settings.alarm_no_data_minutes > 60) settings.alarm_no_data_minutes = 60;
settings.alarm_no_data_snooze_minutes = (*doc)["alarm_no_data_snooze_interval"].as<int>();
settings.alarm_no_data_silence_interval = (*doc)["alarm_no_data_silence_interval"].as<String>();
settings.alarm_no_data_melody = (*doc)["alarm_no_data_melody"].as<String>();
```

#### In `saveSettingsToFile()`:

Add after the existing alarm saving code:

```cpp
// No Data Alarm
(*doc)["alarm_no_data_enabled"] = settings.alarm_no_data_enabled;
(*doc)["alarm_no_data_minutes"] = settings.alarm_no_data_minutes;
(*doc)["alarm_no_data_snooze_interval"] = settings.alarm_no_data_snooze_minutes;
(*doc)["alarm_no_data_silence_interval"] = settings.alarm_no_data_silence_interval;
(*doc)["alarm_no_data_melody"] = settings.alarm_no_data_melody;
```

### Step 3: Update globals.h

Add the default melody constant:

```cpp
const char sound_no_data[] = "no_data:d=4,o=5,b=100:8c,8e,8g,8c6,4p,8c,8e,8g,8c6";
```

### Step 4: Replace BGAlarmManager.cpp

Use the provided `BGAlarmManager.cpp` file which includes:
- No-data alarm setup
- Modified tick() function
- Modified snoozeAlarm() function with "No Data" display
- Helper function to identify no-data alarms

### Step 5: Update Web UI (Optional)

Add the HTML/JS/CSS from `index_html_additions.html` to your `data/index.html` file.

If you don't have access to modify the web UI, you can still configure the alarm by:
1. Manually editing the `config.json` file on the device
2. Using the API directly

### Step 6: Update config.json Factory Defaults

Add these defaults to your `config.json` factory file:

```json
{
  "alarm_no_data_enabled": false,
  "alarm_no_data_minutes": 10,
  "alarm_no_data_snooze_interval": 10,
  "alarm_no_data_silence_interval": "",
  "alarm_no_data_melody": ""
}
```

### Step 7: Build and Flash

```bash
# Build the project
pio run

# Flash to device
pio run --target upload

# Upload filesystem (for web UI changes)
pio run --target uploadfs
```

## Configuration

### Via Web GUI

If you've updated the web UI:

1. Open the clock's web interface
2. Scroll to "No Data Alarm" section
3. Enable the alarm
4. Set your preferred:
   - Time threshold (5-60 minutes)
   - Snooze duration
   - Silence interval
   - Custom melody (optional)

### Via config.json

```json
{
  "alarm_no_data_enabled": true,
  "alarm_no_data_minutes": 15,
  "alarm_no_data_snooze_interval": 10,
  "alarm_no_data_silence_interval": "22_8",
  "alarm_no_data_melody": "Custom:d=4,o=5,b=100:8c6,8a5,8c6,8a5"
}
```

## How It Works

1. **Monitoring**: The alarm manager checks both:
   - Whether a glucose reading exists
   - How old the last reading is

2. **Triggering**: When data is older than the configured threshold:
   - Alarm activates (if not in silent period)
   - Sound plays
   - Display shows "No Data" when snoozed

3. **Clearing**: When fresh data arrives:
   - Alarm automatically clears
   - Normal glucose display resumes

4. **Snoozing**: Press any button to snooze
   - Snooze duration is configurable
   - Visual confirmation shown

## Troubleshooting

### Alarm not triggering
- Check that `alarm_no_data_enabled` is true
- Verify `alarm_no_data_minutes` is between 5-60
- Check if silence interval is active

### Alarm won't stop
- Press any button to snooze
- Check if snooze interval is set to 0 (disabled)
- Verify data source is working

### Settings not saving
- Check browser console for errors
- Verify JSON syntax in config.json
- Ensure sufficient free space on device

## Default Melody Options

Choose from these RTTTL melodies:

```cpp
// Gentle chime (bedroom-friendly)
"NoData:d=4,o=5,b=80:8e6,8d6,8c6,4g5"

// Beeps (attention-getting)
"NoData:d=8,o=5,b=120:c,e,g,c6,c,e,g,c6"

// Phone ring
"NoData:d=4,o=5,b=125:8c6,8a5,8c6,8a5,8c6,8a5,8c6,8a5"

// Clock chime
"NoData:d=2,o=4,b=100:8c5,8c5,8c5"

// Default (ascending arpeggio)
"no_data:d=4,o=5,b=100:8c,8e,8g,8c6,4p,8c,8e,8g,8c6"
```

## API Endpoints

### Test No-Data Alarm

```bash
curl -X POST http://nsclock.local/api/alarm \
  -H "Content-Type: application/json" \
  -d '{"alarmType": "no_data"}'
```

### Custom Melody Test

```bash
curl -X POST http://nsclock.local/api/alarm/custom \
  -H "Content-Type: application/json" \
  -d '{"rtttl": "NoData:d=4,o=5,b=100:8c,8e,8g,8c6"}'
```

## Files Included

- `BGAlarmManager.cpp` - Complete replacement file
- `Settings_h_additions.txt` - Fields to add to Settings.h
- `SettingsManager_cpp_additions.txt` - Code to add to SettingsManager.cpp
- `index_html_additions.html` - Web UI additions
- `globals_h_addition.txt` - Default melody constant

## Support

For issues or questions:
- Original project: https://github.com/ktomy/nightscout-clock
- RTTTL melody creator: https://adamonsoon.github.io/rtttl-play/
