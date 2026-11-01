# Stereo-R (antes Low Mono)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.2

- Nuevo nombre: **Stereo-R** (antes Low Mono). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Stereo-R** (formerly Low Mono): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Low Mono deberían abrir con Stereo-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Low Mono should open with Stereo-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Low Mono" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Low Mono" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

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
