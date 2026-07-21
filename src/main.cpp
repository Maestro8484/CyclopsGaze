#include <Arduino.h>
#include "config.h"
#include "sensors/SEN0626Sensor.h"

// Transport is a config.h knob (CG-S13) -- live IRIS had to move the sensor to
// Serial4 because pin 0 is its left-eye CS. See config.h SEN0626_SERIAL.
SEN0626Sensor sensor(SEN0626_SERIAL);

// ─────────────────────────────────────────────────────────────────────────────
// Runtime-tunable gaze config — the PS_CFG: serial protocol (CG-S13)
// ─────────────────────────────────────────────────────────────────────────────
//
// Ported from IRIS, where this is the mechanism that actually made the SEN0626
// swap tunable: S141 introduced PS_CFG, S212c added the per-axis gain/bias keys
// and the false-ack guard. Every knob below is settable live over the USB serial
// link with NO reflash -- which is exactly what the still-outstanding CG-S12
// re-bench needs (raw-score gate + per-axis gain/bias are UNVERIFIED standalone;
// docs/BENCH_PROTOCOL.md). Edit-reflash-repeat per tuning step was the old cost.
//
// Variable names are IRIS's VERBATIM so `diff` against IRIS src/main.cpp stays
// meaningful -- this repo exists to keep the two from drifting.
//
// Seeded from the config.h defaults. A board reset reverts to them: unlike IRIS
// there is no persistence here (IRIS's Pi4 saves ps_config.json and re-pushes it
// on serial open). Once the bench proves a value, write it back into config.h.
static uint8_t  psConfGate       = PS_CONF_GATE_DEFAULT;        // PS_CFG:CONF=n
static bool     psFacingRequired = PS_FACING_REQUIRED_DEFAULT;  // PS_CFG:FACING=0/1
static uint32_t psLostMs         = PS_LOST_MS_DEFAULT;          // PS_CFG:LOST_MS=n
static float    psXGain          = GAZE_X_GAIN_DEFAULT;         // PS_CFG:X_GAIN=f
static float    psYGain          = GAZE_Y_GAIN_DEFAULT;         // PS_CFG:Y_GAIN=f
static float    psXBias          = GAZE_X_BIAS_DEFAULT;         // PS_CFG:X_BIAS=f
static float    psYBias          = GAZE_Y_BIAS_DEFAULT;         // PS_CFG:Y_BIAS=f
static bool     psLedEnabled     = false;                       // PS_CFG:LED=0/1

static constexpr uint32_t SERIAL_BUF_SIZE = 40;  // matches IRIS; longest PS_CFG line is ~18 chars
static char    serialBuf[SERIAL_BUF_SIZE];
static uint8_t serialBufLen = 0;

