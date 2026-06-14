#include "settings.h"
#include <Preferences.h>

Preferences prefs;

Settings settings;

const Settings factorySettings = {
  .motorStartThreshold = 10,
  .motorStopThreshold = 100,

  .dayStartHour = 1,
  .dayEndHour = 23,

  .voltageMin = 220,
  .voltageMax = 250,
  .voltageFail = 110,

  .dryRunLockMs = 30000,
  .dryRunIntervalMs = 15000,
  .dryRunMinIncrease = 2,
  .dryRunMaxRetries = 2,
  .dryRunStartupDelayMs = 10000,

  .sensorTimeoutMs = 5000,
  .maxMotorRuntimeMs = 10UL * 60UL * 1000UL,
  .faultRecoveryTimeMs = 10000,

  .retryDelayMs = 5000
};

bool saveSettings() {
  prefs.begin("wlms", false);
  prefs.putUInt("ver", SETTINGS_VERSION);
  prefs.putBytes("cfg", &settings, sizeof(Settings));
  prefs.end();
  return true;
}

bool loadSettings() {
  prefs.begin("wlms", true);
  size_t len = prefs.getBytesLength("cfg");
  uint32_t ver = prefs.getUInt("ver", 0);

  if (ver != SETTINGS_VERSION) {
    prefs.end();
    return false;
  }
  if (len != sizeof(Settings)) {
    prefs.end();
    return false;
  }
  prefs.getBytes("cfg", &settings, sizeof(Settings));
  prefs.end();
  if (settings.motorStartThreshold >= settings.motorStopThreshold)
    return false;

  if (settings.motorStartThreshold > 100)
    return false;

  if (settings.motorStopThreshold > 100)
    return false;

  if (settings.voltageMin < 150)
    return false;

  if (settings.voltageFail >= settings.voltageMin)
    return false;

  if (settings.dayStartHour > 23)
    return false;

  if (settings.dayEndHour > 23)
    return false;
  return true;
}

void factoryReset() {
  settings = factorySettings;
  saveSettings();
}