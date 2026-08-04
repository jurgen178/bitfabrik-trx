#include "RadioEngine.h"
#include "Hardware.h"
#include "Display.h"
#include "SettingsManager.h"

RadioEngine radio;

RadioEngine::RadioEngine()
{
    // Initial state set in header defaults
}

void RadioEngine::updateLO()
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
  dds_setFreq(_usb ? _bfoUsb : _bfoLsb, BFO_FQUD);
}

void RadioEngine::updateBandRelays()
{
    if (_band == _lastRelayBand)
        return; // Prevent redundant I2C traffic

    if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(50)))
    {
        Serial.printf("MCP: Updating Band Filters for %s\n", BANDS[_band].label);
        int rxTarget = BANDS[_band].rxRelay;
        int txTarget = BANDS[_band].txRelay;

        // Set RX Bank (Pins 0-5)
        for (int i = 0; i <= 5; i++)
        {
            mcp.digitalWrite(i, (i == rxTarget) ? HIGH : LOW);
        }

        // Set TX Bank (Pins 8-13)
        for (int i = 8; i <= 13; i++)
        {
            mcp.digitalWrite(i, (i == txTarget) ? HIGH : LOW);
        }

        _lastRelayBand = _band;
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::refreshRelays()
{
    _lastRelayBand = -1; // Force re-application regardless of cached state
    updateBandRelays();
}

void RadioEngine::selectBand(int idx)
{
  if (idx < 0 || idx >= NUM_BANDS || !BANDS[idx].enabled)
  {
      return;
  }

  // Save current frequency/mode to band memory
  _bandFreqs[_band] = _freq;
  _bandModes[_band] = _usb;

  _band = idx;
  _freq = _bandFreqs[idx];
  _usb  = _bandModes[idx];

  updateBandRelays();
  updateLO();
  updateBFO();
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
    notifyWebUpdate();
}

void RadioEngine::setVoxEnabled(bool en)
{
    _voxEnabled = en;
    settings.setUpdated();
    notifyWebUpdate();
}

void RadioEngine::setVoxThreshold(int val)
{
    _voxThreshold = constrain(val, 0, 4095);
    settings.setUpdated();
    notifyWebUpdate();
}

void RadioEngine::setVoxDelay(int ms)
{
    _voxDelay = constrain(ms, 0, 5000);
    settings.setUpdated();
    notifyWebUpdate();
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
}

void RadioEngine::memStore(int ch)
{
  if (ch >= 0 && ch < NUM_MEM_CHANNELS)
  {
    _memChannels[ch] = { (long)_freq, (int)_band, (bool)_usb, true };
    _memRevision++;
    settings.setUpdated(); // Trigger auto-save to Flash
    g_guiNeedsUpdate = true;
    notifyWebUpdate();
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
    notifyWebUpdate();
  }
}

void RadioEngine::setFrequency(long f)
{
    _freq = constrain(f, getMinFreq(), getMaxFreq());
    updateLO();
    g_guiNeedsUpdate = true;
    notifyWebUpdate();
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
    notifyWebUpdate();
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
  VfoState& act = (_activeVfo == 0) ? _vfoA : _vfoB;
  _freq = act.freq;
  _band = act.band;
  _usb = act.usb;

  for (int i = 0; i < NUM_BANDS; i++)
  {
    char k[8];
    snprintf(k, 8, "f%d", i);
    _bandFreqs[i] = preferences.getLong(k, BANDS[i].freqDefault);
    // Note: band modes could also be loaded if saved
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
