#include <M5Unified.h>
#include <Avatar.h>
#include "fft.hpp"
#include <cinttypes>
#if defined(ARDUINO_M5STACK_CORES3)
#include <gob_unifiedButton.hpp>
goblib::UnifiedButton unifiedButton;
#endif
#define USE_MIC

#ifdef USE_MIC
// ---------- Configuración de muestreo del micrófono ----------
#define READ_LEN (2 * 256)      // Longitud de lectura de audio
#define LIPSYNC_LEVEL_MAX 10.0f // Nivel máximo para la sincronización de labios

int16_t *adcBuffer = NULL;
static fft_t fft;
static constexpr size_t WAVE_SIZE = 256 * 2; // Tamaño del buffer de onda

static constexpr const size_t record_samplerate = 16000; // Frecuencia de muestreo
static int16_t *rec_data;

// Configuración de la sincronización de labios
uint8_t lipsync_shift_level = 11;      // Nivel de desplazamiento para la reducción de datos
float lipsync_max = LIPSYNC_LEVEL_MAX; // Nivel máximo para la apertura de la boca

#endif

using namespace m5avatar;

Avatar avatar;
ColorPalette *cps[6];
uint8_t palette_index = 0;

uint32_t last_rotation_msec = 0;
uint32_t last_lipsync_max_msec = 0;

void lipsync()
{
  size_t bytesread;
  uint64_t level = 0;

#ifndef SDL_h_
  // Captura de audio y procesamiento de FFT
  if (M5.Mic.record(rec_data, WAVE_SIZE, record_samplerate))
  {
    fft.exec(rec_data);
    for (size_t bx = 5; bx <= 60; ++bx)
    {
      int32_t f = fft.get(bx);
      level += abs(f);
    }
  }

  uint32_t temp_level = level >> lipsync_shift_level;
  float ratio = (float)(temp_level / lipsync_max);

  // Manejo del ratio y ajuste de la apertura de la boca
  if (ratio <= 0.01f)
  {
    ratio = 0.0f;
    if ((lgfx::v1::millis() - last_lipsync_max_msec) > 500)
    {
      last_lipsync_max_msec = lgfx::v1::millis();
      lipsync_max = LIPSYNC_LEVEL_MAX;
    }
  }
  else
  {
    if (ratio > 1.3f)
    {
      if (ratio > 1.5f)
      {
        lipsync_max += 10.0f;
      }
      ratio = 1.3f;
    }
    last_lipsync_max_msec = lgfx::v1::millis();
  }

  // Actualización de la rotación del avatar
  if ((lgfx::v1::millis() - last_rotation_msec) > 350)
  {
    int direction = random(-2, 2);
    avatar.setRotation(direction * 10 * ratio);
    last_rotation_msec = lgfx::v1::millis();
  }
#else
  float ratio = 0.0f;
#endif
  avatar.setMouthOpenRatio(ratio);
}

void setup()
{
  auto cfg = M5.config();
  cfg.internal_mic = true; // Activar micrófono interno
  M5.begin(cfg);

#if defined(ARDUINO_M5STACK_CORES3)
  unifiedButton.begin(&M5.Display, goblib::UnifiedButton::appearance_t::transparent_all);
#endif

  M5.Log.setLogLevel(m5::log_target_display, ESP_LOG_NONE);
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO);
  M5.Log.setEnableColor(m5::log_target_serial, false);
  M5_LOGI("Avatar Start");

  // Configuración inicial de posición, escala y rotación del avatar
  float scale = 0.0f;
  int8_t position_top = 0;
  int8_t position_left = 0;
  uint8_t display_rotation = 1; // Rotación inicial del display
  uint8_t first_cps = 0;

  // Configuración del micrófono según el tipo de placa
  auto mic_cfg = M5.Mic.config();
  switch (M5.getBoard())
  {
  case m5::board_t::board_M5AtomS3:
    first_cps = 4;
    scale = 0.55f;
    position_top = -60;
    position_left = -95;
    display_rotation = 2;
    mic_cfg.sample_rate = 16000;
    mic_cfg.pin_ws = 1;
    mic_cfg.pin_data_in = 2;
    M5.Mic.config(mic_cfg);
    break;
  case m5::board_t::board_M5StickC:
    first_cps = 1;
    scale = 0.6f;
    position_top = -80;
    position_left = -80;
    display_rotation = 3;
    break;
  // Otras configuraciones de placas...
  default:
    M5.Log.println("Invalid board.");
    break;
  }

#ifndef SDL_h_
  rec_data = (typeof(rec_data))heap_caps_malloc(WAVE_SIZE * sizeof(int16_t), MALLOC_CAP_8BIT);
  memset(rec_data, 0, WAVE_SIZE * sizeof(int16_t));
  M5.Mic.begin();
#endif

  M5.Speaker.end();
  M5.Display.setRotation(display_rotation);
  avatar.setScale(scale);
  avatar.setPosition(position_top, position_left);
  avatar.init(1); // Iniciar el dibujo del avatar

  // Configuración de paletas de colores para el avatar
  cps[0] = new ColorPalette();
  cps[0]->set(COLOR_PRIMARY, TFT_BLACK);
  cps[0]->set(COLOR_BACKGROUND, TFT_YELLOW);
  // Configuración de otras paletas...
  avatar.setColorPalette(*cps[first_cps]);
  last_rotation_msec = lgfx::v1::millis();
  M5_LOGI("setup end");
}

uint32_t count = 0;

void loop()
{
  M5.update(); // Actualizar estado de la placa

#if defined(ARDUINO_M5STACK_CORES3)
  unifiedButton.update(); // Actualizar botones unificados
#endif

  // Manejo de pulsaciones de botones
  if (M5.BtnA.wasPressed())
  {
    M5_LOGI("Push BtnA");
    palette_index++;
    if (palette_index > 5)
    {
      palette_index = 0;
    }
    avatar.setColorPalette(*cps[palette_index]);
  }
  if (M5.BtnA.wasDoubleClicked())
  {
    M5.Display.setRotation(3);
  }
  if (M5.BtnPWR.wasClicked())
  {
#ifdef ARDUINO
    esp_restart(); // Reiniciar la placa
#endif
  }

  // Actualización de la sincronización de labios
  lipsync();
  lgfx::v1::delay(1); // Pequeña pausa para estabilizar el bucle
}
