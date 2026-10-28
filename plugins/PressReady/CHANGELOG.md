# Compressor-R (antes Press Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Compressor-R** (antes Press Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Compressor-R** (formerly Press Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Press Ready abren igual con Compressor-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Press Ready open with Compressor-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Press Ready" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Press Ready" copy so the DAW does not list the plugin twice.

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
