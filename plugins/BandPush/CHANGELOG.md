# Multiband-R (antes Band Push)

## 1.0.0-alpha.2

- Nuevo nombre: **Multiband-R** (antes Band Push). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Multiband-R** (formerly Band Push): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Band Push deberían abrir con Multiband-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Band Push should open with Multiband-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Band Push" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Band Push" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Compresor multibanda de 3 bandas con compresión hacia arriba y hacia abajo por banda (el sonido "OTT").
- Una perilla principal, **Profundidad**, que escala toda la acción dinámica; **Tiempo** escala ataque (15 ms) y release (150 ms) de 0.1x a 10x; **Umbral** global.
- Por banda: Arriba (%), Abajo (%) y Ganancia; medidor de ganancia con signo por banda.
- Crossovers Linkwitz-Riley de 4.º orden ajustables (graves/medios y medios/agudos) con compensación de fase: las bandas suman plano (test: < 0.05 dB en todo el espectro).
- Detector de pico con retención adaptada a cada banda (20 / 5 / 2 ms) para que los graves no bombeen entre picos.
- La subida de las partes suaves se limita a +24 dB para no levantar el ruido de fondo.
- Entrada, Salida y Mix (la señal "seca" del mix es la suma de bandas, así que no hay cancelaciones de fase).
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
