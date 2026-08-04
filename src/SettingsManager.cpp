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
    _lastActivity = millis();
    _isUpdated = true;
}

void SettingsManager::process()
{
    if (_isUpdated && (millis() - _lastActivity > 5000))
    {
        saveAll();
        _isUpdated = false;
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
    int vol = preferences.getInt("volume", 50);
    int pwr = preferences.getInt("pa_pwr", 100);
    int mic = preferences.getInt("mic_gain", 50);

    // Note: AudioManager::begin() will call its own setters if we want,
    // but here we just set the values and let audio.begin() handle the rest
    // if it's called after this.
    // However, audio.begin() is called in setup().

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
