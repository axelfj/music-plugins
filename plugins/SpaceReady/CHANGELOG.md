# Reverb-R (antes Space Ready)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.2

- Nuevo nombre: **Reverb-R** (antes Space Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Reverb-R** (formerly Space Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Space Ready deberían abrir con Reverb-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Space Ready should open with Reverb-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Space Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Space Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).
- Menos CPU: ~2.4x (2.0 % → 0.8 % de un núcleo): las ganancias de decaimiento solo se recalculan cuando Size o Decay cambian, los LFO giran como fasores y las líneas de delay usan índices sin división. / Less CPU: ~2.4x.

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Reverb algorítmica sobre una red de 8 líneas de retardo realimentadas (FDN) con matriz de Hadamard (ortogonal, conserva la energía) y ganancias por línea calculadas para el RT60 pedido: el Decay medido coincide con la perilla en los cuatro modos (test: ±15 %).
- Modos **Room**, **Plate**, **Hall** y **Ambience** (espacio enorme, estilo "supermassive", hasta 30 s), con Size (50-200 %).
- Modulación lenta de las líneas contra resonancias metálicas (test: en el espectro de la cola el pico más fuerte queda a menos de 18 dB de la mediana).
- Pre-delay 0-200 ms (test: al sample), Damping (los agudos mueren antes), Diffusion de entrada (4 all-pass por canal), Width mid/side (test: 0 % = mono exacto, 100 % = canales decorrelados), corte de graves y agudos en la salida.
- **Ducking**: la reverb baja mientras suena la señal original (test: > 20 dB) y **Freeze**: la cola se mantiene constante (test: ±1 dB en 2.5 s) y la entrada se silencia.
- Mix 0 % deja la señal original idéntica (test). Sin latencia. Cola larga estable (test: 30 s, size 200 %, modulación máxima).
- Vista de la envolvente de decaimiento con el nivel real de la cola encima. Presets: Sala, Plate voz, Hall y Espacio.
- Pendiente para 1.1: comparación a oído contra reverbs de referencia (issue #13), early reflections separadas.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
