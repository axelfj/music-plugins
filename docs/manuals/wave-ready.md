# Synth-R — Manual

![Synth-R](../screenshots/wave-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Synth-R es un **sintetizador**: un instrumento que crea sonidos desde cero (leads, bajos, pads, plucks, acordes). Está pensado para **aprender síntesis**: todo está a la vista, sin menús escondidos. El sonido viaja de izquierda a derecha:

**OSC 1 + OSC 2 + SUB Y RUIDO** (crean el sonido) → **FILTRO** (le da color) → **ENVOLVENTE AMP** (cómo empieza y termina) → **EFECTOS** (drive, chorus, delay, reverb).

La **ENVOLVENTE MOD** y los **LFO** mueven cosas solas (el filtro, el tono, el volumen) para que el sonido tenga vida.

### Empieza en 1 minuto

1. Cárgalo como **instrumento** (ver [Primeros pasos](primeros-pasos.md#instrumento)).
2. Abre **PRESET** y elige uno: Supersaw Lead, Pluck, Reese Bass, Sub 808, Future Chord, Pad, Organ, Arp Pluck, Acid, Hoover, Bell o Wobble.
3. Toca notas en el piano roll o en el **teclado en pantalla**.
4. Gira **CORTE** del filtro: es la perilla que más cambia el sonido.
5. Cuando cambias algo, el preset dice *(editado)*. Guarda tu sonido como preset de tu DAW.

### Controles

**OSC 1 y OSC 2** (los dos osciladores)

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ON** | encendido / apagado (OSC 1 encendido, OSC 2 apagado) | Enciende o apaga el oscilador. |
| **ONDA** | Sine, Triangle, Saw, Square, Pulse 25, Pulse 12, Organ, Formant, Metal, Digital (Saw) | La forma de onda: cada una suena distinta. Saw es brillante; Sine es suave. |
| **MORPH** | 0 a 100 % (0 %) | Mezcla poco a poco hacia la siguiente onda de la lista. |
| **OCTAVA** | -2 a +2 (0) | Sube o baja octavas. |
| **SEMI** | -12 a +12 semitonos (0) | Transposición en semitonos. |
| **FINO** | -100 a +100 cents (0) | Afinación fina. |
| **NIVEL** | 0 a 100 % (OSC 1: 80 %, OSC 2: 60 %) | Volumen del oscilador. |
| **PAN** | izquierda a derecha (centro) | Posición estéreo. |
| **UNÍSONO** | 1 a 7 (1) | Copias desafinadas del oscilador. 7 = supersaw. |
| **DESAFINE** | 0 a 100 cents (20) | Separación entre las copias del unísono. |
| **ANCHO** | 0 a 100 % (70 %) | Cuánto se reparten las copias a izquierda y derecha. |
| **FASE** / **RETRIG** | 0 a 100 % / apagado | Con RETRIG, cada nota empieza igual (ideal para bajos). |

**SUB Y RUIDO**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **SUB** | 0 a 100 % (0 %) | Oscilador sub una o dos octavas abajo. |
| **Octava del sub** | -1 oct, -2 oct (-1) | Cuántas octavas abajo. |
| **CUADRADA** | encendido / apagado (apagado) | Sub con onda cuadrada en vez de seno. |
| **RUIDO** | 0 a 100 % (0 %) | Capa de ruido (pasa por el filtro y la envolvente). |
| **COLOR** | White, Pink (White) | Blanco = brillante; rosa = más grave. |

**FILTRO**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **TIPO** | LP 12, LP 24, HP 12, BP 12 (LP 24) | LP quita agudos, HP quita graves, BP deja una banda. |
| **CORTE** | 20 Hz a 20 kHz (8 kHz) | Frecuencia del filtro. |
| **RESONANCIA** | 0 a 100 % (20 %) | Realce en la frecuencia de corte. |
| **DRIVE** | 0 a 100 % (0 %) | Saturación dentro del filtro. |
| **SEGUIR** | 0 a 100 % (0 %) | El corte sube con la nota (100 % = una octava por octava). |
| **ENV>CORTE** | -5 a +5 octavas (0) | Cuánto mueve la envolvente MOD el corte. |
| **VEL>CORTE** | 0 a 5 octavas (0) | Las notas más fuertes abren más el filtro. |

**ENVOLVENTE AMP y ENVOLVENTE MOD** (ADSR)

| Control | Rango (inicio AMP) | Qué hace |
| --- | --- | --- |
| **ATAQUE** | 0.1 ms a 10 s (5 ms) | Tiempo hasta el máximo. Largo = el sonido entra suave (pads). |
| **CAÍDA** | 1 ms a 10 s (200 ms) | Tiempo hasta el nivel de sostenido. |
| **SOSTENIDO** | 0 a 100 % (80 %) | Nivel mientras mantienes la tecla. 0 % = sonido corto (plucks). |
| **SOLTAR** | 1 ms a 10 s (200 ms) | Tiempo hasta el silencio después de soltar. |
| **VEL>AMP** | 0 a 100 % (50 %) | Cuánto cambia el volumen con la fuerza de la nota. |
| **ENV>TONO** (solo MOD) | -24 a +24 semitonos (0) | La envolvente MOD mueve el tono. Útil para 808 y hoover. |

**LFO 1 y LFO 2** (movimiento automático)

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **FORMA** | Sine, Triangle, Saw, Square, S&H (Sine) | Forma del movimiento. |
| **SYNC** | encendido / apagado (apagado) | Sigue el tempo del proyecto. |
| **VELOC.** | 0.01 a 40 Hz (LFO 1: 5 Hz, LFO 2: 0.5 Hz) | Velocidad (con SYNC apagado). |
| **DIVISIÓN** | 4 bars a 1/64 (1/4) | Duración de un ciclo (con SYNC encendido). |
| **FADE** | 0 a 5000 ms (0) | El LFO entra poco a poco tras cada nota. |
| **RETRIG** | encendido / apagado (encendido) | Reinicia el LFO en cada nota. |
| LFO 1 **>TONO / >CORTE / >AMP** | 0–12 st / ±5 oct / 0–100 % | Vibrato, movimiento del filtro y trémolo. |
| LFO 2 **>MORPH / >CORTE / >TONO** | 0–100 % / ±5 oct / 0–12 st | Mueve el MORPH de los osciladores, el filtro o el tono. |

**VOZ**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **MONO** | encendido / apagado (apagado) | Una sola nota a la vez (bajos, leads). |
| **LEGATO** | encendido / apagado (encendido) | En mono: una nota nueva sin soltar la anterior no reinicia las envolventes. |
| **GLIDE** | 0 a 2000 ms (0) | Deslizamiento entre notas. |
| **BEND** | 0 a 24 semitonos (2) | Rango de la rueda de pitch. |
| **AFINAR** | -100 a +100 cents (0) | Afinación global. |
| **RUEDA>CRT** | 0 a 5 octavas (0) | La rueda de modulación abre el filtro. |
| **RUEDA>VIB** | 0 a 2 semitonos (0.5) | La rueda de modulación añade vibrato (con LFO 1). |

**EFECTOS**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **DRIVE** | 0 a 36 dB (0) | Saturación global. |
| **CHORUS** (mix / veloc. / prof.) | 0–100 % / 0.05–10 Hz / 0–100 % (0 % / 0.6 Hz / 50 %) | Ensancha el sonido. |
| **DELAY** (mix) | 0 a 100 % (0 %) | Cantidad de eco. |
| **SYNC / DIVISIÓN / TIEMPO** | 1/8 o 1–4000 ms (1/8) | Tiempo del eco, al tempo o en ms. |
| **FEEDBACK** | 0 a 95 % (40 %) | Repeticiones. |
| **PING-PONG** | encendido / apagado (encendido) | Los ecos alternan izquierda y derecha. |
| **TONO** | 200 Hz a 20 kHz (6 kHz) | Los ecos pierden agudos por encima de esta frecuencia. |
| **REVERB** (mix) | 0 a 100 % (0 %) | Cantidad de reverb. |
| **COLA** | 0.1 a 30 s (2 s) | Duración de la reverb. |
| **PLATE** | encendido / apagado (apagado) | Plate (brillante) en vez de sala. |
| **SALIDA** | -24 a +12 dB (-6 dB) | Ganancia final. Un limitador suave evita que pase de 0 dBFS. |

### Recetas

- **Lead de EDM (supersaw):** preset Supersaw Lead. Para más ancho, sube UNÍSONO a 7 y DESAFINE a 30.
- **Bajo de house o techno:** preset Reese Bass, o desde Init: Saw, OCTAVA -1, MONO, CORTE 800 Hz.
- **808 de trap:** preset Sub 808. Toca notas encimadas con GLIDE para que se deslice.
- **Pluck para melodías:** preset Pluck o Arp Pluck (perfecto con [Arpeggiator-R](arp-ready.md)).
- **Acordes de future bass:** preset Future Chord con [Chord-R](chord-ready.md).
- **Pad de fondo:** preset Pad (ataque lento, chorus y reverb).
- **Bajo ácido (303):** preset Acid. Mueve CORTE y RESONANCIA mientras suena.
- **Wobble de dubstep:** preset Wobble. Cambia la DIVISIÓN del LFO 1 para variar el ritmo.

### Tips

- Para **aprender**: elige INIT, cambia una sola perilla y escucha qué pasa.
- Si suena "fino", agrega **SUB** o una segunda onda una octava abajo.
- Si suena muy brillante o áspero, baja el **CORTE** del filtro.
- Muchas voces de unísono en muchas notas gasta CPU: si va lento, baja el UNÍSONO.

---

## English

### What does it do?

Synth-R is a **synthesizer**: an instrument that builds sounds from scratch (leads, basses, pads, plucks, chords). It's designed for **learning synthesis**: everything is in view, no hidden menus. The sound flows left to right:

**OSC 1 + OSC 2 + SUB & NOISE** (make the sound) → **FILTER** (colours it) → **AMP ENV** (how it starts and ends) → **FX** (drive, chorus, delay, reverb).

The **MOD ENV** and the **LFOs** move things automatically (filter, pitch, volume) so the sound feels alive.

### Start in 1 minute

1. Load it as an **instrument** (see [Getting started](primeros-pasos.md#instrument-en)).
2. Open **PRESET** and pick one: Supersaw Lead, Pluck, Reese Bass, Sub 808, Future Chord, Pad, Organ, Arp Pluck, Acid, Hoover, Bell or Wobble.
3. Play notes in the piano roll or on the **on-screen keyboard**.
4. Turn the filter **CUTOFF**: it's the knob that changes the sound the most.
5. When you change something, the preset shows *(edited)*. Save your sound as a DAW preset.

### Controls

**OSC 1 and OSC 2** (the two oscillators)

| Control | Range (default) | What it does |
| --- | --- | --- |
| **ON** | on / off (OSC 1 on, OSC 2 off) | Turns the oscillator on or off. |
| **WAVE** | Sine, Triangle, Saw, Square, Pulse 25, Pulse 12, Organ, Formant, Metal, Digital (Saw) | The waveform: each one sounds different. Saw is bright; Sine is soft. |
| **MORPH** | 0 to 100 % (0 %) | Gradually blends towards the next wave in the list. |
| **OCTAVE** | -2 to +2 (0) | Transpose in octaves. |
| **SEMI** | -12 to +12 semitones (0) | Transpose in semitones. |
| **FINE** | -100 to +100 cents (0) | Fine tune. |
| **LEVEL** | 0 to 100 % (OSC 1: 80 %, OSC 2: 60 %) | Oscillator level. |
| **PAN** | left to right (centre) | Stereo position. |
| **UNISON** | 1 to 7 (1) | Detuned copies of the oscillator. 7 = supersaw. |
| **DETUNE** | 0 to 100 cents (20) | Spread between the unison copies. |
| **WIDTH** | 0 to 100 % (70 %) | How far the copies spread left and right. |
| **PHASE** / **RETRIG** | 0 to 100 % / off | With RETRIG, every note starts the same (great for bass). |

**SUB & NOISE**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **SUB** | 0 to 100 % (0 %) | Sub oscillator one or two octaves below. |
| **Sub octave** | -1 oct, -2 oct (-1) | How many octaves below. |
| **SQUARE** | on / off (off) | Square sub instead of sine. |
| **NOISE** | 0 to 100 % (0 %) | Noise layer (goes through the filter and envelope). |
| **COLOR** | White, Pink (White) | White = bright; pink = darker. |

**FILTER**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **TYPE** | LP 12, LP 24, HP 12, BP 12 (LP 24) | LP removes highs, HP removes lows, BP keeps a band. |
| **CUTOFF** | 20 Hz to 20 kHz (8 kHz) | Filter frequency. |
| **RESO** | 0 to 100 % (20 %) | Boost at the cutoff frequency. |
| **DRIVE** | 0 to 100 % (0 %) | Saturation inside the filter. |
| **KEY TRK** | 0 to 100 % (0 %) | Cutoff follows the note (100 % = one octave per octave). |
| **ENV>CUT** | -5 to +5 octaves (0) | How much the MOD envelope moves the cutoff. |
| **VEL>CUT** | 0 to 5 octaves (0) | Harder notes open the filter more. |

**AMP ENV and MOD ENV** (ADSR)

| Control | Range (AMP default) | What it does |
| --- | --- | --- |
| **ATTACK** | 0.1 ms to 10 s (5 ms) | Time to reach the peak. Long = the sound fades in (pads). |
| **DECAY** | 1 ms to 10 s (200 ms) | Time to fall to the sustain level. |
| **SUSTAIN** | 0 to 100 % (80 %) | Level while the key is held. 0 % = short sound (plucks). |
| **RELEASE** | 1 ms to 10 s (200 ms) | Time to silence after you let go. |
| **VEL>AMP** | 0 to 100 % (50 %) | How much velocity changes the level. |
| **ENV>PITCH** (MOD only) | -24 to +24 semitones (0) | The MOD envelope moves the pitch. Great for 808s and hoovers. |

**LFO 1 and LFO 2** (automatic movement)

| Control | Range (default) | What it does |
| --- | --- | --- |
| **SHAPE** | Sine, Triangle, Saw, Square, S&H (Sine) | Movement shape. |
| **SYNC** | on / off (off) | Follows the project tempo. |
| **RATE** | 0.01 to 40 Hz (LFO 1: 5 Hz, LFO 2: 0.5 Hz) | Speed (with SYNC off). |
| **DIVISION** | 4 bars to 1/64 (1/4) | Length of one cycle (with SYNC on). |
| **FADE** | 0 to 5000 ms (0) | The LFO fades in after each note. |
| **RETRIG** | on / off (on) | Restarts the LFO on every note. |
| LFO 1 **>PITCH / >CUTOFF / >AMP** | 0–12 st / ±5 oct / 0–100 % | Vibrato, filter movement and tremolo. |
| LFO 2 **>MORPH / >CUTOFF / >PITCH** | 0–100 % / ±5 oct / 0–12 st | Moves the oscillators' MORPH, the filter or the pitch. |

**VOICE**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **MONO** | on / off (off) | One note at a time (bass, leads). |
| **LEGATO** | on / off (on) | In mono: a new note while holding the previous one doesn't restart the envelopes. |
| **GLIDE** | 0 to 2000 ms (0) | Slide between notes. |
| **BEND** | 0 to 24 semitones (2) | Pitch wheel range. |
| **TUNE** | -100 to +100 cents (0) | Global tune. |
| **WHL>CUT** | 0 to 5 octaves (0) | The mod wheel opens the filter. |
| **WHL>VIB** | 0 to 2 semitones (0.5) | The mod wheel adds vibrato (with LFO 1). |

**FX**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **DRIVE** | 0 to 36 dB (0) | Global saturation. |
| **CHORUS** (mix / rate / depth) | 0–100 % / 0.05–10 Hz / 0–100 % (0 % / 0.6 Hz / 50 %) | Widens the sound. |
| **DELAY** (mix) | 0 to 100 % (0 %) | Amount of echo. |
| **SYNC / DIVISION / TIME** | 1/8 or 1–4000 ms (1/8) | Echo time, in tempo or in ms. |
| **FEEDBACK** | 0 to 95 % (40 %) | Repeats. |
| **PING-PONG** | on / off (on) | Repeats alternate left and right. |
| **TONE** | 200 Hz to 20 kHz (6 kHz) | Repeats lose highs above this frequency. |
| **REVERB** (mix) | 0 to 100 % (0 %) | Amount of reverb. |
| **DECAY** | 0.1 to 30 s (2 s) | Reverb length. |
| **PLATE** | on / off (off) | Plate (bright) instead of hall. |
| **OUTPUT** | -24 to +12 dB (-6 dB) | Final gain. A soft limiter keeps it under 0 dBFS. |

### Recipes

- **EDM lead (supersaw):** Supersaw Lead preset. For more width, push UNISON to 7 and DETUNE to 30.
- **House or techno bass:** Reese Bass preset, or from Init: Saw, OCTAVE -1, MONO, CUTOFF 800 Hz.
- **Trap 808:** Sub 808 preset. Play overlapping notes with GLIDE so it slides.
- **Pluck for melodies:** Pluck or Arp Pluck preset (perfect with [Arpeggiator-R](arp-ready.md)).
- **Future bass chords:** Future Chord preset with [Chord-R](chord-ready.md).
- **Background pad:** Pad preset (slow attack, chorus and reverb).
- **Acid bass (303):** Acid preset. Move CUTOFF and RESO while it plays.
- **Dubstep wobble:** Wobble preset. Change LFO 1's DIVISION to vary the rhythm.

### Tips

- To **learn**: pick INIT, change one knob at a time and listen to what happens.
- If it sounds "thin", add **SUB** or a second wave one octave down.
- If it's too bright or harsh, lower the filter **CUTOFF**.
- Lots of unison voices on lots of notes uses CPU: if it gets slow, lower UNISON.
