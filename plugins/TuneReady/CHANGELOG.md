# Tune-R (antes Tune Ready)

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
