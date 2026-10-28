# Chorus-R (antes Wide Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Chorus-R** (antes Wide Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Chorus-R** (formerly Wide Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Wide Ready abren igual con Chorus-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Wide Ready open with Chorus-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Wide Ready" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Wide Ready" copy so the DAW does not list the plugin twice.

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Tres modos sobre un mismo retardo fraccional (interpolación cúbica Hermite; test: un seno de 10 kHz leído con retardo fraccional queda puro, residuo < −80 dB):
  - **Chorus**: 1 a 3 voces con retardos base distintos, Rate, Depth, Delay (5-40 ms), Stereo spread; modo **Ensemble** (cada voz a su propia velocidad) para synths y pads.
  - **Flanger**: Rate, Depth, retardo manual (0.1-10 ms), Feedback positivo o negativo hasta 95 % con límite suave (test: estable), **Through-zero** (la señal original también se retrasa para que el barrido la cruce) e **Invertir** polaridad (test: con through-zero e inversión, Depth 0 cancela por completo).
  - **Micro-pitch**: dos voces desafinadas ±cents (izquierda sube, derecha baja) con dos cabezales cruzados en ventana de 24 ms: ancho sin vaivén audible (test: frecuencia medida ±0.5 Hz y nivel estable).
- Con Depth 0 la salida es la entrada con un retardo fijo exacto (test, bit a bit).
- Cambiar de modo, voces, ensemble, through-zero o polaridad hace un fundido de 10 ms de salida y 10 ms de entrada de las voces, y los cambios de tiempo/feedback saltan mientras están en silencio: sin clics ni "chirridos" (test).
- Mix 0 % deja la señal original idéntica en los tres modos (test). Sin latencia.
- Vista de las voces moviéndose sobre el eje de tiempo (L y R) y presets: Synth Chorus, Pad Ensemble, Voz ancha (micro-pitch) y Flanger Jet.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
