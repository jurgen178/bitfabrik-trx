#include "RadioEngine.h"
#include "Hardware.h"
#include "Display.h"
#include "NetworkManager.h"
#include "SettingsManager.h"
#include <ArduinoJson.h>
#include <FFat.h>

RadioEngine radio;

RadioEngine::RadioEngine()
{
    // Initialize band frequencies with defaults from Constants.cpp
    for (int i = 0; i < NUM_BANDS; i++)
    {
        _bandFreqs[i] = BANDS[i].freqDefault;
    }
}

void RadioEngine::updateLO()
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        _updateLOInternal();
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::_updateLOInternal()
{
  double calcFreq = (double)_freq;
  // Apply Clarifier offset ONLY in RX mode
  if (!g_tx && _ritEnabled)
  {
      calcFreq += (double)_ritOffset;
  }
  dds_setFreq(calcFreq + ZF_FREQ, LO_FQUD);
}

void RadioEngine::updateBFO()
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        _updateBFOInternal();
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::_updateBFOInternal()
{
  dds_setFreq(_usb ? _bfoUsb : _bfoLsb, BFO_FQUD);
}

void RadioEngine::updateBandRelays()
{
    if (_band == _lastRelayBand)
        return;

    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        _updateBandRelaysInternal(_lastRelayBand, _band);
        _lastRelayBand = _band;
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::_updateBandRelaysInternal(int oldIdx, int newIdx)
{
    // 1. Turn OFF the previous band if valid and different from new
    if (oldIdx >= 0 && oldIdx < NUM_BANDS && oldIdx != newIdx)
    {
        mcp.digitalWrite(BANDS[oldIdx].rxRelay, LOW);
        mcp.digitalWrite(BANDS[oldIdx].txRelay, LOW);
    }
    // 2. Turn ON the new band
    if (newIdx >= 0 && newIdx < NUM_BANDS)
    {
        mcp.digitalWrite(BANDS[newIdx].rxRelay, HIGH);
        mcp.digitalWrite(BANDS[newIdx].txRelay, HIGH);
    }
}

void RadioEngine::refreshRelays()
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        _updateBandRelaysInternal(-1, _band); // Force current band to ON
        _lastRelayBand = _band;
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::selectBand(int idx)
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        _selectBandInternal(idx);
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::_selectBandInternal(int idx)
{
  if (idx < 0 || idx >= NUM_BANDS || !BANDS[idx].enabled)
  {
      return;
  }

  int oldBand = _band;

  // Save current frequency to band memory
  _bandFreqs[_band] = _freq;

  _band = idx;
  _freq = _bandFreqs[idx];

  // ALWAYS use the sideband defined in the BANDS table (Source of Truth)
  _usb  = BANDS[idx].sideBand;

  _updateBandRelaysInternal(oldBand, _band);
  _lastRelayBand = _band;
  _updateLOInternal();
  _updateBFOInternal();
  g_guiNeedsUpdate = true;
  
}

void RadioEngine::setStepIdx(int idx)
{
    _stepIdx = idx;
    g_guiNeedsUpdate = true;
    settings.setUpdated();
    
}

void RadioEngine::setUnlockedRange(bool en)
{
    _unlockedRange = en;
    g_guiNeedsUpdate = true;
    
}

void RadioEngine::setVoxEnabled(bool en)
{
    _voxEnabled = en;
    settings.setUpdated();
    
}

void RadioEngine::setVoxThreshold(int val)
{
    _voxThreshold = constrain(val, 0, 4095);
    settings.setUpdated();
    
}

void RadioEngine::setVoxDelay(int ms)
{
    _voxDelay = constrain(ms, 0, 5000);
    settings.setUpdated();
    
}

void RadioEngine::saveActiveToVfo()
{
  if (_activeVfo == 0)
  {
      _vfoA = { (long)_freq, (int)_band, (bool)_usb };
  }
  else
  {
      _vfoB = { (long)_freq, (int)_band, (bool)_usb };
  }
}

void RadioEngine::switchVfo(int target)
{
  if (target == _activeVfo)
  {
      return;
  }
  saveActiveToVfo();
  _activeVfo = target;
  VfoState& next = (_activeVfo == 0) ? _vfoA : _vfoB;

  if (next.band != _band)
  {
      selectBand(next.band);
  }

  _freq = next.freq;
  _usb = next.usb;

  updateLO();
  updateBFO();
  g_guiNeedsUpdate = true;
  
}

void RadioEngine::vfoCopy()
{
  if (_activeVfo == 0)
  {
      _vfoB = { (long)_freq, (int)_band, (bool)_usb };
  }
  else
  {
      _vfoA = { (long)_freq, (int)_band, (bool)_usb };
  }
  g_guiNeedsUpdate = true;
  g_sync.vfoCopyFlash = true;
}

void RadioEngine::memStore(int ch)
{
  if (ch >= 0 && ch < NUM_MEM_CHANNELS)
  {
    _memChannels[ch] = { (long)_freq, (int)_band, (bool)_usb, true };
    _memRevision++;
    settings.setUpdated(); // Trigger auto-save to Flash
    g_guiNeedsUpdate = true;
    
  }
}

void RadioEngine::memRecall(int ch)
{
  if (ch < 0 || ch >= NUM_MEM_CHANNELS)
  {
      return;
  }
  VfoState& m = _memChannels[ch];
  if (!m.occupied) return; // Slot is empty — nothing to recall
  if (m.band != _band)
  {
      selectBand(m.band);
  }
  _freq = m.freq;
  _usb = m.usb;
  updateLO();
  updateBFO();
  g_guiNeedsUpdate = true;
  
}

void RadioEngine::memDelete(int ch)
{
  if (ch >= 0 && ch < NUM_MEM_CHANNELS)
  {
    _memChannels[ch].occupied = false;
    _memRevision++;
    settings.setUpdated(); // Trigger auto-save
    g_guiNeedsUpdate = true;
    
  }
}

void RadioEngine::loadBandsFromJson()
{
    if (!FFat.exists("/bands.json")) return;

    File file = FFat.open("/bands.json", "r");
    if (!file) return;

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("JSON: Failed to parse bands.json");
        return;
    }

    JsonArray array = doc.as<JsonArray>();
    for (int i = 0; i < array.size() && i < NUM_BANDS; i++) {
        JsonObject obj = array[i];
        // Map JSON to BANDS array (which is now dynamic)
        BANDS[i].name        = strdup(obj["id"] | BANDS[i].name);
        BANDS[i].freqMin     = obj["min"] | BANDS[i].freqMin;
        BANDS[i].freqMax     = obj["max"] | BANDS[i].freqMax;
        BANDS[i].freqDefault = obj["def"] | BANDS[i].freqDefault;
        BANDS[i].rxRelay     = obj["rx_filter"] | BANDS[i].rxRelay;
        BANDS[i].txRelay     = obj["tx_filter"] | BANDS[i].txRelay;
        BANDS[i].sideBand    = obj["usb"] | BANDS[i].sideBand;
        BANDS[i].enabled     = obj["active"] | BANDS[i].enabled;
    }
    Serial.println("JSON: Band configuration loaded");
}

