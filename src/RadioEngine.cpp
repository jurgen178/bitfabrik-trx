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
        bandFreqs[i] = BANDS[i].freqDefault;
    }
}

void RadioEngine::updateLO()
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        updateLOInternal();
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::updateLOInternal()
{
  double calcFreq = static_cast<double>(freq);
  // Apply Clarifier offset ONLY in RX mode
  if (!g_tx && ritEnabled)
  {
      calcFreq += static_cast<double>(ritOffset);
  }
  dds_setFreq(calcFreq + ZF_FREQ, LO_FQUD);
}

void RadioEngine::updateBFO()
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        updateBFOInternal();
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::updateBFOInternal()
{
  dds_setFreq(usb ? bfoUsb : bfoLsb, BFO_FQUD);
}

void RadioEngine::updateBandRelays()
{
    if (band == lastRelayBand)
        return;

    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        updateBandRelaysInternal(lastRelayBand, band);
        lastRelayBand = band;
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::updateBandRelaysInternal(int oldIdx, int newIdx)
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
        updateBandRelaysInternal(-1, band); // Force current band to ON
        lastRelayBand = band;
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void RadioEngine::selectBand(int idx)
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        selectBandInternal(idx);
        xSemaphoreGiveRecursive(g_hwMutex);
        settings.setUpdated();
    }
}

void RadioEngine::selectBandInternal(int idx)
{
  if (idx < 0 || idx >= NUM_BANDS || !BANDS[idx].enabled)
  {
      return;
  }

  int oldBand = band;

  // Save current frequency to band memory
  bandFreqs[band] = freq;

  band = idx;
  freq = bandFreqs[idx];

  // ALWAYS use the sideband defined in the BANDS table (Source of Truth)
  usb  = BANDS[idx].sideBand;

  updateBandRelaysInternal(oldBand, band);
  lastRelayBand = band;
  updateLOInternal();
  updateBFOInternal();
  g_guiNeedsUpdate = true;
}

void RadioEngine::setStepIdx(int newIdx)
{
    stepIdx = newIdx;
    g_guiNeedsUpdate = true;
    settings.setUpdated();
}

void RadioEngine::setUnlockedRange(bool enabled)
{
    unlockedRange = enabled;
    g_guiNeedsUpdate = true;
}

void RadioEngine::setVoxEnabled(bool enabled)
{
    voxEnabled = enabled;
    settings.setUpdated();
}

void RadioEngine::setVoxThreshold(int threshold)
{
    voxThreshold = constrain(threshold, 0, 4095);
    settings.setUpdated();
}

void RadioEngine::setVoxDelay(int ms)
{
    voxDelay = constrain(ms, 0, 5000);
    settings.setUpdated();
}

void RadioEngine::saveActiveToVfo()
{
  if (activeVfo == 0)
  {
      vfoA = { static_cast<long>(freq), static_cast<int>(band), static_cast<bool>(usb) };
  }
  else
  {
      vfoB = { static_cast<long>(freq), static_cast<int>(band), static_cast<bool>(usb) };
  }
}

void RadioEngine::switchVfo(int target)
{
  if (target == activeVfo)
  {
      return;
  }
  saveActiveToVfo();
  activeVfo = target;
  VfoState& next = (activeVfo == 0) ? vfoA : vfoB;

  if (next.band != band)
  {
      selectBand(next.band);
  }

  freq = next.freq;
  usb = next.usb;

  updateLO();
  updateBFO();
  g_guiNeedsUpdate = true;
}

void RadioEngine::vfoCopy()
{
  if (activeVfo == 0)
  {
      vfoB = { static_cast<long>(freq), static_cast<int>(band), static_cast<bool>(usb) };
  }
  else
  {
      vfoA = { static_cast<long>(freq), static_cast<int>(band), static_cast<bool>(usb) };
  }
  g_guiNeedsUpdate = true;
  g_sync.vfoCopyFlash = true;
}

