#include "AudioManager.h"
#include "Hardware.h"
#include "Display.h"
#include <SPI.h>

AudioManager audio;

AudioManager::AudioManager()
{
}

void AudioManager::begin()
{
    // PWM Signal Initialization for Audio Volume
    #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcAttach(PIN_VOLUME_PWM, VOL_PWM_FREQ, VOL_PWM_RES);
        ledcAttach(PIN_PA_PWR_PWM, PA_PWR_PWM_FREQ, PA_PWR_PWM_RES);
    #else
        ledcSetup(VOL_PWM_CHAN, VOL_PWM_FREQ, VOL_PWM_RES);
        ledcAttachPin(PIN_VOLUME_PWM, VOL_PWM_CHAN);
        ledcSetup(PA_PWR_PWM_CHAN, PA_PWR_PWM_FREQ, PA_PWR_PWM_RES);
        ledcAttachPin(PIN_PA_PWR_PWM, PA_PWR_PWM_CHAN);
    #endif

    setVolume(_volume);
    setPaPower(_paPower);
    setMicGain(_micGain);
}

/**
 * Sets receiver gain via PWM low-pass filtered voltage.
 */
void AudioManager::setVolume(int vol)
{
    _volume = constrain(vol, 0, 100);
    int pwmVal = VOL_PWM_MIN + (int)((float)_volume * (VOL_PWM_MAX - VOL_PWM_MIN) / 100.0f);

    #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(PIN_VOLUME_PWM, pwmVal);
    #else
        ledcWrite(VOL_PWM_CHAN, pwmVal);
    #endif

    g_guiNeedsUpdate = true;
    notifyWebUpdate();
}

/**
 * Sets PA output power level via PWM bias/driver control.
 */
void AudioManager::setPaPower(int level)
{
    _paPower = constrain(level, 0, 100);

    int pwmVal = 0;
    if (_paPower > 0)
    {
        // Map 1-100% to PA_PWR_MIN-255 (Bias range)
        pwmVal = map(_paPower, 1, 100, PA_PWR_MIN, 255);
    }

    #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(PIN_PA_PWR_PWM, pwmVal);
    #else
        ledcWrite(PA_PWR_PWM_CHAN, pwmVal);
    #endif

    g_guiNeedsUpdate = true;
    notifyWebUpdate();
}

/**
 * Sets Microphone Gain via MCP41010 Digital Potentiometer (SPI).
 */
void AudioManager::setMicGain(int gain)
{
    _micGain = constrain(gain, 0, 100);

    // MCP41010: 16-bit command [00010001] [Data 0-255]
    // 0x11 = Write to Potentiometer 0
    uint8_t rawVal = (uint8_t)((float)_micGain * 2.55f);

    if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(50)))
    {
        digitalWrite(MIC_CS, LOW);
        SPI.transfer(0x11);
        SPI.transfer(rawVal);
        digitalWrite(MIC_CS, HIGH);
        xSemaphoreGiveRecursive(g_hwMutex);
    }
    g_guiNeedsUpdate = true;
    notifyWebUpdate();
}