void RadioEngine::saveBandsToJson()
{
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < NUM_BANDS; i++) {
        JsonObject obj = array.createNestedObject();
        obj["id"]    = BANDS[i].name;
        obj["min"]   = BANDS[i].freqMin;
        obj["max"]   = BANDS[i].freqMax;
        obj["def"]   = BANDS[i].freqDefault;
        obj["rx_filter"] = BANDS[i].rxRelay;
        obj["tx_filter"] = BANDS[i].txRelay;
        obj["usb"]   = BANDS[i].sideBand;
        obj["active"] = BANDS[i].enabled;
    }

    File file = FFat.open("/bands.json", "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
        Serial.println("JSON: Band configuration saved");
    }
}

void RadioEngine::setFrequency(long f)
{
    _freq = constrain(f, getMinFreq(), getMaxFreq());
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        _updateLOInternal();
        xSemaphoreGiveRecursive(g_hwMutex);
    }
    g_guiNeedsUpdate = true;
    
}

long RadioEngine::getMinFreq() const
{
    return _unlockedRange ? 10L : BANDS[_band].freqMin;
}

long RadioEngine::getMaxFreq() const
{
    return _unlockedRange ? 30000000L : BANDS[_band].freqMax;
}

void RadioEngine::setRitEnabled(bool en)
{
    _ritEnabled = en;
    updateLO();
    g_guiNeedsUpdate = true;
    
}

