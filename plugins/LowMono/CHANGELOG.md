# Stereo-R (antes Low Mono)

## 1.0.0-alpha.2

- Nuevo nombre: **Stereo-R** (antes Low Mono). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Stereo-R** (formerly Low Mono): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Low Mono abren igual con Stereo-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Low Mono open with Stereo-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Low Mono" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Low Mono" copy so the DAW does not list the plugin twice.

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Imager estéreo: Ancho 0-200 % (mid/side) y **mono por debajo** de una frecuencia (20-500 Hz) con crossover Linkwitz-Riley de 24 dB/oct o 48 dB/oct; las bandas suman con magnitud plana (test: ±0.1 dB con el crossover activo, incluso en la frecuencia de corte).
- Debajo del corte L−R desaparece: con 48 dB/oct queda bajo −80 dB dos octavas por debajo (test); con 24 dB/oct bajo −40 dB.
- Ancho graves: cuánto estéreo dejar en los graves (0 % = mono total).
- Con Ancho 100 % y el crossover apagado la salida es idéntica a la entrada, bit a bit (test). Ancho 0 % = mono exacto; 200 % = lados +6.02 dB (test).
- Goniómetro (mono vertical) y medidor de correlación con retención del peor valor (test: +1 mono, −1 invertido, ~0 ruido independiente) y aviso cuando la mezcla se cancelaría en mono.
- Presets: Master seguro, Club, Ancho, Solo medir. Sin latencia.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
