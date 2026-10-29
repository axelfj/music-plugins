# Compressor-R (antes Press Ready)

## 1.0.0-alpha.3

- Bajos sin distorsión con release rápido: el detector de picos sostenía cada pico solo 2 ms, así que con un release corto la ganancia subía y bajaba dentro de cada ciclo de un bajo (−34 dB de THD a 50 Hz con release de 30 ms). Ahora sostiene hasta 10 ms (nunca más de un tercio del release) y lo descuenta del release, así que la perilla sigue marcando el tiempo real: −115 dB de THD. / Clean bass with fast release: the 2 ms peak hold let the gain ride each cycle of a bass note (−34 dB THD at 50 Hz, 30 ms release); it now holds up to 10 ms, taken out of the release so the knob stays accurate (−115 dB).

## 1.0.0-alpha.2

- Nuevo nombre: **Compressor-R** (antes Press Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Compressor-R** (formerly Press Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Press Ready deberían abrir con Compressor-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Press Ready should open with Compressor-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Press Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Press Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Compresor feed-forward con detección de pico (con retención de 2 ms) o RMS, estéreo enlazado.
- Threshold, Ratio 1:1 a ∞:1 (modo limiter), Attack 0.05–300 ms, Release 5–3000 ms con Auto (mezcla de una etapa rápida y una lenta), Knee 0–24 dB, Makeup con Auto Gain, Mix para compresión paralela.
- Los tiempos de ataque y release se aplican sobre la reducción de ganancia, así que miden lo que dice la perilla (test: ±10 %).
- Filtro paso-alto del sidechain (para que el bombo no bombee) y sidechain externo.
- Tres caracteres: **Clean** (transparente), **Punch** (estilo FET: pico, release más corto, saturación al comprimir) y **Glue** (estilo bus: RMS, ataque más lento, knee ancho, calidez leve). Cambiar de carácter no produce clics.
- Lookahead 0–10 ms (reporta la latencia al DAW).
- Medidor de reducción grande, curva de transferencia con el punto de trabajo en vivo, medidores de entrada y salida.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
