#include "EncoderActions.h"
#include "Hardware.h"
#include "RadioEngine.h"
#include "AudioManager.h"
#include "SettingsManager.h"
#include "Display.h"

/**
 * ── TUNE HANDLER ──────────────────────────────────────────────────────────
 * Standard frequency control mode with speed-sensitive acceleration.
 * ──────────────────────────────────────────────────────────────────────────
 */
void TuneHandler::onRotate(int delta)
{
    if (g_tx)
    {
        return; // Prevent frequency change during transmission
    }
    long mult = 1;
    unsigned long dT = g_encInterval; // Use the true physical interval measured in ISR

    // Dynamic acceleration logic - Separated by hardware type
#if ENC_TYPE == 1
    // Optical Encoder (High pulse rate)
    if (dT < 6)
    {
        mult = 20; // Turbo
    }
    else if (dT < 12)
    {
        mult = 10; // Fast
    }
    else if (dT < 25)
    {
        mult = 4; // Medium
    }
    // > 25ms is 1:1 precision
#else
    // Mechanical Encoder (Low pulse rate)
    if (dT < 30)
    {
        mult = 20;
    }
    else if (dT < 60)
    {
        mult = 10;
    }
    else if (dT < 100)
    {
        mult = 4;
    }
#endif

    radio.setFrequency(radio.getFrequency() + delta * STEPS[radio.getStepIdx()] * mult);
    settings.setUpdated();
}

/**
 * Generates the professional status line for OLED (e.g., "40m - LSB S:1k").
 */
String TuneHandler::getDisplayLabel()
{
    char stepBuf[16];
    long st = STEPS[radio.getStepIdx()];
    if (st >= 1000)
    {
        snprintf(stepBuf, sizeof(stepBuf), "%ldk", st/1000);
    }
    else
    {
        snprintf(stepBuf, sizeof(stepBuf), "%ld", st);
    }

    char buf[48];
    snprintf(buf, sizeof(buf), "%s - %s S:%s", BANDS[radio.getBand()].label, radio.isUsb() ? "USB" : "LSB", stepBuf);
    return String(buf);
}

void TuneHandler::renderFocused(Adafruit_SSD1306 &display)
{
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.printf("%ld.%03ld", radio.getFrequency() / 1000000, (radio.getFrequency() % 1000000) / 1000);
    display.setTextSize(1);
    display.printf(".%03ld", radio.getFrequency() % 1000);
}

/**
 * ── VOLUME HANDLER ────────────────────────────────────────────────────────
 */
void VolumeHandler::onRotate(int delta)
{
    audio.setVolume(audio.getVolume() + delta);
}

String VolumeHandler::getDisplayLabel()
{
    return "VOLUME CONTROL";
}

void VolumeHandler::renderFocused(Adafruit_SSD1306 &display)
{
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.printf("VOL: %d%%", audio.getVolume());
}

/**
 * ── POWER HANDLER ─────────────────────────────────────────────────────────
 */
void PowerHandler::onRotate(int delta)
{
    audio.setPaPower(audio.getPaPower() + delta);
}

String PowerHandler::getDisplayLabel()
{
    return "PA POWER LEVEL";
}

void PowerHandler::renderFocused(Adafruit_SSD1306 &display)
{
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.printf("PWR: %d%%", audio.getPaPower());
}

/**
 * ── MIC GAIN HANDLER ──────────────────────────────────────────────────────
 */
void MicGainHandler::onRotate(int delta)
{
    audio.setMicGain(audio.getMicGain() + delta);
}

String MicGainHandler::getDisplayLabel()
{
    return "MIC GAIN CONTROL";
}

void MicGainHandler::renderFocused(Adafruit_SSD1306 &display)
{
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.printf("MIC: %d%%", audio.getMicGain());
}

/**
 * ── CALIBRATE HANDLER ─────────────────────────────────────────────────────
 */
void CalibrateHandler::onRotate(int delta)
{
    if (radio.isUsb())
    {
        radio.setBfoUsb(radio.getBfoUsb() + delta * 50.0);
    }
    else
    {
        radio.setBfoLsb(radio.getBfoLsb() + delta * 50.0);
    }
    g_guiNeedsUpdate = true;
}

String CalibrateHandler::getDisplayLabel()
{
    return "BFO CALIBRATION";
}

void CalibrateHandler::renderFocused(Adafruit_SSD1306 &display)
{
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.printf("%.1f Hz", radio.isUsb() ? radio.getBfoUsb() : radio.getBfoLsb());
}

/**
 * ── RIT HANDLER ───────────────────────────────────────────────────────────
 */
void RitHandler::onRotate(int delta)
{
    radio.setRitOffset(radio.getRitOffset() + delta * 10); // 10Hz steps
}

String RitHandler::getDisplayLabel()
{
    return "RIT OFFSET";
}

void RitHandler::renderFocused(Adafruit_SSD1306 &display)
{
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.printf("%+ld Hz", radio.getRitOffset());
}

/**
 * ── ENCODER MANAGER ────────────────────────────────────────────────────────
 */
EncoderManager::EncoderManager()
{
    setMode(EncoderMode::Tune);
}

void EncoderManager::setMode(EncoderMode mode)
{
    if (mode == _mode && _currentHandler) return;

    if (_currentHandler)
    {
        _currentHandler->onLeave();
    }

    _mode = mode;
    switch(mode)
    {
        case EncoderMode::Volume:
            _currentHandler = &_vol;
            ui.setMode(DisplayMode::Volume);
            break;
        case EncoderMode::Power:
            _currentHandler = &_power;
            ui.setMode(DisplayMode::Power);
            break;
        case EncoderMode::Mic:
            _currentHandler = &_mic;
            ui.setMode(DisplayMode::Mic);
            break;
        case EncoderMode::Calibrate:
            _currentHandler = &_cal;
            break;
        case EncoderMode::Rit:
            _currentHandler = &_rit;
            ui.setMode(DisplayMode::Rit);
            break;
        default:
            _currentHandler = &_tune;
            ui.setMode(DisplayMode::Radio);
            break;
    }

    _currentHandler->onEnter();
    _lastActivity = millis();
    g_guiNeedsUpdate = true;
}

void EncoderManager::cycleMode()
{
    if (_mode == EncoderMode::Tune)
    {
        setMode(EncoderMode::Volume);
    }
    else if (_mode == EncoderMode::Volume)
    {
        setMode(EncoderMode::Power);
    }
    else if (_mode == EncoderMode::Rit)
    {
        setMode(EncoderMode::Tune);
    }
    else
    {
        setMode(EncoderMode::Tune);
    }
}

void EncoderManager::handleRotation(int delta)
{
    if (!_currentHandler)
    {
        setMode(EncoderMode::Tune);
    }
    _currentHandler->onRotate(delta);
    _lastActivity = millis();
}

void EncoderManager::checkTimeout()
{
    if (_mode != EncoderMode::Tune && _mode != EncoderMode::Calibrate)
    {
        if (millis() - _lastActivity > 5000)
        {
            setMode(EncoderMode::Tune);
        }
    }
}
