# Chorus-R (antes Wide Ready)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.3

- Menos CPU: las líneas de delay usan índices sin división (1.0 % → 0.8 % de un núcleo). / Less CPU: delay lines index without divisions.

## 1.0.0-alpha.2

- Nuevo nombre: **Chorus-R** (antes Wide Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Chorus-R** (formerly Wide Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Wide Ready deberían abrir con Chorus-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Wide Ready should open with Chorus-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Wide Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Wide Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

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
