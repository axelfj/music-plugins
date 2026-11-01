# Chord-R (antes Chord Ready)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.2

- Nuevo nombre: **Chord-R** (antes Chord Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Chord-R** (formerly Chord Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Chord Ready deberían abrir con Chord-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Chord Ready should open with Chord-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Chord Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Chord Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Herramienta MIDI (se carga como instrumento): tocas una nota y sale un acorde completo. Pensada para quien todavía no sabe armar acordes.
- Modo **Escala**: eliges tonalidad y escala (mayor, menor, menor armónica, dórico, mixolidio, lidio, frigio, pentatónicas, cromática) y cada tecla da el acorde correcto de esa tonalidad (test: en Do mayor C→C, D→Dm, B→Bdim; con 7ª C→Cmaj7, G→G7). SNAP mueve las teclas fuera de la escala a la nota más cercana, así nunca suena mal (test).
- Modo **Fijo**: 20 tipos de acorde (mayor, menor, dim, aug, sus2, sus4, quinta, octava, maj7, m7, 7, m7b5, dim7, 6, m6, add9, madd9, maj9, m9, 9) siempre sobre la nota que toques (test).
- 7ª y 9ª opcionales, 4 voicings (cerrado, abierto/drop 2, extendido, amplio), inversiones −2..+2, raíz grave y aguda, transposición (tests de cada uno).
- Strum 0–300 ms arriba / abajo / alterno / aleatorio, con tiempos exactos en muestras incluso entre bloques (test), suavizado de notas agudas y humanize de velocity acotados (tests).
- Las notas compartidas entre dos acordes no se cortan hasta soltar el último; soltar una tecla apaga solo sus notas; soltar durante un strum cancela las que faltaban; CC 123/120 apaga todo (tests).
- Sonido interno sencillo para oír los acordes al instante, con nivel y apagado para cuando mandes el MIDI a otro instrumento.
- Pantalla con el nombre del acorde y sus notas, teclado para tocar con el ratón y teclado que muestra lo que sale, 10 presets (Triads, 7ths, 9ths Pad, Trance Stab, Power Chord, Lofi Keys, Guitar Strum, House Piano, Minor Fixed, Sus Wash).
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).

### Cómo mandar los acordes a otro instrumento / Routing the MIDI out

- **FL Studio**: carga Chord Ready en un canal, en el ajuste del wrapper activa *MIDI output* y pon un puerto (p. ej. 1). En el instrumento que quieras tocar, pon el *MIDI input port* al mismo número. Apaga SOUND en Chord Ready.
- **Ableton Live**: pon Chord Ready en una pista MIDI; en la pista del instrumento elige *MIDI From: (pista de Chord Ready)* → *Chord Ready* y *Monitor: In*.
- **Otros DAWs** (Bitwig, Reaper, Studio One, Cubase): enrutar la salida MIDI de la pista de Chord Ready a la pista del instrumento; en Bitwig también aparece como note FX (CLAP).

Pendiente para 1.0.x: acordes guardados por tecla (chord memory), detección de acorde al tocar varias notas, arpegiador integrado (ver Arp Ready).
