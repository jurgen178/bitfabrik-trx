#include "SettingsManager.h"
#include "RadioEngine.h"
#include "AudioManager.h"
#include "DigitalEngine.h"

SettingsManager settings;

SettingsManager::SettingsManager()
{
}

void SettingsManager::begin()
{
    loadAll();
}

void SettingsManager::setUpdated()
{
    lastActivity = millis();
    isUpdated = true;
}

void SettingsManager::process()
{
    if (isUpdated && (millis() - lastActivity > 5000))
    {
        saveAll();
        isUpdated = false;
    }
}

void SettingsManager::loadAll()
{
    preferences.begin("trx", false);

    // Load Global/Misc settings
    digital.setMode(preferences.getInt("digi_mode", 0));

    // Delegate loading to engines
    radio.loadFromPreferences();

    // VOX
    radio.setVoxEnabled(preferences.getBool("vox_en", false));
    radio.setVoxThreshold(preferences.getInt("vox_thresh", 1000));
    radio.setVoxDelay(preferences.getInt("vox_delay", 500));

    // Load Audio settings (moved from Transceiver.ino)
    audio.setVolume(preferences.getInt("volume", 50));
    audio.setPaPower(preferences.getInt("pa_pwr", 100));
    audio.setMicGain(preferences.getInt("mic_gain", 50));

    preferences.end();
}

void SettingsManager::saveAll()
{
    preferences.begin("trx", false);

    // Save Global/Misc settings
    if (preferences.getInt("digi_mode", -1) != digital.getMode())
    {
        preferences.putInt("digi_mode", digital.getMode());
    }

    // Delegate saving
    radio.saveToPreferences();

    // Save VOX
    preferences.putBool("vox_en", radio.isVoxEnabled());
    preferences.putInt("vox_thresh", radio.getVoxThreshold());
    preferences.putInt("vox_delay", radio.getVoxDelay());

    // Save Audio settings (Smart-Save logic moved here)
    if (preferences.getInt("volume", -1) != audio.getVolume())
    {
        preferences.putInt("volume", audio.getVolume());
    }
    if (preferences.getInt("pa_pwr", -1) != audio.getPaPower())
    {
        preferences.putInt("pa_pwr", audio.getPaPower());
    }
    if (preferences.getInt("mic_gain", -1) != audio.getMicGain())
    {
        preferences.putInt("mic_gain", audio.getMicGain());
    }

    preferences.end();
}
