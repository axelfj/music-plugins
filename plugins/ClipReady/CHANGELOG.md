# Clipper-R (antes Clip Ready)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.3

- Nuevo nombre: **Clipper-R** (antes Clip Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Clipper-R** (formerly Clip Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Clip Ready deberían abrir con Clipper-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Clip Ready should open with Clipper-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Clip Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Clip Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).
- Menos CPU: el sobremuestreo usa la simetría de sus filtros y sumas en paralelo (mismo filtro, mismo sonido): ~2.8x menos CPU a 4x (4.4 % → 1.6 % de un núcleo, estéreo a 48 kHz). / Less CPU: the oversampler exploits its filters' symmetry and parallel sums (same filter, same sound): ~2.8x less CPU at 4x.
- Nuevo botón **HQ**: antialiasing por antiderivada (ADAA) sobre el sobremuestreo, con un filtro que lo deja plano hasta 20 kHz (±0.015 dB) y alineado con la señal dry. Recortando duro, HQ quita 16-23 dB de aliasing: 2x HQ (−54 dBFS) queda más limpio que 4x normal (−51) y usa menos CPU (1.2 % vs 1.5 %). El techo estricto se sigue cumpliendo (tests). Suma 1 muestra de latencia (2 a 2x), que se reporta al DAW. / New **HQ** button: antiderivative antialiasing (ADAA) on top of oversampling, flat to 20 kHz and aligned with the dry path. Hard clipping loses 16-23 dB of aliasing; 2x HQ is cleaner than plain 4x and cheaper. Adds 1 sample of latency (2 at 2x), reported to the host.

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
