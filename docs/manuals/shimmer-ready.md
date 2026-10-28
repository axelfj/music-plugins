# Pitch Delay-R — Manual

![Pitch Delay-R](../screenshots/shimmer-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Pitch Delay-R es un delay donde **cada eco cambia de tono**. Con +12, cada repetición sube una octava: una nota se convierte en una **escalera** que sube al cielo. Con la **reverb shimmer** encendida, esos ecos se vuelven un pad brillante y etéreo. También puede hacer ecos al revés, quintas, y congelar el sonido.

### Empieza en 1 minuto

1. Ponlo en una voz, un synth, una guitarra o un pad (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un preset: **PAD OCTAVA**, **QUINTAS**, **VOZ ETÉREA**, **REVERSA**, **TRANSICIÓN** o **DELAY LIMPIO**.
3. Ajusta **TONO**: +12 sube una octava por eco, +7 una quinta, 0 es un delay normal.
4. Sube **SHIMMER** para más reverb en los ecos.
5. Ajusta **MIX**: 30–45 % suele ser suficiente.

### Controles

**Delay:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **SYNC** | encendido / apagado (encendido) | Tiempo del eco en divisiones del tempo del DAW. |
| **DIVISIÓN** | 1/32 a 1 bar, con T y D (1/8) | División del tempo (T = tresillo, D = con puntillo). |
| **TIEMPO** | 10 a 4000 ms (375 ms) | Tiempo libre del eco (con SYNC apagado). |
| **TONO** | -24 a +24 semitonos (+12) | Cuánto sube o baja cada repetición respecto a la anterior. 0 = delay normal. |
| **GRANO** | 20 a 200 ms (80 ms) | Tamaño de los "granos" del cambio de tono. Pequeño = más preciso en golpes; grande = más suave en pads. Se ajusta solo a la nota que detecta. |
| **REVERSA** | encendido / apagado (apagado) | Cada eco suena al revés. |
| **FEEDBACK** | 0 a 110 % (50 %) | Cuánto vuelve al bucle. Más de 100 % crece hasta un límite suave. |
| **GRAVES** | Off a 2 kHz (Off) | Quita graves en cada vuelta del bucle. |
| **AGUDOS** | 500 Hz a 20 kHz (Off) | Quita agudos en cada vuelta: los ecos se oscurecen. |
| **FREEZE** | encendido / apagado (apagado) | Congela el bucle: deja de entrar sonido nuevo y lo que hay se repite sin perder nivel. |

**Shimmer (reverb):**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **SHIMMER** | 0 a 100 % (50 %) | Reverb añadida a los ecos (el mismo motor de [Reverb-R](space-ready.md)). |
| **COLA** | 0.5 a 30 s (4 s) | Duración de la cola de la reverb. |

**Salida:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ANCHO** | 0 a 100 % (50 %) | Desplaza el eco derecho para abrir el estéreo. |
| **MIX** | 0 a 100 % (40 %) | Mezcla con la señal original. |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |

La pantalla muestra los ecos como una escalera, con la nota de cada repetición y la cola de la reverb.

### Recetas

- **Pad celestial:** preset Pad octava (+12, 1/4, Shimmer 60 %) en un pad o un acorde largo.
- **Synth con quintas:** preset Quintas (+7 en 1/8).
- **Voz etérea / ambient:** preset Voz etérea (mucho Shimmer, Agudos en 6 kHz para que no pique).
- **Ecos al revés:** preset Reversa en una voz o una guitarra.
- **Transición hacia el drop:** preset Transición (Feedback 85 %). Al final del build activa **FREEZE** y deja que suene sobre el silencio antes del drop.
- **Delay normal:** preset Delay limpio (Tono 0, sin Shimmer).

### Tips

- El cambio de tono funciona mejor con **una nota a la vez** (voz, lead). Con acordes también funciona, pero un poco menos preciso.
- Con Tono +12 y Feedback alto, los ecos se van muy agudos: baja **AGUDOS** para que no piquen.
- Usa **GRAVES** en 200–400 Hz para que los ecos no embarren el bajo.

---

## English

### What does it do?

Pitch Delay-R is a delay where **every echo changes pitch**. At +12, each repeat climbs an octave: one note becomes a **staircase** to the sky. With the **shimmer reverb** on, those echoes turn into a bright, ethereal pad. It can also do reverse echoes, fifths, and freeze the sound.

### Start in 1 minute

1. Put it on a vocal, a synth, a guitar or a pad (see [Getting started](primeros-pasos.md)).
2. Pick a preset: **OCTAVE PAD**, **FIFTHS**, **ETHEREAL VOICE**, **REVERSE**, **TRANSITION** or **CLEAN DELAY**.
3. Set **PITCH**: +12 climbs an octave per echo, +7 a fifth, 0 is a normal delay.
4. Raise **SHIMMER** for more reverb on the echoes.
5. Set **MIX**: 30–45 % is usually enough.

### Controls

**Delay:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **SYNC** | on / off (on) | Echo time in divisions of the DAW tempo. |
| **DIVISION** | 1/32 to 1 bar, with T and D (1/8) | Tempo division (T = triplet, D = dotted). |
| **TIME** | 10 to 4000 ms (375 ms) | Free echo time (with SYNC off). |
| **PITCH** | -24 to +24 semitones (+12) | How far each repeat climbs or drops from the previous one. 0 = plain delay. |
| **GRAIN** | 20 to 200 ms (80 ms) | Size of the pitch-shift "grains". Small = tighter on hits; big = smoother on pads. It locks itself to the detected note. |
| **REVERSE** | on / off (off) | Every echo plays backwards. |
| **FEEDBACK** | 0 to 110 % (50 %) | How much goes back round the loop. Above 100 % it grows up to a soft limit. |
| **LOW CUT** | Off to 2 kHz (Off) | Removes lows on every pass round the loop. |
| **HIGH CUT** | 500 Hz to 20 kHz (Off) | Removes highs on every pass: the echoes get darker. |
| **FREEZE** | on / off (off) | Freezes the loop: no new sound comes in and what's inside repeats without losing level. |

**Shimmer (reverb):**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **SHIMMER** | 0 to 100 % (50 %) | Reverb added to the echoes (the same engine as [Reverb-R](space-ready.md)). |
| **DECAY** | 0.5 to 30 s (4 s) | Length of the reverb tail. |

**Output:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **WIDTH** | 0 to 100 % (50 %) | Offsets the right echo to widen the stereo. |
| **MIX** | 0 to 100 % (40 %) | Blend with the original signal. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |

The display shows the echoes as a staircase, with each repeat's note and the reverb tail.

### Recipes

- **Heavenly pad:** Octave pad preset (+12, 1/4, Shimmer 60 %) on a pad or a long chord.
- **Synth with fifths:** Fifths preset (+7 at 1/8).
- **Ethereal / ambient vocal:** Ethereal voice preset (lots of Shimmer, High cut at 6 kHz so it isn't harsh).
- **Reverse echoes:** Reverse preset on a vocal or guitar.
- **Transition into the drop:** Transition preset (85 % Feedback). At the end of the build turn on **FREEZE** and let it ring over the silence before the drop.
- **Plain delay:** Clean delay preset (Pitch 0, no Shimmer).

### Tips

- The pitch shift works best on **one note at a time** (vocal, lead). It works on chords too, just a bit less precisely.
- With Pitch +12 and high Feedback the echoes get very high: lower the **HIGH CUT** so they aren't harsh.
- Use **LOW CUT** at 200–400 Hz so the echoes don't muddy the bass.
