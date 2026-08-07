#include "AudioManager.h"
#include "Hardware.h"
#include "Display.h"
#include "NetworkManager.h"
#include "SettingsManager.h"
#include <SPI.h>

AudioManager audio;

AudioManager::AudioManager()
{
}

void AudioManager::begin()
{
    // PWM Signal Initialization for Dual-Volume Control (NF + ZF)
    #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcAttach(AUDIO_VOLUME_PWM, AUDIO_VOL_PWM_FREQ, AUDIO_VOL_PWM_RES);
        ledcAttach(ZF_AGC_PWM, ZF_AGC_PWM_FREQ, ZF_AGC_PWM_RES);
    #else
        ledcSetup(AUDIO_VOL_PWM_CHAN, AUDIO_VOL_PWM_FREQ, AUDIO_VOL_PWM_RES);
        ledcAttachPin(AUDIO_VOLUME_PWM, AUDIO_VOL_PWM_CHAN);
        ledcSetup(ZF_AGC_PWM_CHAN, ZF_AGC_PWM_FREQ, ZF_AGC_PWM_RES);
        ledcAttachPin(ZF_AGC_PWM, ZF_AGC_PWM_CHAN);
    #endif

    setVolume(volume);
    setPaPower(paPower);
    setMicGain(micGain);
}

/**
 * Sets receiver gain via Dual-PWM control:
 * 1. AUDIO_VOLUME_PWM (NF Stage / Speaker)
 * 2. ZF_AGC_PWM (ZF Stage / Pre-amplification)
 */
void AudioManager::setVolume(int newVolume)
{
    volume = constrain(newVolume, 0, 100);

    // Calculate NF Volume (Pin 13)
    int nfPwm = AUDIO_VOL_PWM_MIN + static_cast<int>(static_cast<float>(volume) * (AUDIO_VOL_PWM_MAX - AUDIO_VOL_PWM_MIN) / 100.0f);

    // Calculate ZF Gain (Pin 14)
    // Mapping 0-100% to ZF_AGC_MIN-255
    int zfPwm = 0;
    if (volume > 0)
    {
        zfPwm = map(volume, 1, 100, ZF_AGC_MIN, 255);
    }

    #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(AUDIO_VOLUME_PWM, nfPwm);
        ledcWrite(ZF_AGC_PWM, zfPwm);
    #else
        ledcWrite(AUDIO_VOL_PWM_CHAN, nfPwm);
        ledcWrite(ZF_AGC_PWM_CHAN, zfPwm);
    #endif

    g_guiNeedsUpdate = true;
    settings.setUpdated();
}

/**
 * Sets Sendeleistung (UI-Status).
 * Note: Hardware PWM for PA power is currently not mapped to a dedicated pin,
 * as Pin 14 is used for ZF Volume Gain.
 */
void AudioManager::setPaPower(int level)
{
    paPower = constrain(level, 0, 100);
    g_guiNeedsUpdate = true;
    settings.setUpdated();
}

/**
 * Sets Microphone Gain via MCP41010 Digital Potentiometer (SPI).
 */
void AudioManager::setMicGain(int gain)
{
    micGain = constrain(gain, 0, 100);

    // MCP41010: 16-bit command [00010001] [Data 0-255]
    // 0x11 = Write to Potentiometer 0
    uint8_t rawVal = static_cast<uint8_t>(static_cast<float>(micGain) * 2.55f);

    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        digitalWrite(MIC_CS, LOW);
        SPI.transfer(0x11);
        SPI.transfer(rawVal);
        digitalWrite(MIC_CS, HIGH);
        xSemaphoreGiveRecursive(g_hwMutex);
    }
    g_guiNeedsUpdate = true;
    settings.setUpdated();
}
