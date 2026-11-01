# Tune-R — Manual

![Tune-R](../screenshots/tune-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Tune-R **afina la voz en tiempo real**: lleva cada nota cantada a la nota correcta de la escala. Usado suave, nadie lo nota; usado al máximo, da el famoso efecto robótico de T-Pain y el trap. En modo **Fijo** también cambia la voz: más grave, más aguda o tipo robot. Funciona con **voces solas** (una nota a la vez), no con acordes.

### Empieza en 1 minuto

1. Ponlo en la pista de la voz (ver [Primeros pasos](primeros-pasos.md)).
2. Elige la **TONALIDAD** y la **ESCALA** de tu canción (por ejemplo, La menor = A + Minor). Si no sabes, deja **Cromática**.
3. Elige un preset: **NATURAL** (afinación que no se nota) o **DURO (T-PAIN)** (efecto robótico).
4. Mira la gráfica: la línea muestra la nota que cantas y la nota corregida.
5. Ajusta **VELOCIDAD**: más baja = más robot; más alta = más natural.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **MODO** | Corregir, Fijo (Corregir) | Corregir: lleva cada nota a la escala. Fijo: transpone y cambia formantes (voz grave, aguda, robot). |
| **TONALIDAD** | C a B (C) | Nota raíz de la escala. |
| **ESCALA** | Cromática, Mayor, Menor, Personalizada (Cromática) | Cromática: cualquier semitono. Mayor / Menor: solo las notas de la tonalidad. Personalizada: eliges las notas abajo. |
| **NOTAS PERMITIDAS** | 12 botones (todas) | Con escala Personalizada: qué notas están permitidas. |
| **VELOCIDAD** | 0 a 400 ms (20 ms) | Qué tan rápido llega a la nota. 0 ms = robot (T-Pain); 50–150 ms = natural. |
| **CANTIDAD** | 0 a 100 % (100 %) | Cuánto de la corrección aplicar. 100 % = exacto a la nota. |
| **HUMANIZE** | 0 a 50 cents (0) | Pequeña deriva aleatoria para que no suene perfecto. |
| **RANGO** | Voz grave (50 Hz), Normal (60 Hz), Voz aguda (80 Hz), Baja latencia (110 Hz) (Normal) | La nota más grave que sigue. Más alto = menos latencia. |
| **TONO** | -24 a +24 semitonos (0) | Modo Fijo: transpone la voz estos semitonos, sin corregir. |
| **FORMANTE** | -12 a +12 semitonos (0) | Tamaño de la voz: negativo = más grande y grave; positivo = más pequeña y aguda. No cambia la nota. |
| **MIDI** | encendido / apagado (encendido) | Las notas MIDI que mantengas pulsadas mandan: la voz va a esa nota exacta. |
| **MIX** | 0 a 100 % (100 %) | Mezcla con la voz original (para doblar). |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |

**Latencia:** Tune-R necesita escuchar un poquito de la voz para saber qué nota es, así que retrasa el audio unos 30 a 60 ms según el **RANGO**. El DAW lo compensa en la mezcla. Si quieres cantar con el efecto puesto, usa **Baja latencia (110 Hz)**.

### Recetas

- **Afinación invisible:** preset Natural (Velocidad 80 ms, Humanize 10 cents), con tu tonalidad y escala.
- **Efecto T-Pain / trap:** preset Duro (T-Pain), tonalidad y escala correctas. Canta con deslizamientos entre notas para que el salto se note más.
- **Voz grave de villano:** preset Voz grave (Fijo, Tono −4, Formante −4). Para más, baja el Formante a −8.
- **Voz aguda de dibujo animado:** preset Voz aguda (Fijo, Tono +4, Formante +4).
- **Doble una octava abajo:** Fijo, Tono −12, Formante 0, Mix 40 %.
- **Controlar la melodía con MIDI:** escribe la melodía en un clip MIDI y mándalo a Tune-R. En Ableton: pista MIDI → *MIDI To* → la pista de la voz → Tune-R. En FL Studio: un canal *MIDI Out* con el mismo puerto que el *MIDI input port* de Tune-R (engranaje del plugin).

### Tips

- Si la tonalidad está mal, la voz salta a notas raras. Si no estás seguro, usa **Cromática**.
- En silencios y consonantes ("s", "t") el plugin no toca la voz.
- Una voz grave masculina puede necesitar **RANGO: Voz grave** para que la siga bien.
- Las armonías (varias voces a la vez) están en [Vocoder-R](vox-synth.md).

---

## English

### What does it do?

Tune-R **tunes vocals in real time**: it pulls each sung note to the right note of the scale. Used gently, nobody notices; pushed all the way, it gives the famous T-Pain / trap robot effect. In **Fixed** mode it also changes the voice: deeper, higher or robotic. It works on **solo vocals** (one note at a time), not chords.

### Start in 1 minute

1. Put it on the vocal track (see [Getting started](primeros-pasos.md)).
2. Choose your song's **KEY** and **SCALE** (e.g. A minor = A + Minor). If you're not sure, leave **Chromatic**.
3. Pick a preset: **NATURAL** (tuning you don't notice) or **HARD** (robot effect).
4. Watch the graph: the line shows the note you sing and the corrected note.
5. Set **SPEED**: lower = more robot; higher = more natural.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **MODE** | Correct, Fixed (Correct) | Correct: pulls every note to the scale. Fixed: transposes and changes formants (deep, high, robot voice). |
| **KEY** | C to B (C) | Root note of the scale. |
| **SCALE** | Chromatic, Major, Minor, Custom (Chromatic) | Chromatic: any semitone. Major / Minor: only the notes of the key. Custom: you pick the notes below. |
| **ALLOWED NOTES** | 12 buttons (all) | With the Custom scale: which notes are allowed. |
| **SPEED** | 0 to 400 ms (20 ms) | How fast it reaches the note. 0 ms = robot (T-Pain); 50–150 ms = natural. |
| **AMOUNT** | 0 to 100 % (100 %) | How much of the correction to apply. 100 % = exactly on the note. |
| **HUMANIZE** | 0 to 50 cents (0) | Small random drift so it doesn't sound perfect. |
| **RANGE** | Low voice (50 Hz), Normal (60 Hz), High voice (80 Hz), Low latency (110 Hz) (Normal) | The lowest note it tracks. Higher = less latency. |
| **PITCH** | -24 to +24 semitones (0) | Fixed mode: transposes the voice by this many semitones, no correction. |
| **FORMANT** | -12 to +12 semitones (0) | Voice size: negative = bigger and deeper; positive = smaller and brighter. Doesn't change the note. |
| **MIDI** | on / off (on) | Held MIDI notes take over: the voice goes to that exact note. |
| **MIX** | 0 to 100 % (100 %) | Blend with the original voice (for doubling). |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |

**Latency:** Tune-R needs to hear a bit of the voice to know which note it is, so it delays the audio by about 30 to 60 ms depending on the **RANGE**. The DAW compensates in the mix. If you want to sing through the effect, use **Low latency (110 Hz)**.

### Recipes

- **Invisible tuning:** Natural preset (Speed 80 ms, Humanize 10 cents), with your key and scale.
- **T-Pain / trap effect:** Hard preset, correct key and scale. Sing with slides between notes so the jump stands out.
- **Deep villain voice:** Deep voice preset (Fixed, Pitch −4, Formant −4). For more, lower Formant to −8.
- **Cartoon high voice:** High voice preset (Fixed, Pitch +4, Formant +4).
- **Octave-down double:** Fixed, Pitch −12, Formant 0, Mix 40 %.
- **Drive the melody with MIDI:** write the melody in a MIDI clip and send it to Tune-R. In Ableton: MIDI track → *MIDI To* → the vocal track → Tune-R. In FL Studio: a *MIDI Out* channel on the same port as Tune-R's *MIDI input port* (the plugin's cog).

### Tips

- If the key is wrong, the voice jumps to odd notes. If you're not sure, use **Chromatic**.
- In silences and consonants ("s", "t") the plugin leaves the voice alone.
- A deep male voice may need **RANGE: Low voice** to be tracked well.
- Harmonies (several voices at once) live in [Vocoder-R](vox-synth.md).