void RadioEngine::setRitOffset(long offset)
{
    _ritOffset = offset;
    if (!g_tx)
    {
        updateLO();
    }
    g_guiNeedsUpdate = true;
    
}

void RadioEngine::setUtcOffset(int hours)
{
    _utcOffset = hours;
    settings.setUpdated();
    
}

void RadioEngine::setDstActive(bool active)
{
    _dstActive = active;
    settings.setUpdated();
    
}

void RadioEngine::loadFromPreferences()
{
  _bfoUsb = preferences.getDouble("bfo_usb", 9001500.0);
  _bfoLsb = preferences.getDouble("bfo_lsb", 8998500.0);
  _stepIdx = preferences.getInt("stepIdx", 1);

  if (preferences.getBytes("vfoA", &_vfoA, sizeof(VfoState)) == 0)
  {
      _vfoA = { 7100000, 3, false };
  }
  if (preferences.getBytes("vfoB", &_vfoB, sizeof(VfoState)) == 0)
  {
      _vfoB = { 14200000, 2, true };
  }
  if (preferences.getBytes("mems", _memChannels, sizeof(_memChannels)) == 0)
  {
    for(int i = 0; i < NUM_MEM_CHANNELS; i++)
    {
        _memChannels[i] = { 0L, 0, false, false }; // occupied=false = empty slot
    }
  }

  _activeVfo = preferences.getInt("vfoActive", 0);
  _utcOffset = preferences.getInt("utc_off", 1);
  _dstActive = preferences.getBool("dst_act", true);
  VfoState& act = (_activeVfo == 0) ? _vfoA : _vfoB;
  _freq = act.freq;
  _band = act.band;
  _usb = act.usb;

  for (int i = 0; i < NUM_BANDS; i++)
  {
    char k[8];
    snprintf(k, 8, "f%d", i);
    _bandFreqs[i] = preferences.getLong(k, BANDS[i].freqDefault);
  }
}

void RadioEngine::saveToPreferences()
{
  // Smart-Save: Only write if changed to save Flash cycles
  if (preferences.getDouble("bfo_usb", 0) != _bfoUsb)
  {
      preferences.putDouble("bfo_usb", _bfoUsb);
  }
  if (preferences.getDouble("bfo_lsb", 0) != _bfoLsb)
  {
      preferences.putDouble("bfo_lsb", _bfoLsb);
  }
  if (preferences.getInt("stepIdx", -1) != _stepIdx)
  {
      preferences.putInt("stepIdx", _stepIdx);
  }

  saveActiveToVfo();

  // Smart-Save for structs: compare before writing
  VfoState tmp;
  if (preferences.getBytes("vfoA", &tmp, sizeof(VfoState)) != sizeof(VfoState) || memcmp(&tmp, &_vfoA, sizeof(VfoState)) != 0)
  {
    preferences.putBytes("vfoA", &_vfoA, sizeof(VfoState));
  }
  if (preferences.getBytes("vfoB", &tmp, sizeof(VfoState)) != sizeof(VfoState) || memcmp(&tmp, &_vfoB, sizeof(VfoState)) != 0)
  {
    preferences.putBytes("vfoB", &_vfoB, sizeof(VfoState));
  }

  VfoState tmpMems[NUM_MEM_CHANNELS];
  if (preferences.getBytes("mems", tmpMems, sizeof(tmpMems)) != sizeof(tmpMems) || memcmp(tmpMems, _memChannels, sizeof(tmpMems)) != 0)
  {
    preferences.putBytes("mems", _memChannels, sizeof(_memChannels));
  }

  if (preferences.getInt("vfoActive", -1) != _activeVfo)
  {
      preferences.putInt("vfoActive", _activeVfo);
  }

  if (preferences.getInt("utc_off", 99) != _utcOffset)
  {
      preferences.putInt("utc_off", _utcOffset);
  }
  if (preferences.getBool("dst_act", !_dstActive) != _dstActive)
  {
      preferences.putBool("dst_act", _dstActive);
  }

  for (int i = 0; i < NUM_BANDS; i++)
  {
    char key[8];
    snprintf(key, 8, "f%d", i);
    if (preferences.getLong(key, 0) != _bandFreqs[i])
    {
        preferences.putLong(key, _bandFreqs[i]);
    }
  }
}
