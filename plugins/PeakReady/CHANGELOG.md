# Limiter-R (antes Peak Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Limiter-R** (antes Peak Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Limiter-R** (formerly Peak Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Peak Ready abren igual con Limiter-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Peak Ready open with Limiter-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Peak Ready" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Peak Ready" copy so the DAW does not list the plugin twice.

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
