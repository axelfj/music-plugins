# Tune-R (antes Tune Ready)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.3

- Corrección: al cambiar el tono, cada grano se leía corrido por la fracción de muestra hacia el lado contrario, y las marcas de análisis caían en muestras enteras. En voces reales (cuyo período nunca es un número entero de muestras) eso hacía vibrar el tono y sonaba áspero. Ahora las marcas son fraccionarias y siguen el período detectado: el ruido de una quinta hacia arriba baja de −8 dB a −36 dB (diente de sierra a 220 Hz). / Fix: grains read the fractional offset with the wrong sign and analysis marks sat on whole samples, which jittered the shifted pitch on real voices; marks are now fractional and follow the detected period (noise on a fifth-up shift: −8 dB → −36 dB).
- Menos CPU: detección de tono ~2.6x más rápida (4.8 % → 1.8 % de un núcleo): FFT nueva, la mitad de tamaño (la misma correlación exacta) y las dos señales en una sola FFT. / Less CPU: ~2.6x faster pitch detection, same result.

## 1.0.0-alpha.2

- Nuevo nombre: **Tune-R** (antes Tune Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Tune-R** (formerly Tune Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Tune Ready deberían abrir con Tune-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Tune Ready should open with Tune-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Tune Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Tune Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Corrección de afinación en tiempo real para voces (monofónico): detección YIN por FFT y re-síntesis TD-PSOLA con marcas alineadas a los pulsos, que conserva los formantes (la voz no suena a ardilla).
- Un seno 30 cents desafinado queda a ±3 cents de la nota con velocidad 0 (test). Afinado también en tonalidad / escala (mayor, menor, cromática o personalizada con 12 botones) y por MIDI: las notas que mantengas pulsadas mandan (test).
- Velocidad de corrección de 0 ms (robot / T-Pain) a 400 ms (natural; test: llega gradualmente), Cantidad y Humanize (deriva aleatoria en cents).
- Modo Fijo: transpone ±24 semitonos y mueve los formantes ±12 semitonos de forma independiente (voz grave, aguda, robot). Test: +7 st de tono deja el centroide espectral en su sitio; +12 st de formante lo duplica sin cambiar la nota.
- Sin tono (silencio, consonantes) la señal pasa idéntica salvo la latencia (test). Latencia reportada al DAW: 30 ms en el rango de baja latencia (110 Hz) y 60 ms en voz grave (50 Hz); el test comprueba que coincide con el retardo medido.
- Vista de tono tipo piano-roll con la nota detectada y la corregida, y las notas de la escala resaltadas.
- Presets: Natural, Duro (T-Pain), Voz grave, Voz aguda.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).

Pendiente para 1.0.x: vibrato controlado, detección de pitch por canal, modo de baja latencia con ventana más corta, pruebas con voces reales (formantes a oído).
