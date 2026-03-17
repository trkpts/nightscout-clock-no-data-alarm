// BGAlarmManager.cpp - MODIFIED VERSION WITH NO-DATA ALARM
// Replace your existing BGAlarmManager.cpp with this version

#include <BGAlarmManager.h>
#include <BGDisplayManager.h>
#include <PeripheryManager.h>
#include <SettingsManager.h>

#include "ServerManager.h"
#include "globals.h"

#define ALARM_REPEAT_INTERVAL_SECONDS 300
#define ALARM_REPEAT_INTERVAL_INTENSIVE_SECONDS 2

static String pickAlarmMelody(const String& customMelody, const String& fallbackMelody) {
    if (customMelody.length() > 0) {
        return customMelody;
    }
    return fallbackMelody;
}

// The getter for the instantiated singleton instance
BGAlarmManager_& BGAlarmManager_::getInstance() {
    static BGAlarmManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGAlarmManager_& bgAlarmManager = bgAlarmManager.getInstance();

#ifdef DEBUG_ALARMS
int debounceTicks = 0;
int debounceTicks2 = 0;
int debounceTicks3 = 0;
int debounceTicks4 = 0;
int debounceTicks5 = 0;
int debounceTicks6 = 0;
int debounceTicks7 = 0;
int debounceTicks8 = 0;
int debounceTicks9 = 0;  // Added for no-data alarm
#endif

void BGAlarmManager_::setup() {
    // Clear existing alarms
    enabledAlarms.clear();
    
    if (SettingsManager.settings.alarm_urgent_low_enabled) {
        AlarmData alarmData;
        alarmData.bottom = 1;
        alarmData.top = SettingsManager.settings.alarm_urgent_low_mgdl;
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_urgent_low_snooze_minutes;
        alarmData.silenceInterval = SettingsManager.settings.alarm_urgent_low_silence_interval;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound =
            pickAlarmMelody(SettingsManager.settings.alarm_urgent_low_melody, sound_urgent_low);
        alarmData.isSnoozed = false;
        enabledAlarms.push_back(alarmData);
    }
    if (SettingsManager.settings.alarm_low_enabled) {
        AlarmData alarmData;
        alarmData.bottom = SettingsManager.settings.alarm_urgent_low_mgdl + 1;
        alarmData.top = SettingsManager.settings.alarm_low_mgdl - 1;
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_low_snooze_minutes;
        alarmData.silenceInterval = SettingsManager.settings.alarm_low_silence_interval;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound = pickAlarmMelody(SettingsManager.settings.alarm_low_melody, sound_low);
        alarmData.isSnoozed = false;
        enabledAlarms.push_back(alarmData);
    }
    if (SettingsManager.settings.alarm_high_enabled) {
        AlarmData alarmData;
        alarmData.bottom = SettingsManager.settings.alarm_high_mgdl;
        alarmData.top = 401;
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_high_snooze_minutes;
        alarmData.silenceInterval = SettingsManager.settings.alarm_high_silence_interval;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound = pickAlarmMelody(SettingsManager.settings.alarm_high_melody, sound_high);
        alarmData.isSnoozed = false;
        enabledAlarms.push_back(alarmData);
    }
    
    // NO DATA ALARM - NEW
    if (SettingsManager.settings.alarm_no_data_enabled) {
        AlarmData alarmData;
        alarmData.bottom = -1;  // Special marker for no-data alarm
        alarmData.top = -1;     // Special marker for no-data alarm
        alarmData.snoozeTimeMinutes = SettingsManager.settings.alarm_no_data_snooze_minutes;
        alarmData.silenceInterval = SettingsManager.settings.alarm_no_data_silence_interval;
        alarmData.lastAlarmTime = 0;
        alarmData.alarmSound =
            pickAlarmMelody(SettingsManager.settings.alarm_no_data_melody, sound_no_data);
        alarmData.isSnoozed = false;
        enabledAlarms.push_back(alarmData);
    }

    if (SettingsManager.settings.alarm_intensive_mode) {
        alarmIntervalSeconds = ALARM_REPEAT_INTERVAL_INTENSIVE_SECONDS;
    } else {
        alarmIntervalSeconds = ALARM_REPEAT_INTERVAL_SECONDS;
    }
}

bool isInSilentInterval(String silenceInterval) {
    if (silenceInterval == "" || silenceInterval == "0") {
        return false;
    }

    auto time = ServerManager.getTimezonedTime();

    if (silenceInterval == "22_8" && (time.tm_hour >= 22 || time.tm_hour < 8)) {
        return true;
    }

    if (silenceInterval == "8_22" && (time.tm_hour >= 8 && time.tm_hour < 22)) {
        return true;
    }

    return false;
}

bool isNoDataAlarm(const AlarmData& alarm) {
    return alarm.bottom == -1 && alarm.top == -1;
}

void BGAlarmManager_::tick() {
    auto glucoseReading = bgDisplayManager.getLastDisplayedGlucoseReading();
    
    // Check no-data condition
    bool isDataStale = false;
    
    // Check if glucose reading itself is old
    if (glucoseReading == nullptr) {
        isDataStale = true;
    } else if (glucoseReading->getSecondsAgo() > SettingsManager.settings.alarm_no_data_minutes * 60) {
        isDataStale = true;
    }
    
    // Handle no-data alarms
    for (AlarmData& alarmData : enabledAlarms) {
        if (isNoDataAlarm(alarmData)) {
            if (isDataStale) {
                // Data is stale - trigger or maintain alarm
                if (isInSilentInterval(alarmData.silenceInterval)) {
                    if (activeAlarm != NULL && isNoDataAlarm(*activeAlarm)) {
                        activeAlarm->isSnoozed = false;
                        activeAlarm->lastAlarmTime = 0;
                    }
                    if (activeAlarm == &alarmData) {
                        activeAlarm = NULL;
                    }
                    return;
                }
                
                if (activeAlarm == NULL) {
                    // New alarm
                    activeAlarm = &alarmData;
                    alarmData.lastAlarmTime = ServerManager.getUtcEpoch();
                    alarmData.isSnoozed = false;
                    PeripheryManager.playRTTTLString(alarmData.alarmSound);
                    DEBUG_PRINTLN("Playing NO DATA alarm (new)");
                } else if (activeAlarm == &alarmData) {
                    // Existing alarm
                    if (activeAlarm->isSnoozed) {
                        if (activeAlarm->snoozeTimeMinutes != 0 &&
                            ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime >
                                60 * activeAlarm->snoozeTimeMinutes) {
                            activeAlarm->isSnoozed = false;
                            activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                            PeripheryManager.playRTTTLString(alarmData.alarmSound);
                            DEBUG_PRINTLN("Playing NO DATA alarm (after snooze)");
                        }
                    } else {
                        if (ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime >
                            alarmIntervalSeconds) {
                            activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                            PeripheryManager.playRTTTLString(alarmData.alarmSound);
                            DEBUG_PRINTLN("Playing NO DATA alarm (repeat)");
                        }
                    }
                }
                return;
            } else {
                // Data is fresh - clear no-data alarm if active
                if (activeAlarm != NULL && isNoDataAlarm(*activeAlarm)) {
                    activeAlarm->isSnoozed = false;
                    activeAlarm->lastAlarmTime = 0;
                    activeAlarm = NULL;
                }
            }
        }
    }
    
    // Handle glucose value alarms (original logic)
    if (glucoseReading == nullptr ||
        glucoseReading->getSecondsAgo() >
            SettingsManager.settings.bg_data_too_old_threshold_minutes * 60) {
        // Don't process glucose alarms if data is too old
        return;
    }

    for (AlarmData& alarmData : enabledAlarms) {
        if (isNoDataAlarm(alarmData)) continue;  // Skip no-data alarms
        
        if (glucoseReading->sgv >= alarmData.bottom && glucoseReading->sgv <= alarmData.top) {
            if (isInSilentInterval(alarmData.silenceInterval)) {
                if (activeAlarm != NULL) {
                    activeAlarm->isSnoozed = false;
                    activeAlarm->lastAlarmTime = 0;
                }
                activeAlarm = NULL;
                return;
            }
            if (activeAlarm == NULL) {
                activeAlarm = &alarmData;
                alarmData.lastAlarmTime = ServerManager.getUtcEpoch();
                alarmData.isSnoozed = false;
                PeripheryManager.playRTTTLString(alarmData.alarmSound);
                DEBUG_PRINTLN("Playing alarm sound (new alarm)");
            } else {
                if (activeAlarm->isSnoozed) {
                    if (activeAlarm->snoozeTimeMinutes != 0 &&
                        ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime >
                            60 * activeAlarm->snoozeTimeMinutes) {
                        activeAlarm->isSnoozed = false;
                        activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                        PeripheryManager.playRTTTLString(alarmData.alarmSound);
                        DEBUG_PRINTLN("Playing alarm sound after snooze");
                    }
                } else {
                    if (ServerManager.getUtcEpoch() - activeAlarm->lastAlarmTime >
                        alarmIntervalSeconds) {
                        activeAlarm->lastAlarmTime = ServerManager.getUtcEpoch();
                        PeripheryManager.playRTTTLString(alarmData.alarmSound);
                        DEBUG_PRINTLN("Playing alarm sound (repeat)");
                    }
                }
            }
            return;
        }
    }
    
    if (activeAlarm != NULL && !isNoDataAlarm(*activeAlarm)) {
        activeAlarm->isSnoozed = false;
        activeAlarm->lastAlarmTime = 0;
    }
    if (activeAlarm != NULL && !isNoDataAlarm(*activeAlarm)) {
        activeAlarm = NULL;
    }
}

void BGAlarmManager_::snoozeAlarm() {
    if (activeAlarm != NULL && !activeAlarm->isSnoozed) {
        DEBUG_PRINTLN("Snoozing alarm");
        DisplayManager.clearMatrix();
        DisplayManager.setTextColor(COLOR_CYAN);
        
        // Show different message for no-data alarm
        if (isNoDataAlarm(*activeAlarm)) {
            DisplayManager.printText(0, 6, "No Data", TEXT_ALIGNMENT::CENTER, 0);
        } else {
            DisplayManager.printText(0, 6, "Snoozed", TEXT_ALIGNMENT::CENTER, 0);
        }
        
        delay(2000);
        bgDisplayManager.maybeRrefreshScreen(true);
        activeAlarm->isSnoozed = true;
    }
}