void RadioEngine::memStore(int ch)
{
  if (ch >= 0 && ch < NUM_MEM_CHANNELS)
  {
    memChannels[ch] = { static_cast<long>(freq), static_cast<int>(band), static_cast<bool>(usb), true };
    memRevision++;
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
  VfoState& m = memChannels[ch];
  if (!m.occupied) return; // Slot is empty — nothing to recall
  if (m.band != band)
  {
      selectBand(m.band);
  }
  freq = m.freq;
  usb = m.usb;
  updateLO();
  updateBFO();
  g_guiNeedsUpdate = true;
}

void RadioEngine::memDelete(int ch)
{
  if (ch >= 0 && ch < NUM_MEM_CHANNELS)
  {
    memChannels[ch].occupied = false;
    memRevision++;
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

        // Safety: If band name was changed via JSON, we need to handle the string memory
        const char* newName = obj["id"] | BANDS[i].name;
        if (strcmp(newName, BANDS[i].name) != 0) {
            BANDS[i].name = strdup(newName);
        }

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

void RadioEngine::setFrequency(long newFreq)
{
    // Auto-band switching: If target frequency is outside current band,
    // try to find the correct band in the BANDS table.
    if (!unlockedRange && (newFreq < BANDS[band].freqMin || newFreq > BANDS[band].freqMax))
    {
        for (int i = 0; i < NUM_BANDS; i++)
        {
            if (BANDS[i].enabled && newFreq >= BANDS[i].freqMin && newFreq <= BANDS[i].freqMax)
            {
                if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
                {
                    int oldBand = band;
                    bandFreqs[band] = freq; // Save current frequency to old band memory
                    band = i;
                    freq = newFreq;
                    usb = BANDS[i].sideBand; // Switch to band-standard sideband

                    updateBandRelaysInternal(oldBand, band);
                    lastRelayBand = band;
                    updateLOInternal();
                    updateBFOInternal();

                    xSemaphoreGiveRecursive(g_hwMutex);
                    g_guiNeedsUpdate = true;
                    settings.setUpdated();
                    return;
                }
            }
        }
    }

    // Normal tuning (within band or in Unlocked/GEN mode)
    freq = constrain(newFreq, getMinFreq(), getMaxFreq());
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        updateLOInternal();
        xSemaphoreGiveRecursive(g_hwMutex);
    }
    g_guiNeedsUpdate = true;
    settings.setUpdated();
}

long RadioEngine::getMinFreq() const
{
    return unlockedRange ? 10L : BANDS[band].freqMin;
}

long RadioEngine::getMaxFreq() const
{
    return unlockedRange ? 30000000L : BANDS[band].freqMax;
}

void RadioEngine::setRitEnabled(bool enabled)
{
    ritEnabled = enabled;
    updateLO();
    g_guiNeedsUpdate = true;
}

void RadioEngine::setRitOffset(long newOffset)
{
    ritOffset = newOffset;
    if (!g_tx)
    {
        updateLO();
    }
    g_guiNeedsUpdate = true;
}

void RadioEngine::setBfoUsb(double newBfo)
{
    bfoUsb = newBfo;
    updateBFO();
}

void RadioEngine::setBfoLsb(double newBfo)
{
    bfoLsb = newBfo;
    updateBFO();
}

void RadioEngine::setUsb(bool newUsb)
{
    usb = newUsb;
    updateBFO();
    updateLO();
    g_guiNeedsUpdate = true;
    settings.setUpdated();
}

void RadioEngine::setUtcOffset(int hours)
{
    utcOffset = hours;
    settings.setUpdated();
}

void RadioEngine::setDstActive(bool active)
{
    dstActive = active;
    settings.setUpdated();
}

void RadioEngine::loadFromPreferences()
{
  bfoUsb = preferences.getDouble("bfo_usb", 9001500.0);
  bfoLsb = preferences.getDouble("bfo_lsb", 8998500.0);
  stepIdx = preferences.getInt("stepIdx", 1);

  if (preferences.getBytes("vfoA", &vfoA, sizeof(VfoState)) == 0)
  {
      vfoA = { 7100000, 3, false };
  }
  if (preferences.getBytes("vfoB", &vfoB, sizeof(VfoState)) == 0)
  {
      vfoB = { 14200000, 2, true };
  }
  if (preferences.getBytes("mems", memChannels, sizeof(memChannels)) == 0)
  {
    for(int i = 0; i < NUM_MEM_CHANNELS; i++)
    {
        memChannels[i] = { 0L, 0, false, false }; // occupied=false = empty slot
    }
  }

  activeVfo = preferences.getInt("vfoActive", 0);
  utcOffset = preferences.getInt("utc_off", 1);
  dstActive = preferences.getBool("dst_act", true);
  VfoState& act = (activeVfo == 0) ? vfoA : vfoB;
  freq = act.freq;
  band = act.band;
  usb = act.usb;

  for (int i = 0; i < NUM_BANDS; i++)
  {
    char k[8];
    snprintf(k, 8, "f%d", i);
    bandFreqs[i] = preferences.getLong(k, BANDS[i].freqDefault);
  }
}

void RadioEngine::saveToPreferences()
{
  // Smart-Save: Only write if changed to save Flash cycles
  if (preferences.getDouble("bfo_usb", 0) != bfoUsb)
  {
      preferences.putDouble("bfo_usb", bfoUsb);
  }
  if (preferences.getDouble("bfo_lsb", 0) != bfoLsb)
  {
      preferences.putDouble("bfo_lsb", bfoLsb);
  }
  if (preferences.getInt("stepIdx", -1) != stepIdx)
  {
      preferences.putInt("stepIdx", stepIdx);
  }

  saveActiveToVfo();

  // Smart-Save for structs: compare before writing
  VfoState tmp;
  if (preferences.getBytes("vfoA", &tmp, sizeof(VfoState)) != sizeof(VfoState) || memcmp(&tmp, &vfoA, sizeof(VfoState)) != 0)
  {
    preferences.putBytes("vfoA", &vfoA, sizeof(VfoState));
  }
  if (preferences.getBytes("vfoB", &tmp, sizeof(VfoState)) != sizeof(VfoState) || memcmp(&tmp, &vfoB, sizeof(VfoState)) != 0)
  {
    preferences.putBytes("vfoB", &vfoB, sizeof(VfoState));
  }

  VfoState tmpMems[NUM_MEM_CHANNELS];
  if (preferences.getBytes("mems", tmpMems, sizeof(tmpMems)) != sizeof(tmpMems) || memcmp(tmpMems, memChannels, sizeof(tmpMems)) != 0)
  {
    preferences.putBytes("mems", memChannels, sizeof(memChannels));
  }

  if (preferences.getInt("vfoActive", -1) != activeVfo)
  {
      preferences.putInt("vfoActive", activeVfo);
  }

  if (preferences.getInt("utc_off", 99) != utcOffset)
  {
      preferences.putInt("utc_off", utcOffset);
  }
  if (preferences.getBool("dst_act", !dstActive) != dstActive)
  {
      preferences.putBool("dst_act", dstActive);
  }

  for (int i = 0; i < NUM_BANDS; i++)
  {
    char key[8];
    snprintf(key, 8, "f%d", i);
    if (preferences.getLong(key, 0) != bandFreqs[i])
    {
        preferences.putLong(key, bandFreqs[i]);
    }
  }
}
