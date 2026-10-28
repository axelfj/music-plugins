# Clipper-R (antes Clip Ready)

## 1.0.0-alpha.3

- Nuevo nombre: **Clipper-R** (antes Clip Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Clipper-R** (formerly Clip Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Clip Ready abren igual con Clipper-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Clip Ready open with Clipper-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Clip Ready" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Clip Ready" copy so the DAW does not list the plugin twice.

## 1.0.0-alpha.2

- Techo estricto ahora limita la salida final: antes solo limitaba la señal recortada, así que con Mix < 100 % o Salida > 0 dB el pico podía pasar del techo (#34). DELTA sigue sin límite, es un modo de monitoreo.
- Los instaladores y el release llevan la versión completa (`ClipReady-Setup-1.0.0-alpha.2.exe`, etc.); antes decían `1.0.0` (#35).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Clipper suave con sobremuestreo 1x/2x/4x/8x (filtros de fase lineal, aliasing bajo -100 dBFS a 8x).
- Controles: Entrada, Techo, Suavidad, Salida, Mix.
- LINK (compara a igual volumen), DELTA (escucha solo lo que se recorta), TECHO ESTRICTO.
- Medidores de entrada, salida y cuánto se recorta; curva de transferencia en vivo.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
