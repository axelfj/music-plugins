# Phaser-R (antes Phase Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Phaser-R** (antes Phase Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Phaser-R** (formerly Phase Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Phase Ready abren igual con Phaser-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Phase Ready open with Phaser-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Phase Ready" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Phase Ready" copy so the DAW does not list the plugin twice.

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Phaser clásico de 4 a 12 etapas all-pass (de primer orden) que se abren en abanico alrededor de la frecuencia central.
- LFO libre (0.01 a 10 Hz) o sincronizado al tempo del DAW (de 4 compases a 1/16, con punteados y tresillos); en sync la fase sigue la posición del host al sample (test: se repite exacto cada ciclo).
- Formas de LFO: seno, triángulo y random suave (interpolación cúbica, sin saltos; test).
- Depth (barrido de ±2.5 octavas), Feedback positivo o negativo hasta 95 % con límite suave (test: estable con 12 etapas y profundidad máxima), Center, Stereo spread (hasta 180° entre L y R; test) y Mix.
- Cambiar el número de etapas hace un crossfade de 20 ms, sin clics (test).
- Con Depth 0 y Feedback 0, Mix 100 % deja la magnitud plana ±0.1 dB (test). Mix 0 % deja la señal original idéntica (test).
- Vista de la respuesta en frecuencia en tiempo real (muescas moviéndose) y del LFO.
- Presets: Guitarra clásica, Pad lento y Synth 1/4 (sync).
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
