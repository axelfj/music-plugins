# Arpeggiator-R — Manual

![Arpeggiator-R](../screenshots/arp-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Un arpegiador **toca las notas de un acorde una por una**, al ritmo de la canción. Mantienes un acorde y Arpeggiator-R lo convierte en una melodía que sube, baja o salta: el sonido típico del trance, el synthwave y muchos plucks de EDM. Trae un **sonido interno** para escucharlo al instante, y puede mandar el MIDI a cualquier otro instrumento.

### Empieza en 1 minuto

1. Cárgalo como **instrumento** (ver [Primeros pasos](primeros-pasos.md#instrumento)).
2. Dale play a tu DAW y **mantén un acorde** (en tu teclado MIDI, el piano roll o el **teclado de abajo**).
3. Prueba un **PRESET**: Classic Up, Trance 1/16, Up-Down 2 Oct, Pluck 1/8, Swing House, Triplet Roll, Offbeat, Latch Pad, Random 3 Oct o Slow 1/4 Down.
4. Cambia el **MODO** (orden de las notas) y la **VELOCIDAD** (1/8, 1/16...).
5. Para usar otro instrumento, apaga **SONIDO** y [manda el MIDI](primeros-pasos.md#midi) a ese instrumento.

### Controles

**ARPEGIO**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **MODO** | Arriba, Abajo, Arriba-abajo, Abajo-arriba, Como tocas, Aleatorio, Acorde (Arriba) | Orden en que suenan las notas que mantienes. Acorde: todas juntas en cada paso (ritmo de acordes). |
| **VELOCIDAD** | 1/1 a 1/32, con D y T (1/16) | Duración de cada paso. D = con puntillo, T = tresillo. |
| **OCTAVAS** | 1 a 4 (1) | Cuántas octavas recorre antes de repetir. |
| **DIR. OCT.** | Up, Down, Up-Down (Up) | Hacia dónde van las octavas extra. |
| **LATCH** | encendido / apagado (apagado) | Sigue sonando al soltar las teclas; tocar un acorde nuevo lo reemplaza. |
| **SYNC** | encendido / apagado (encendido) | Los pasos caen en la rejilla del DAW mientras reproduce. Apagado: reloj propio desde la primera nota. |

**TOQUE**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **GATE** | 5 a 100 % (50 %) | Duración de cada nota como porcentaje del paso. Corto = staccato; 100 % = ligado. |
| **SWING** | 0 a 100 % (0 %) | Retrasa los pasos pares para un ritmo con groove (100 % = tresillo). |
| **TRANSP.** | -24 a +24 semitonos (0) | Transposición. |
| **Velocity** | Como tocas, Fija (Como tocas) | Fuerza de las notas: la que tocaste o un valor fijo. |
| **FIJA** | 1 a 127 (100) | Velocity fija. |
| **ACENTO** | 0 a 100 % (0 %) | Refuerza el primer paso del patrón. |

**PATRÓN**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **PASO 1–16** | encendido / apagado (todos encendidos) | Paso encendido = suena; apagado = silencio. Así haces ritmos. |
| **LARGO** | 1 a 16 pasos (16) | Cuántos pasos tiene el patrón antes de repetirse. |

**SONIDO INTERNO**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **SONIDO** | encendido / apagado (encendido) | Sonido sencillo para escuchar el arpegio al instante. Apágalo cuando mandes el MIDI a otro instrumento. |
| **NIVEL** | -30 a +6 dB (-6 dB) | Volumen del sonido interno. |

La pantalla muestra el paso actual, el BPM y si está sincronizado al DAW o con reloj propio.

### Recetas

- **Arpegio de trance:** preset Trance 1/16. Mándalo a [Synth-R](wave-ready.md) con el preset Supersaw Lead o Arp Pluck.
- **Pluck de house con groove:** preset Swing House (1/16 con Swing 55 % y acento).
- **Synthwave:** Modo Arriba-abajo, 1/16, 2 octavas, Gate 70 %.
- **Ritmo de acordes:** Modo Acorde, 1/8, y apaga algunos pasos del patrón (por ejemplo 2, 5, 7) para un ritmo sincopado.
- **Pad que se mueve solo:** preset Latch Pad (modo Acorde en negras, ligado y con Latch): toca un acorde, suelta y sigue sonando.
- **Acordes + arpegio con una sola tecla:** pon [Chord-R](chord-ready.md) antes: tocas una tecla, Chord-R hace el acorde y Arpeggiator-R lo arpegia.

### Tips

- Con **SYNC** encendido, el arpegio siempre cae a tiempo con la canción. Dale play al DAW primero.
- Si suena muy "metralleta", baja la VELOCIDAD (1/8) o el GATE.
- Apagar pasos del patrón es la forma más rápida de hacer el arpegio más interesante.
- Al soltar las teclas, termina el paso actual y para (salvo con LATCH).

---

## English

### What does it do?

An arpeggiator **plays the notes of a chord one by one**, in time with the song. You hold a chord and Arpeggiator-R turns it into a melody that goes up, down or jumps around: the classic sound of trance, synthwave and lots of EDM plucks. It has a **built-in sound** so you hear it right away, and it can send the MIDI to any other instrument.

### Start in 1 minute

1. Load it as an **instrument** (see [Getting started](primeros-pasos.md#instrument-en)).
2. Press play in your DAW and **hold a chord** (on your MIDI keyboard, in the piano roll or on the **keyboard below**).
3. Try a **PRESET**: Classic Up, Trance 1/16, Up-Down 2 Oct, Pluck 1/8, Swing House, Triplet Roll, Offbeat, Latch Pad, Random 3 Oct or Slow 1/4 Down.
4. Change the **MODE** (note order) and the **RATE** (1/8, 1/16...).
5. To use another instrument, turn **SOUND** off and [send the MIDI](primeros-pasos.md#midi-en) to it.

### Controls

**ARPEGGIO**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **MODE** | Up, Down, Up-Down, Down-Up, As played, Random, Chord (Up) | Order the held notes play in. Chord: all together on every step (chord rhythm). |
| **RATE** | 1/1 to 1/32, with D and T (1/16) | Length of each step. D = dotted, T = triplet. |
| **OCTAVES** | 1 to 4 (1) | How many octaves it climbs before repeating. |
| **OCT. DIR.** | Up, Down, Up-Down (Up) | Where the extra octaves go. |
| **LATCH** | on / off (off) | Keeps playing after you let go; playing a new chord replaces it. |
| **SYNC** | on / off (on) | Steps land on the DAW grid while it plays. Off: its own clock from the first note. |

**FEEL**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **GATE** | 5 to 100 % (50 %) | Length of each note as a percentage of the step. Short = staccato; 100 % = legato. |
| **SWING** | 0 to 100 % (0 %) | Delays the even steps for a groovy feel (100 % = triplet). |
| **TRANSP.** | -24 to +24 semitones (0) | Transpose. |
| **Velocity** | As played, Fixed (As played) | Note velocity: as you played it or a fixed value. |
| **FIXED** | 1 to 127 (100) | Fixed velocity. |
| **ACCENT** | 0 to 100 % (0 %) | Boosts the first step of the pattern. |

**PATTERN**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **STEP 1–16** | on / off (all on) | Step on = plays; off = rest. This is how you make rhythms. |
| **LENGTH** | 1 to 16 steps (16) | How many steps the pattern has before it repeats. |

**BUILT-IN SOUND**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **SOUND** | on / off (on) | Simple sound to hear the arpeggio right away. Turn it off when sending the MIDI to another instrument. |
| **LEVEL** | -30 to +6 dB (-6 dB) | Built-in sound level. |

The display shows the current step, the BPM and whether it's synced to the DAW or on its own clock.

### Recipes

- **Trance arpeggio:** Trance 1/16 preset. Send it to [Synth-R](wave-ready.md) with the Supersaw Lead or Arp Pluck preset.
- **Groovy house pluck:** Swing House preset (1/16 with 55 % Swing and accent).
- **Synthwave:** Mode Up-Down, 1/16, 2 octaves, Gate 70 %.
- **Chord rhythm:** Mode Chord, 1/8, and turn off some pattern steps (e.g. 2, 5, 7) for a syncopated rhythm.
- **Self-moving pad:** Latch Pad preset (Chord mode in quarter notes, legato, with Latch): play a chord, let go and it keeps playing.
- **Chords + arpeggio from one key:** put [Chord-R](chord-ready.md) before it: you press one key, Chord-R builds the chord and Arpeggiator-R arpeggiates it.

### Tips

- With **SYNC** on, the arpeggio always lands in time with the song. Press play in the DAW first.
- If it sounds like a "machine gun", lower the RATE (1/8) or the GATE.
- Turning off pattern steps is the quickest way to make the arpeggio more interesting.
- When you let go of the keys, it finishes the current step and stops (unless LATCH is on).
