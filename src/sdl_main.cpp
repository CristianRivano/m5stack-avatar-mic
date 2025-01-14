#include <M5GFX.h>  // Incluye la librería M5GFX para manejo gráfico
#if defined(SDL_h_) // Comprueba si SDL (Simple DirectMedia Layer) está definida

void setup(void); // Declaración de la función setup
void loop(void);  // Declaración de la función loop

// Función débil que se puede sobrescribir
__attribute__((weak)) int user_func(bool *running)
{
  setup(); // Llama a la función setup una vez
  do
  {
    loop(); // Llama repetidamente a la función loop
  } while (*running); // Continúa mientras el puntero running apunte a true
  return 0; // Devuelve 0 al finalizar
}

// Función principal de la aplicación
int main(int, char **)
{
  // El segundo argumento es efectivo para la ejecución paso a paso con puntos de interrupción.
  // Puedes especificar el tiempo en milisegundos para realizar una ejecución lenta que asegure las actualizaciones de pantalla.
  return lgfx::Panel_sdl::main(user_func, 128); // Llama a la función main de Panel_sdl con user_func y un tiempo de 128 ms
}

#endif
