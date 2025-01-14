// #include <Arduino.h>
//  Este código está extraído del siguiente repositorio (ejemplos/Advanced/Bluetooth_with_ESP32A2DP)
//  https://github.com/m5stack/M5Unified

#define FFT_SIZE 256 // Tamaño del FFT
class fft_t
{
  float _wr[FFT_SIZE + 1];    // Parte real del coeficiente
  float _wi[FFT_SIZE + 1];    // Parte imaginaria del coeficiente
  float _fr[FFT_SIZE + 1];    // Parte real del resultado FFT
  float _fi[FFT_SIZE + 1];    // Parte imaginaria del resultado FFT
  uint16_t _br[FFT_SIZE + 1]; // Tabla de bits reversos para reordenar
  size_t _ie;                 // Número de etapas en el algoritmo FFT

public:
  fft_t(void)
  {
#ifndef M_PI
#define M_PI 3.141592653 // Definición de PI si no está definida
#endif
    _ie = logf((float)FFT_SIZE) / log(2.0) + 0.5;          // Calcula el número de etapas del FFT
    static constexpr float omega = 2.0f * M_PI / FFT_SIZE; // Frecuencia angular
    static constexpr int s4 = FFT_SIZE / 4;
    static constexpr int s2 = FFT_SIZE / 2;

    // Precalcula los coeficientes de seno y coseno necesarios
    for (int i = 1; i < s4; ++i)
    {
      float f = cosf(omega * i);
      _wi[s4 + i] = f;
      _wi[s4 - i] = f;
      _wr[i] = f;
      _wr[s2 - i] = -f;
    }
    _wi[s4] = _wr[0] = 1; // Configura valores para 0 y s4

    // Configura la tabla de bits reversos
    size_t je = 1;
    _br[0] = 0;
    _br[1] = FFT_SIZE / 2;
    for (size_t i = 0; i < _ie - 1; ++i)
    {
      _br[je << 1] = _br[je] >> 1;
      je = je << 1;
      for (size_t j = 1; j < je; ++j)
      {
        _br[je + j] = _br[je] + _br[j];
      }
    }
  }

  // Ejecuta el FFT en la entrada dada
  void exec(const int16_t *in)
  {
    memset(_fi, 0, sizeof(_fi)); // Inicializa la parte imaginaria a cero

    // Realiza la ventana de Hann y convierte de estéreo a mono
    for (size_t j = 0; j < FFT_SIZE / 2; ++j)
    {
      float basej = 0.25 * (1.0 - _wr[j]);
      size_t r = FFT_SIZE - j - 1;

      _fr[_br[j]] = basej * (in[j * 2] + in[j * 2 + 1]);
      _fr[_br[r]] = basej * (in[r * 2] + in[r * 2 + 1]);
    }

    // Realiza el algoritmo FFT
    size_t s = 1;
    size_t i = 0;
    do
    {
      size_t ke = s;
      s <<= 1;
      size_t je = FFT_SIZE / s;
      size_t j = 0;
      do
      {
        size_t k = 0;
        do
        {
          size_t l = s * j + k;
          size_t m = ke * (2 * j + 1) + k;
          size_t p = je * k;
          float Wxmr = _fr[m] * _wr[p] + _fi[m] * _wi[p];
          float Wxmi = _fi[m] * _wr[p] - _fr[m] * _wi[p];
          _fr[m] = _fr[l] - Wxmr;
          _fi[m] = _fi[l] - Wxmi;
          _fr[l] += Wxmr;
          _fi[l] += Wxmi;
        } while (++k < ke);
      } while (++j < je);
    } while (++i < _ie);
  }

  // Devuelve la magnitud del componente de frecuencia en el índice dado
  uint32_t get(size_t index)
  {
    return (index < FFT_SIZE / 2) ? (uint32_t)sqrtf(_fr[index] * _fr[index] + _fi[index] * _fi[index]) : 0u;
  }
};
