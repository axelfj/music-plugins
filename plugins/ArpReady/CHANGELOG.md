# Arpeggiator-R (antes Arp Ready)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.2

- Nuevo nombre: **Arpeggiator-R** (antes Arp Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Arpeggiator-R** (formerly Arp Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Arp Ready deberían abrir con Arpeggiator-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Arp Ready should open with Arpeggiator-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Arp Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Arp Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Arpegiador clásico como herramienta MIDI (se carga como instrumento): mantienes notas y las reparte en el tiempo.
- 7 modos: arriba, abajo, arriba-abajo, abajo-arriba, como tocas, aleatorio (nunca repite la misma nota seguida) y acorde (todas juntas por paso). Tests de orden exacto de cada modo.
- 15 velocidades del 1/1 al 1/32 con puntillo y tresillo, tiempos exactos en muestras (test: 1/8 a 120 BPM = 12000 muestras a 48 kHz; 1/8T = 8000; cambio de tempo del DAW en vivo; si el DAW no da tempo, 120 BPM).
- 1 a 4 octavas hacia arriba, abajo o ida y vuelta (tests), transposición.
- Gate 5–100 % (100 % = ligado, la nota se corta justo antes de la siguiente, test), swing hasta tresillo (test: paso par retrasado 1/3 de paso), velocity como tocas o fija, acento en el primer paso.
- Patrón rítmico de 16 pasos con largo ajustable (los silencios no se comen la nota, test), latch (sigue al soltar; un acorde nuevo lo reemplaza, test).
- SYNC: con el DAW reproduciendo, los pasos caen en la rejilla de compás (la primera nota suena al instante y la siguiente ya en el beat, test); sin reproducir, reloj propio desde la primera nota.
- Al soltar las teclas termina el paso actual con su gate y para (test); CC 123/120 apaga todo (test).
- Sonido interno sencillo para oírlo al instante, pantalla con paso actual / BPM / sync, 16 botones de patrón con indicador de paso, teclado para tocar con el ratón y teclado que muestra lo que sale, 10 presets (Classic Up, Trance 1/16, Up-Down 2 Oct, Pluck 1/8, Swing House, Triplet Roll, Offbeat, Latch Pad, Random 3 Oct, Slow 1/4 Down).
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).

### Cómo mandar el arpegio a otro instrumento / Routing the MIDI out

- **FL Studio**: carga Arp Ready en un canal, en el wrapper activa *MIDI output* con un puerto (p. ej. 2) y en el instrumento pon el *MIDI input port* al mismo número. Apaga SOUND en Arp Ready.
- **Ableton Live**: Arp Ready en una pista MIDI; en la pista del instrumento, *MIDI From: (pista de Arp Ready)* → *Arp Ready* y *Monitor: In*.
- **Otros DAWs**: enruta la salida MIDI de la pista de Arp Ready al instrumento; en Bitwig aparece también como note FX (CLAP).
- Combo recomendado: Chord Ready → Arp Ready → tu sinte (una tecla = acorde arpegiado).

Pendiente para 1.0.x: ratchets / repeticiones por paso, patrón de velocity por paso, modo de pasos con notas fijas (secuenciador).
