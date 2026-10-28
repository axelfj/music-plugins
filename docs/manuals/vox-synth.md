# Vocoder-R — Manual

![Vocoder-R](../screenshots/vox-synth.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Vocoder-R transforma tu voz de tres formas:

- **Armonías:** crea hasta **4 voces extra** a partir de la tuya (una tercera arriba, una quinta, una octava...) para que suene como un coro.
- **Vocoder:** tu voz "habla" a través de un sintetizador, como Daft Punk o un talkbox.
- **Robot:** toda tu voz canta una sola nota fija, como un robot o un monstruo.

Funciona con **voces solas** (una nota a la vez).

### Empieza en 1 minuto

1. Ponlo en la pista de la voz (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un preset: **3ª Y 5ª**, **CORO OCTAVAS**, **TALKBOX**, **VOCODER MIDI**, **ROBOT** o **MONSTRUO**.
3. Si usas Armonías, pon la **TONALIDAD** y la **ESCALA** de tu canción para que las voces no desafinen.
4. Ajusta **DRY** (tu voz original) y **MIX** (el efecto).

### Controles

**Generales:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **MODO** | Armonías, Vocoder, Robot (Armonías) | Armonías: hasta 4 voces afinadas a la escala o a MIDI. Vocoder: tu voz habla a través de un sinte. Robot: una sola nota fija. |
| **RANGO** | Voz grave (50 Hz), Normal (60 Hz), Voz aguda (80 Hz), Baja latencia (110 Hz) (Normal) | La nota más grave que sigue. Más alto = menos latencia. |
| **DRY** | -60 a +6 dB (0 dB) | Nivel de tu voz original. Al mínimo = solo el efecto. |
| **MIX** | 0 a 100 % (100 %) | Nivel del efecto. |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |

**Armonías:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **TONALIDAD** | C a B (C) | Nota raíz de la escala. |
| **ESCALA** | Cromática, Mayor, Menor (Mayor) | Las armonías por grados se calculan dentro de esta escala. |
| **INTERVALOS** | Grados de escala, Semitonos, Notas MIDI (Grados) | Grados: tercera, quinta... siempre dentro de la escala (suena bien solo). Semitonos: intervalo fijo. MIDI: cada voz canta una nota que tocas en el teclado. |
| **GLIDE** | 0 a 200 ms (20 ms) | Suaviza el cambio de nota de las armonías. |

**Por voz (VOZ 1 a VOZ 4):**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ON** | encendido / apagado (voces 1 y 2 encendidas) | Activa esta voz. |
| **Grado** | -7 a +7 (voz 1: +2, voz 2: +4) | Grados arriba (+) o abajo (−) de tu nota: +2 = tercera, +4 = quinta, +7 = octava. |
| **Semitonos** | -24 a +24 | Intervalo fijo (modo Semitonos). |
| **DETUNE** | -50 a +50 cents (0) | Desafinación para engordar el coro. |
| **Pan** | izquierda a derecha | Posición estéreo. |
| **NIVEL** | -36 a +6 dB (-6 dB) | Volumen de la voz. |
| **FORMANTE** | -12 a +12 (0) | Tamaño de la voz: negativo = más grande; positivo = más pequeña. |

**Vocoder:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **PORTADORA** | Sierra, Pulso, Ruido, Externa (sidechain) (Sierra) | El sonido que "habla". Externa: manda un sinte por el [sidechain](primeros-pasos.md#sidechain). |
| **SIGUE LA VOZ** | encendido / apagado (encendido) | La portadora toca la nota que cantas (talkbox). Apagado: toca la nota fija o las notas MIDI (acordes). |
| **NOTA** | C1 a C5 (C3) | Nota de la portadora cuando no sigue la voz y no hay MIDI. |
| **BANDAS** | 8 a 32 (20) | Más bandas = se entienden más las palabras, pero gasta más CPU. |
| **ATAQUE** | 1 a 50 ms (10 ms) | Qué tan rápido sube cada banda. |
| **RELEASE** | 5 a 500 ms (60 ms) | Qué tan rápido cae cada banda. |
| **SIBILANTES** | 0 a 100 % (30 %) | Deja pasar las consonantes (s, t, k) para entender las palabras. |
| **DETUNE** | -50 a +50 cents (0) | Desafinación de la portadora. |

**Robot:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **NOTA** | C1 a C5 (C3) | La única nota que canta el robot. |
| **FORMANTE** | -12 a +12 (0) | Tamaño del robot: negativo = monstruo; positivo = pequeño. |

**Latencia:** como [Tune-R](tune-ready.md), necesita escuchar un poquito de la voz, así que atrasa el audio unos 30–60 ms. El DAW lo compensa.

### Recetas

- **Coro de tercera y quinta:** preset 3ª y 5ª. Pon tu tonalidad y escala. Las voces quedan a izquierda y derecha.
- **Coro de octavas épico:** preset Coro octavas (una octava abajo y una arriba).
- **Talkbox / Daft Punk:** preset Talkbox. La portadora sigue tu voz y solo se oye el efecto.
- **Vocoder de acordes:** preset Vocoder MIDI. Manda acordes MIDI al plugin (puedes usar [Chord-R](chord-ready.md)): tu voz canta esos acordes. En Ableton: pista MIDI → *MIDI To* → la pista de la voz → Vocoder-R. En FL Studio: un canal *MIDI Out* con el mismo puerto que el *MIDI input port* del plugin.
- **Vocoder con tu propio sinte:** Portadora Externa, y manda el sinte al sidechain del plugin.
- **Robot o monstruo:** presets Robot y Monstruo (nota C2 con Formante −7).

### Tips

- Para que las armonías suenen bien, la **tonalidad** tiene que ser la correcta.
- Si no se entienden las palabras en el vocoder, sube **BANDAS** y **SIBILANTES**.
- Un poco de **DETUNE** (5–10 cents) en cada voz hace que el coro suene más real.

---

## English

### What does it do?

Vocoder-R transforms your voice three ways:

- **Harmony:** creates up to **4 extra voices** from yours (a third up, a fifth, an octave...) so it sounds like a choir.
- **Vocoder:** your voice "talks" through a synth, like Daft Punk or a talkbox.
- **Robot:** your whole voice sings one fixed note, like a robot or a monster.

It works on **solo vocals** (one note at a time).

### Start in 1 minute

1. Put it on the vocal track (see [Getting started](primeros-pasos.md)).
2. Pick a preset: **3RD & 5TH**, **OCTAVE CHOIR**, **TALKBOX**, **MIDI VOCODER**, **ROBOT** or **MONSTER**.
3. For Harmony, set your song's **KEY** and **SCALE** so the voices stay in tune.
4. Set **DRY** (your original voice) and **MIX** (the effect).

### Controls

**Global:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **MODE** | Harmony, Vocoder, Robot (Harmony) | Harmony: up to 4 voices tuned to the scale or to MIDI. Vocoder: your voice talks through a synth. Robot: one fixed note. |
| **RANGE** | Low voice (50 Hz), Normal (60 Hz), High voice (80 Hz), Low latency (110 Hz) (Normal) | The lowest note it tracks. Higher = less latency. |
| **DRY** | -60 to +6 dB (0 dB) | Level of your original voice. Minimum = effect only. |
| **MIX** | 0 to 100 % (100 %) | Effect level. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |

**Harmony:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **KEY** | C to B (C) | Root note of the scale. |
| **SCALE** | Chromatic, Major, Minor (Major) | Scale-degree harmonies are computed inside this scale. |
| **INTERVALS** | Scale degrees, Semitones, MIDI notes (Scale degrees) | Degrees: third, fifth... always inside the scale (sounds right by itself). Semitones: fixed interval. MIDI: each voice sings a note you hold on the keyboard. |
| **GLIDE** | 0 to 200 ms (20 ms) | Smooths the harmonies' pitch changes. |

**Per voice (VOICE 1 to VOICE 4):**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **ON** | on / off (voices 1 and 2 on) | Enables this voice. |
| **Degree** | -7 to +7 (voice 1: +2, voice 2: +4) | Scale degrees above (+) or below (−) your note: +2 = third, +4 = fifth, +7 = octave. |
| **Semitones** | -24 to +24 | Fixed interval (Semitones mode). |
| **DETUNE** | -50 to +50 cents (0) | Detune to thicken the choir. |
| **Pan** | left to right | Stereo position. |
| **LEVEL** | -36 to +6 dB (-6 dB) | Voice level. |
| **FORMANT** | -12 to +12 (0) | Voice size: negative = bigger; positive = smaller. |

**Vocoder:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **CARRIER** | Saw, Pulse, Noise, External (sidechain) (Saw) | The sound that "talks". External: feed a synth into the [sidechain](primeros-pasos.md#sidechain-en). |
| **FOLLOW VOICE** | on / off (on) | The carrier plays the note you sing (talkbox). Off: it plays the fixed note or the MIDI notes (chords). |
| **NOTE** | C1 to C5 (C3) | Carrier note when not following the voice and no MIDI is held. |
| **BANDS** | 8 to 32 (20) | More bands = clearer words, more CPU. |
| **ATTACK** | 1 to 50 ms (10 ms) | How fast each band rises. |
| **RELEASE** | 5 to 500 ms (60 ms) | How fast each band falls. |
| **SIBILANCE** | 0 to 100 % (30 %) | Lets consonants (s, t, k) through so the words stay clear. |
| **DETUNE** | -50 to +50 cents (0) | Carrier detune. |

**Robot:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **NOTE** | C1 to C5 (C3) | The only note the robot sings. |
| **FORMANT** | -12 to +12 (0) | Robot size: negative = monster; positive = tiny. |

**Latency:** like [Tune-R](tune-ready.md), it needs to hear a bit of the voice, so it delays the audio by about 30–60 ms. The DAW compensates.

### Recipes

- **Third-and-fifth choir:** 3rd & 5th preset. Set your key and scale. The voices sit left and right.
- **Epic octave choir:** Octave choir preset (one octave down and one up).
- **Talkbox / Daft Punk:** Talkbox preset. The carrier follows your voice and you only hear the effect.
- **Chord vocoder:** MIDI vocoder preset. Send MIDI chords to the plugin (you can use [Chord-R](chord-ready.md)): your voice sings those chords. In Ableton: MIDI track → *MIDI To* → the vocal track → Vocoder-R. In FL Studio: a *MIDI Out* channel on the same port as the plugin's *MIDI input port*.
- **Vocoder with your own synth:** Carrier External, and send the synth to the plugin's sidechain.
- **Robot or monster:** Robot and Monster presets (note C2 with Formant −7).

### Tips

- For the harmonies to sound right, the **key** has to be correct.
- If you can't understand the words in the vocoder, raise **BANDS** and **SIBILANCE**.
- A little **DETUNE** (5–10 cents) on each voice makes the choir sound more real.