// Line-oriented command reader, structurally identical to IRIS's processSerial().
static void processSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBufLen > 0) {
        serialBuf[serialBufLen] = '\0';
        serialBufLen = 0;

        if (strncmp(serialBuf, "PS_CFG:", 7) == 0) {
          // IRIS-verbatim parser (S141 + S212c). Form: PS_CFG:KEY=value
          char* eq = strchr(serialBuf + 7, '=');
          if (eq) {
            *eq = '\0';
            const char* key = serialBuf + 7;
            float val = atof(eq + 1);
            bool  known = true;   // S212c: gates the ack below to implemented keys only
            if      (strcmp(key, "CONF")    == 0) psConfGate       = (uint8_t)constrain((int)val, 0, 100);
            else if (strcmp(key, "FACING")  == 0) psFacingRequired = (val != 0.0f);
            else if (strcmp(key, "LOST_MS") == 0) psLostMs         = (uint32_t)val;
            else if (strcmp(key, "Y_BIAS")  == 0) psYBias          = val;
            else if (strcmp(key, "X_GAIN")  == 0) psXGain          = val;
            else if (strcmp(key, "Y_GAIN")  == 0) psYGain          = val;
            else if (strcmp(key, "X_BIAS")  == 0) psXBias          = val;
            else if (strcmp(key, "LED")     == 0) { psLedEnabled = (val != 0.0f);
                                                    // No-op on SEN0626 -- no LED register exists
                                                    // (confirmed vs DFRobot's Modbus map + library,
                                                    // CG-S8). Called anyway so the surface matches.
                                                    sensor.enableLED(psLedEnabled); }
            else                                    known = false;
            // S212c: only ack keys this firmware ACTUALLY implements. IRIS's ack
            // print used to sit after the chain with no else, so an unimplemented
            // key acked identically to a real one -- S212b cheerfully echoed
            // "PS_CFG X_GAIN=1.0" while having no psXGain at all, a false
            // confirmation that also defeated the Pi4's config-drift check.
            //
            // The "[DBG] PS_CFG k=v" wording is IRIS's wire format kept VERBATIM
            // (not this repo's "[CG]" prefix) so IRIS's own tooling -- iris_post.py
            // scrapes the ack regex '\[DBG\] PS_CFG (\w+)=(\S+)' -- can drive a
            // CyclopsGaze board unmodified. The UNKNOWN line is deliberately
            // worded so it does NOT match that regex.
            if (known) {
              Serial.print("[DBG] PS_CFG ");
              Serial.print(key); Serial.print("="); Serial.println(eq + 1);
            } else {
              Serial.print("[DBG] PS_CFG UNKNOWN key "); Serial.println(key);
            }
          }

        } else if (strcmp(serialBuf, "PS_CFG?") == 0) {
          // CyclopsGaze-only readback. IRIS has no equivalent because its Pi4
          // holds the authoritative copy in ps_config.json and the WebUI renders
          // it; a standalone board has no such store, so without this there is no
          // way to read the live values back mid-bench. Single line, deliberately
          // NOT in the "[DBG] PS_CFG k=v" ack shape so it can never be mistaken
          // for an ack by IRIS tooling.
          Serial.printf("[CG] PS_CFG_DUMP CONF=%u FACING=%d LOST_MS=%lu "
                        "X_GAIN=%.2f Y_GAIN=%.2f X_BIAS=%.2f Y_BIAS=%.2f LED=%d\n",
                        psConfGate, psFacingRequired ? 1 : 0, (unsigned long)psLostMs,
                        psXGain, psYGain, psXBias, psYBias, psLedEnabled ? 1 : 0);

        } else if (strcmp(serialBuf, "VERSION") == 0) {
          Serial.printf("[CG] CyclopsGaze %s\n", FIRMWARE_VERSION);
        }
      }
    } else {
      if (serialBufLen < SERIAL_BUF_SIZE - 1) {
        serialBuf[serialBufLen++] = c;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t deadline = millis() + 2000;
  while (!Serial && millis() < deadline) {}
  Serial.printf("[CG] CyclopsGaze %s\n", FIRMWARE_VERSION);

  sensor.begin();

  randomSeed(analogRead(A0));
  initEyes(true, true, true);
  eyes->setTargetPupil(0.40f, 300);
}

void loop() {
  processSerial();

  bool sampled = sensor.read();

  // Gate: facing (inert on SEN0626 -- the shim hardcodes is_facing=1) AND
  // confidence. Both knobs are live-tunable via PS_CFG. box_confidence is the RAW
  // DFRobot score (0-100) since CG-S12, and psConfGate defaults to DFRobot's own
  // documented validity floor of 60 (wiki.dfrobot.com/sen0626/docs/23024) -- the
  // same effective threshold as the pre-CG-S12 152/255, in the vendor's native
  // scale. Tune against real bench scores only (audit 3.7).
  bool anyFace = sampled && sensor.numFacesFound() > 0;
  person_sensor_face_t f = anyFace ? sensor.faceDetails(0) : person_sensor_face_t{};
  bool haveFace = anyFace && (!psFacingRequired || f.is_facing) && f.box_confidence > psConfGate;

#if CG_CALIB_RAW
  // Bench range/angle tuning (S4): log EVERY raw detection the sensor reports,
  // gate-passed or not. Logging only inside the gated branch (CG-S3) meant the
  // line vanished the instant a detection dropped below the gate -- exactly the
  // boundary data needed to tell "gate too strict" from "sensor's own detection
  // envelope ended" apart. gate=PASS/REJECT makes that call visible without
  // touching the eye (untracked detections never call setTargetPosition).
  if (anyFace) {
    Serial.printf("[CG] faces=%d rawScore=%u conf=%d gate=%s | rawX=%u rawY=%u\n",
                  sensor.numFacesFound(), sensor.rawScore(), f.box_confidence,
                  haveFace ? "PASS" : "REJECT", sensor.rawFaceX(), sensor.rawFaceY());
  }
#endif

  if (haveFace) {
    // Gaze shaping: targetN = rawN * gain + bias per axis (IRIS S212c model,
    // adopted CG-S12; ⚠ still UNVERIFIED standalone -- re-bench is priority #1).
    // The gain's SIGN is direction and its MAGNITUDE is range, so one knob covers
    // both "it's mirrored" and "it barely moves". setTargetPosition clamps to the
    // unit circle, so |gain|>1 saturates gracefully (audit 3.8).
    //
    // CG-S13: the box-derived center form below is now IRIS's VERBATIM expression
    // rather than reading box_left/box_top directly. Identical result here -- the
    // SEN0626 shim stores a center-only box (box_left==box_right, box_top==
    // box_bottom), so both delta terms are 0 and this collapses to the exact
    // center -- but writing it IRIS's way keeps the two files diffable and keeps
    // the (box_bottom-box_top)/3 "aim a third down the box = eye level" term that
    // a real bounding-box sensor would need. Zero behavior change; verified by
    // algebra, not by bench (both reduce to cx and cy exactly).
    float rawX = (static_cast<float>(f.box_left) + static_cast<float>(f.box_right - f.box_left) / 2.0f) / 127.5f - 1.0f;
    float rawY = (static_cast<float>(f.box_top)  + static_cast<float>(f.box_bottom - f.box_top) / 3.0f) / 127.5f - 1.0f;
    float targetX = rawX * psXGain + psXBias;
    float targetY = rawY * psYGain + psYBias;

    eyes->setAutoMove(false);
    eyes->setTargetPosition(targetX, targetY);

#if CG_CALIB_RAW
    // Prints the SAME variables the eye was just driven with -- IRIS's S212c
    // debug block had to duplicate the gain/bias math and carries a warning that
    // the readout lies if the two drift. Sharing the variables removes that risk.
    Serial.printf("[CG]   -> raw=%.2f,%.2f  target=%.2f,%.2f  (gain %.2f/%.2f bias %.2f/%.2f)\n",
                  rawX, rawY, targetX, targetY, psXGain, psYGain, psXBias, psYBias);
#endif

  } else if (sensor.timeSinceFaceDetectedMs() > psLostMs && !eyes->autoMoveEnabled()) {
    // Runs whether read() returned false (throttle / comms failure) or returned
    // true with no qualifying face, so autoMove always resumes and never sticks
    // OFF when the sensor stops responding (audit 3.2). The autoMoveEnabled()
    // guard is IRIS's -- it just skips redundant re-enables once wandering.
    eyes->setAutoMove(true);
  }

  eyes->renderFrame();
}
