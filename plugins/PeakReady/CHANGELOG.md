# Limiter-R (antes Peak Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Limiter-R** (antes Peak Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Limiter-R** (formerly Peak Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Peak Ready deberían abrir con Limiter-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Peak Ready should open with Limiter-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Peak Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Peak Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).
- Corrección: el ataque bajaba la ganancia en un escalón `lookahead` muestras antes del pico en vez de en rampa (el mínimo deslizante anulaba la media móvil). Ahora la rampa es suave en forma de S (dos medias móviles en cascada) y llega exacta al pico: 12 dB menos distorsión en Transparente y 9 dB en Punchy con bombo + tono (test nuevo). El techo true-peak se sigue cumpliendo (tests). / Fix: the attack stepped the gain down `lookahead` samples before the peak instead of ramping. The ramp is now a smooth S-curve that still lands exactly on the peak: 12 dB less distortion in Transparent, 9 dB in Punchy.
- Menos CPU: detección true-peak ~2.8x más rápida (1.9 % → 0.7 % de un núcleo). / Less CPU: ~2.8x faster true-peak detection.

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Limitador **true-peak** con lookahead: la detección corre con sobremuestreo 4x y la ganancia baja en rampa antes de que llegue el pico, así que el pico real de salida (medido a 4x) nunca pasa del techo (test: ruido a +12 dB, impulsos y seno a 0 dBFS, los tres estilos: ≤ techo + 0.05 dB).
- Por debajo del techo la salida es idéntica a la entrada (bit a bit, solo retrasada por la latencia reportada; test).
- **Release** manual (10–1000 ms) o **Auto**: adapta el tiempo a cuánto se está limitando (picos aislados vuelven rápido; limitación sostenida suelta más lento para no distorsionar los graves).
- Tres estilos: **Transparente** (lookahead 4 ms), **Punchy** (1.5 ms) y **Agresivo** (0.5 ms + clipper suave antes del limitador, para EDM fuerte).
- Medidor de volumen **LUFS** (ITU-R BS.1770): momentáneo, corto plazo e integrado con gating, con **objetivo** (−24 a −4 LUFS) y cuánto falta para llegar (test: seno de 997 Hz a −20 dBFS estéreo = −20.0 LUFS ±0.1).
- Un solo detector para los dos canales: la imagen estéreo no se mueve.
- Presets: Streaming −14, Club −8, EDM −6.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
