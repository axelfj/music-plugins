# De-Esser-R (antes Ess Clean)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.2

- Nuevo nombre: **De-Esser-R** (antes Ess Clean). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **De-Esser-R** (formerly Ess Clean): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Ess Clean deberían abrir con De-Esser-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Ess Clean should open with De-Esser-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Ess Clean" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Ess Clean" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- De-esser con rango de detección ajustable (Desde / Hasta, 2–16 kHz) dibujado sobre el espectro de la entrada en tiempo real.
- Dos modos: **Banda** (solo baja la banda detectada; lo demás no se toca: test: un tono de 200 Hz cambia menos de 0.3 dB mientras la "s" baja 10 dB en el centro del rango; en los bordes del rango baja la mitad) y **Todo** (baja la señal completa, estilo clásico).
- Umbral, Reducción máxima (0–24 dB, test: la "s" baja exactamente lo que marca), Ataque y Release. Rodilla suave de 6 dB.
- **Lookahead** opcional (1, 2 o 5 ms, con latencia reportada al DAW) para que el primer milisegundo de la "s" también baje (test).
- **Escuchar**: oyes solo lo que se está quitando, para ajustar el rango (test: escuchar + salida normal = entrada).
- Un detector para los dos canales: la imagen estéreo no se mueve.
- Por debajo del umbral la salida es idéntica a la entrada, bit a bit, en los dos modos y con lookahead (test).
- Presets por tipo de voz: Grave, Media, Aguda.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
