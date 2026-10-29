# Sidechain-R (antes Duck Ready)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.2

- Nuevo nombre: **Sidechain-R** (antes Duck Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Sidechain-R** (formerly Duck Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Duck Ready deberían abrir con Sidechain-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Duck Ready should open with Sidechain-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Duck Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Duck Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Volume shaper sincronizado al tempo: la curva se calcula desde la posición del DAW, así que queda alineada al sample con la rejilla aunque se mueva el cursor, se haga loop o se inserte a mitad de la canción (test: posición exacta al sample).
- Curva paramétrica: Profundidad, Ataque, Hold, Release, Curva (forma) y Offset. Tres curvas listas: Clásico, Suave, Corto. Se puede arrastrar la curva (arriba/abajo = profundidad, izquierda/derecha = release).
- Velocidades: 1 compás, 1/2, 1/4, 1/4 T, 1/8 D, 1/8, 1/8 T, 1/16, 1/16 T, 1/32.
- Disparo por Tempo, por MIDI (cada nota reinicia la curva) o por Sidechain (detector de bombo con umbral).
- Modo multibanda: Full, Low (solo bajan los graves) o High, con crossover Linkwitz-Riley de 4.º orden que suma plano (test: < 0.05 dB).
- Sin clics: cualquier salto (reinicio, salto del transporte) se funde en 1 ms (test).
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).

Pendiente para 1.1: curva dibujada a mano con puntos libres (#17).
