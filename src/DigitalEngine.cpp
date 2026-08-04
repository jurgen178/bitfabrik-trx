#include "DigitalEngine.h"
#include "Hardware.h"
#include "RadioEngine.h"

DigitalEngine digital;

/**
 * ── DIGITAL MODULATION ENGINE ─────────────────────────────────────────────
 * Implements low-level timing for CW (Morse) and RTTY.
 * Direct DDS modulation is used to achieve clean FSK/Keying.
 * ──────────────────────────────────────────────────────────────────────────
 */

const char* const MORSE_TABLE[] = {
  ".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....", "..",   ".---", // A-J
  "-.-",  ".-..", "--",   "-.",   "---",  ".--.", "--.-", ".-.",  "...",  "-",    // K-T
  "..-",  "...-", ".--",  "-..-", "-.--", "--..",                                 // U-Z
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----." // 0-9
};

const uint8_t BAU_LTRS[] = { 0x18, 0x13, 0x0E, 0x12, 0x10, 0x16, 0x0B, 0x05, 0x0C, 0x1A, 0x1E, 0x09, 0x07, 0x06, 0x03, 0x0D, 0x1D, 0x0A, 0x14, 0x01, 0x1C, 0x0F, 0x19, 0x17, 0x15, 0x11 };
const uint8_t BAU_FIGS[] = { 0x18, 0x13, 0x0E, 0x12, 0x10, 0x16, 0x0B, 0x05, 0x0C, 0x1A, 0x1E, 0x09, 0x07, 0x06, 0x03, 0x0D, 0x1D, 0x0A, 0x14, 0x01, 0x1C, 0x0F, 0x19, 0x17, 0x15, 0x11 };

// Character mapping for RTTY figures mode
const char FIGS_MAP[] = "-? :$3!&#8'() .,9014'57;2/6\"";

/**
 * Sets the VFO frequency based on Mark/Space state.
 */
void setRttyFreq(bool mark)
{
  double targetFreq = (double)radio.getFrequency() + ZF_FREQ;
  if (!mark)
  {
      targetFreq -= RTTY_SHIFT;
  }
  dds_setFreq(targetFreq, LO_FQUD);
}

void sendMorseChar(char c)
{
  if (c >= 'a' && c <= 'z')
  {
      c -= 32;
  }
  if (c == ' ')
  {
      vTaskDelay(pdMS_TO_TICKS(400));
      return;
  }

  const char* pattern = nullptr;
  if (c >= 'A' && c <= 'Z')
  {
      pattern = MORSE_TABLE[c - 'A'];
  }
  else if (c >= '0' && c <= '9')
  {
      pattern = MORSE_TABLE[c - '0' + 26];
  }

  if (!pattern)
  {
      return;
  }

  for (int i = 0; i < strlen(pattern); i++)
  {
    digital.setKeyed(true);
    vTaskDelay(pdMS_TO_TICKS(pattern[i] == '.' ? 100 : 300));
    digital.setKeyed(false);
    vTaskDelay(pdMS_TO_TICKS(100)); // inter-element gap
  }
  vTaskDelay(pdMS_TO_TICKS(200)); // inter-character gap
}

void sendRttyBit(bool mark)
{
  digital.setKeyed(!mark); // space = keying active for status display
  setRttyFreq(mark);
  vTaskDelay(pdMS_TO_TICKS(RTTY_BIT_MS));
}

void sendRttyByte(uint8_t data)
{
  sendRttyBit(false); // Start bit (Space)
  for (int i=0; i<5; i++)
  {
      sendRttyBit((data >> i) & 1);
  }
  sendRttyBit(true);  // Stop bit (Mark)
  vTaskDelay(pdMS_TO_TICKS(RTTY_BIT_MS / 2));
}

void sendRttyChar(char c)
{
  if (c >= 'a' && c <= 'z')
  {
      c -= 32;
  }
  int idx = -1;
  bool targetIsFigs = false;

  // Find character in letters map
  for (int i=0; i<26; i++)
  {
    if (c == (char)('A' + i))
    {
        idx = i;
        targetIsFigs = false;
        break;
    }
  }
  // Find character in figures map
  if (idx == -1)
  {
    const char* p = strchr(FIGS_MAP, c);
    if (p)
    {
        idx = p - FIGS_MAP;
        targetIsFigs = true;
    }
  }
  if (idx == -1 && c == ' ')
  {
      idx = 0x04;
  }
  if (idx == -1)
  {
      return;
  }

  if (targetIsFigs != digital.isRttyFigs() && c != ' ')
  {
    sendRttyByte(targetIsFigs ? 27 : 31); // ITA2 FIGS or LTRS shift
    digital.setRttyFigs(targetIsFigs);
  }

  uint8_t data = (c == ' ') ? 0x04 : (targetIsFigs ? BAU_FIGS[idx] : BAU_LTRS[idx]);
  sendRttyByte(data);
}


/**
 * ── DIGITAL ENGINE TASK (Core 0 Background) ───────────────────────────────
 * Consumes the transmission queue. Runs on Core 0 to avoid timing
 * interference from UI/Hardware interrupts on Core 1.
 * ──────────────────────────────────────────────────────────────────────────
 */
void TaskDigital(void *p)
{
  for (;;)
  {
    char c;
    if (!digital.isBusy() && !digital.queue.empty())
    {
      digital.setBusy(true);

      if (digital.getMode() == 0)
      { // Morse (CW)
        while (digital.queue.pop(c))
        {
            sendMorseChar(c);
        }
      }
      else
      { // RTTY
        digital.setRttyFigs(false);
        sendRttyByte(31); // ITA2 LTRS init
        while (digital.queue.pop(c))
        {
            sendRttyChar(c);
        }
        sendRttyByte(31);
        radio.updateLO(); // Restore carrier after FSK
      }

      digital.setBusy(false);
      digital.setKeyed(false);
      notifyWebUpdate();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
