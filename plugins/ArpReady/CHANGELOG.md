# Arpeggiator-R (antes Arp Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Arpeggiator-R** (antes Arp Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Arpeggiator-R** (formerly Arp Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Arp Ready abren igual con Arpeggiator-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Arp Ready open with Arpeggiator-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Arp Ready" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Arp Ready" copy so the DAW does not list the plugin twice.

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
