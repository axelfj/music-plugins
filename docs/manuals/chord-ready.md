# Chord-R — Manual

![Chord-R](../screenshots/chord-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Chord-R es para **tocar acordes aunque no sepas armarlos**. Tocas **una sola tecla** y sale un **acorde completo**. En modo **Escala**, eliges la tonalidad de tu canción y cada tecla te da el acorde correcto: nunca suena mal. Trae un **sonido interno** para escucharlo al instante, y puede mandar el MIDI a cualquier otro instrumento (Synth-R, Serum, Vital, un piano...).

### Empieza en 1 minuto

1. Cárgalo como **instrumento** (ver [Primeros pasos](primeros-pasos.md#instrumento)).
2. Elige la **TONALIDAD** y la **ESCALA** de tu canción (por ejemplo, La menor = A + menor).
3. Toca una tecla (en tu teclado MIDI, el piano roll o el **teclado de abajo**): suena un acorde. La pantalla dice el nombre del acorde y sus notas.
4. Prueba un **PRESET**: Triads, 7ths, 9ths Pad, Trance Stab, Power Chord, Lofi Keys, Guitar Strum, House Piano, Minor Fixed o Sus Wash.
5. Cuando quieras usar otro instrumento, apaga **SONIDO** y [manda el MIDI](primeros-pasos.md#midi) a ese instrumento.

### Controles

**ESCALA Y ACORDE**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **MODO** | Escala, Fijo (Escala) | Escala: el acorde se arma con las notas de la tonalidad (una tecla = un acorde correcto). Fijo: siempre el mismo tipo de acorde sobre la nota que toques. |
| **TONALIDAD** | C a B (C) | Nota raíz de la tonalidad de tu canción. |
| **ESCALA** | Mayor, menor, menor armónica, dórico, mixolidio, lidio, frigio, pentatónicas, cromática (Mayor) | Tipo de escala. Mayor = alegre; menor = triste u oscuro. |
| **AJUSTAR** | encendido / apagado (encendido) | Una tecla fuera de la escala se mueve a la nota más cercana de la escala, así nunca suena mal. |
| **7ª** | encendido / apagado (apagado) | Agrega la séptima: acordes de 4 notas, más jazz, lofi o house. |
| **9ª** | encendido / apagado (apagado) | Agrega la novena: acordes más amplios y soñadores. |
| **ACORDE FIJO** | 20 tipos: mayor, menor, dim, aug, sus2, sus4, quinta, octava, maj7, m7, 7, m7b5, dim7, 6, m6, add9, madd9, maj9, m9, 9 (mayor) | Tipo de acorde en modo Fijo. |

**VOICING**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **VOICING** | Cerrado, Abierto, Extendido, Amplio (Cerrado) | Cómo se reparten las notas. Cerrado: juntas. Abierto: la segunda más aguda baja una octava. Extendido: notas alternas suben. Amplio: la grave baja y la aguda sube. |
| **INVERSIÓN** | -2 a +2 (0) | Mueve la nota más grave una octava arriba (+) o la más aguda una octava abajo (−). Sirve para que los acordes no "salten". |
| **TRANSP.** | -24 a +24 semitonos (0) | Transpone todo el acorde. |
| **BAJO 8ª** | encendido / apagado (apagado) | Agrega la raíz una octava abajo (más cuerpo). |
| **AGUDO 8ª** | encendido / apagado (apagado) | Agrega la raíz arriba del acorde (más brillo, estilo trance). |

**STRUM Y TOQUE**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **STRUM** | 0 a 300 ms (0) | Tiempo entre una nota y la siguiente, como rasgar una guitarra. 0 = todas a la vez. |
| **DIRECCIÓN** | Arriba, Abajo, Alterno, Aleatorio (Arriba) | Orden del strum: de grave a agudo, al revés, alternando cada acorde o al azar. |
| **SUAVIZAR** | 0 a 100 % (0 %) | Las notas agudas salen más suaves que la raíz, como una mano real. |
| **HUMANIZE** | 0 a 100 % (0 %) | Variación aleatoria de la fuerza de cada nota. |

**SONIDO INTERNO**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **SONIDO** | encendido / apagado (encendido) | Sonido sencillo para escuchar los acordes al instante. Apágalo cuando mandes el MIDI a otro instrumento. |
| **NIVEL** | -30 a +6 dB (-6 dB) | Volumen del sonido interno. |

En la pantalla ves **TOCAS** (la tecla que tocaste) y **SALE (MIDI OUT)** (las notas que salen).

### Recetas

- **Progresión de pop o EDM en La menor:** Tonalidad A, Escala menor. Toca A, F, C, G (las teclas, una a la vez): suena Am – F – C – G.
- **Acordes de lofi:** preset Lofi Keys, o 7ª encendida + Voicing Abierto + Strum 30 ms.
- **Stabs de trance:** preset Trance Stab con Agudo 8ª. Mándalo a [Synth-R](wave-ready.md) con un preset Supersaw.
- **Piano de house:** preset House Piano, con un ritmo corto en el piano roll.
- **Guitarra rasgueada:** preset Guitar Strum (Strum + Dirección Alterno).
- **Acordes que no saltan:** cuando cambies de acorde, prueba Inversión −1 o +1 para que las notas se muevan poco.
- **Arpegio de acordes:** pon [Arpeggiator-R](arp-ready.md) después de Chord-R para arpegiar los acordes que salen.

### Tips

- Con **AJUSTAR** encendido puedes tocar cualquier tecla, blanca o negra, y siempre suena dentro de la escala.
- Si no sabes en qué tonalidad está tu canción, prueba C mayor o A menor (solo teclas blancas).
- Chord-R no hace sonido de calidad por sí solo: es para escuchar mientras compones. Para la versión final, manda el MIDI a un buen sinte o piano.

---

## English

### What does it do?

Chord-R lets you **play chords even if you don't know how to build them**. You press **one key** and a **full chord** comes out. In **Scale** mode, you pick your song's key and every key gives you the right chord: it never sounds wrong. It has a **built-in sound** so you hear it right away, and it can send the MIDI to any other instrument (Synth-R, Serum, Vital, a piano...).

### Start in 1 minute

1. Load it as an **instrument** (see [Getting started](primeros-pasos.md#instrument-en)).
2. Choose your song's **KEY** and **SCALE** (e.g. A minor = A + minor).
3. Press a key (on your MIDI keyboard, in the piano roll or on the **keyboard below**): a chord plays. The display shows the chord's name and notes.
4. Try a **PRESET**: Triads, 7ths, 9ths Pad, Trance Stab, Power Chord, Lofi Keys, Guitar Strum, House Piano, Minor Fixed or Sus Wash.
5. When you want to use another instrument, turn **SOUND** off and [send the MIDI](primeros-pasos.md#midi-en) to it.

### Controls

**SCALE & CHORD**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **MODE** | Scale, Fixed (Scale) | Scale: the chord is built from the key's notes (one key = one correct chord). Fixed: always the same chord type on the note you play. |
| **KEY** | C to B (C) | Root note of your song's key. |
| **SCALE** | Major, minor, harmonic minor, dorian, mixolydian, lydian, phrygian, pentatonics, chromatic (Major) | Scale type. Major = happy; minor = sad or dark. |
| **SNAP** | on / off (on) | A key outside the scale moves to the nearest scale note, so it never sounds wrong. |
| **7TH** | on / off (off) | Adds the seventh: 4-note chords, more jazz, lofi or house. |
| **9TH** | on / off (off) | Adds the ninth: wider, dreamier chords. |
| **FIXED CHORD** | 20 types: major, minor, dim, aug, sus2, sus4, fifth, octave, maj7, m7, 7, m7b5, dim7, 6, m6, add9, madd9, maj9, m9, 9 (major) | Chord type in Fixed mode. |

**VOICING**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **VOICING** | Close, Open, Spread, Wide (Close) | How the notes are spread out. Close: together. Open: second-highest drops an octave. Spread: alternate notes go up. Wide: lowest down, highest up. |
| **INVERSION** | -2 to +2 (0) | Moves the lowest note up an octave (+) or the highest down (−). Keeps chord changes from "jumping". |
| **TRANSP.** | -24 to +24 semitones (0) | Transposes the whole chord. |
| **BASS 8VE** | on / off (off) | Adds the root one octave down (more body). |
| **TOP 8VE** | on / off (off) | Adds the root above the chord (more sparkle, trance style). |

**STRUM & FEEL**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **STRUM** | 0 to 300 ms (0) | Time between one note and the next, like strumming a guitar. 0 = all at once. |
| **DIRECTION** | Up, Down, Alternate, Random (Up) | Strum order: low to high, reversed, alternating per chord or random. |
| **SOFTEN** | 0 to 100 % (0 %) | Higher notes come out softer than the root, like a real hand. |
| **HUMANIZE** | 0 to 100 % (0 %) | Random variation of each note's velocity. |

**BUILT-IN SOUND**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **SOUND** | on / off (on) | Simple sound to hear the chords right away. Turn it off when sending the MIDI to another instrument. |
| **LEVEL** | -30 to +6 dB (-6 dB) | Built-in sound level. |

The display shows **YOU PLAY** (the key you pressed) and **OUT (MIDI OUT)** (the notes coming out).

### Recipes

- **Pop or EDM progression in A minor:** Key A, Scale minor. Press A, F, C, G (one key at a time): you get Am – F – C – G.
- **Lofi chords:** Lofi Keys preset, or 7th on + Voicing Open + Strum 30 ms.
- **Trance stabs:** Trance Stab preset with Top 8ve. Send it to [Synth-R](wave-ready.md) with a Supersaw preset.
- **House piano:** House Piano preset, with a short rhythm in the piano roll.
- **Strummed guitar:** Guitar Strum preset (Strum + Direction Alternate).
- **Chords that don't jump:** when you change chords, try Inversion −1 or +1 so the notes move less.
- **Arpeggiated chords:** put [Arpeggiator-R](arp-ready.md) after Chord-R to arpeggiate the chords it sends.

### Tips

- With **SNAP** on you can press any key, white or black, and it always stays in the scale.
- If you don't know your song's key, try C major or A minor (white keys only).
- Chord-R's built-in sound is just for listening while you write. For the final version, send the MIDI to a good synth or piano.
