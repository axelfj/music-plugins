# Kick-R — Manual

![Kick-R](../screenshots/kick-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Kick-R es un **instrumento para crear bombos desde cero**. En vez de buscar un sample, diseñas tu propio bombo: lo afinas con el bajo, le pones más o menos golpe, más cola o más distorsión. Cuando te guste, lo puedes **guardar como WAV** y usarlo como sample en cualquier lugar.

Un bombo tiene tres partes:

1. **TONO:** el cuerpo empieza agudo y baja rápido hasta la nota. Ese "bajón" es el golpe.
2. **VOLUMEN:** cuánto dura y cómo se apaga.
3. **CLICK:** un sonidito corto al inicio que hace que se escuche en parlantes pequeños.

### Empieza en 1 minuto

1. Cárgalo como **instrumento** (ver [Primeros pasos](primeros-pasos.md#instrumento)).
2. Elige un preset: **HOUSE**, **TECHNO**, **HARDSTYLE** o **TRAP**.
3. Pulsa **PLAY** para escucharlo, o toca notas en el piano roll.
4. Con **SEGUIR MIDI** encendido, cada nota afina el bombo. Pon la nota igual a la tonalidad de tu canción (o la de tu bajo).
5. Ajusta **CAÍDA** del volumen para hacerlo más corto o más largo.

### Controles

**AFINACIÓN**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **SEGUIR MIDI** | encendido / apagado (encendido) | Encendido: cada nota MIDI afina el bombo. Apagado: siempre suena la nota elegida. |
| **NOTA** | C1 a C4 (C2) | Nota del bombo cuando no sigue MIDI. También la que usan PLAY y EXPORTAR WAV. |
| **AFINAR** | -12 a +12 semitonos (0) | Afinación fina (0.01 = 1 cent). La nota ya sale afinada; úsalo para encajar con el bajo. |
| **GLIDE** | 0 a 500 ms (0 ms) | Si tocas otra nota mientras suena, el tono se desliza desde la anterior (estilo 808). |
| **VELOCIDAD** | 0 a 100 % (50 %) | Cuánto cambia el volumen con la fuerza de la nota MIDI. 0 % = siempre igual. |

**TONO** (envolvente de tono)

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **INICIO** | 0 a 72 semitonos (36) | Cuántos semitonos arriba de la nota empieza el bombo. Más = más golpe. |
| **CAÍDA** | 5 a 1000 ms (60 ms) | Qué tan rápido baja el tono hasta la nota. Corto = seco (techno); largo = 808. |
| **CURVA** | 0 a 100 % (50 %) | Forma de la bajada: izquierda cae rápido y luego lento; derecha aguanta y luego cae. |

**VOLUMEN**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ATAQUE** | 0 a 20 ms (0.1 ms) | Rampa de entrada. Déjalo casi en 0 para un golpe seco. |
| **HOLD** | 0 a 300 ms (0 ms) | Tiempo a volumen máximo antes de caer. |
| **CAÍDA** | 20 a 3000 ms (400 ms) | Duración del bombo hasta el silencio. |
| **CURVA** | 0 a 100 % (70 %) | 0 % = caída recta; 100 % = exponencial (natural). |

**CLICK**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **TIPO** | Noise, Tick (Noise) | Noise: ráfaga de ruido filtrado. Tick: tono corto. |
| **NIVEL** | 0 a 100 % (30 %) | Volumen del click. |
| **CAÍDA** | 1 a 60 ms (6 ms) | Duración del click. |
| **TONO** | 200 Hz a 12 kHz (2.5 kHz) | Noise: desde qué frecuencia pasa el ruido. Tick: tono del tick. |

**SUCIEDAD Y SALIDA**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **DRIVE** | 0 a 36 dB (0 dB) | Saturación: redondea y añade armónicos. El pico se mantiene en 0 dBFS. |
| **CLIP** | 0 a 100 % (0 %) | Recorte: aplana el pico para que el bombo pegue más fuerte. 100 % = duro. |
| **SUB** | -12 a +12 dB (0 dB) | Realce o corte de graves (60 Hz). |
| **CORTE ALTO** | 200 Hz a 20 kHz (Off) | Quita brillo del click y la distorsión. |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |

**Botones:** **PLAY** escucha el bombo con la nota elegida. **EXPORTAR WAV** lo guarda como WAV de 24 bits para usarlo como sample.

### Recetas

- **House:** preset House. Bombo redondo con un click suave y algo de Sub.
- **Techno:** preset Techno. Más Drive y un poco de Clip para que corte.
- **Hardstyle:** preset Hardstyle. Drive alto, Clip al 100 % y Hold largo: el famoso bombo distorsionado. Prueba Inicio más alto para más "punch".
- **Trap / 808:** preset Trap. Caída larga (900 ms) y Sub +4 dB. Activa **GLIDE** (100–200 ms) y toca notas encimadas para que el 808 se deslice.
- **Bombo afinado con la canción:** apaga Seguir MIDI y elige la **NOTA** de la tonalidad (por ejemplo F para una canción en Fa menor).

### Tips

- Un bombo largo choca con el bajo: si tu bajo ya tiene sub, acorta la Caída del volumen.
- El **click** es lo que hace que el bombo se oiga en el celular. Si no se oye, sube el Nivel del click o el Tono.
- Exporta el bombo como WAV cuando estés contento: así usas menos CPU y lo puedes llevar a otros proyectos.

---

## English

### What does it do?

Kick-R is an **instrument for designing kicks from scratch**. Instead of hunting for a sample, you design your own kick: tune it to the bass, give it more or less punch, a longer tail or more distortion. When you like it, you can **save it as a WAV** and use it as a sample anywhere.

A kick has three parts:

1. **PITCH:** the body starts high and drops fast down to the note. That "drop" is the punch.
2. **AMP:** how long it lasts and how it fades.
3. **CLICK:** a short sound at the start that makes it audible on small speakers.

### Start in 1 minute

1. Load it as an **instrument** (see [Getting started](primeros-pasos.md#instrument-en)).
2. Pick a preset: **HOUSE**, **TECHNO**, **HARDSTYLE** or **TRAP**.
3. Press **PLAY** to hear it, or play notes in the piano roll.
4. With **FOLLOW MIDI** on, every note tunes the kick. Set the note to your song's key (or your bass's).
5. Set the amp **DECAY** to make it shorter or longer.

### Controls

**TUNING**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **FOLLOW MIDI** | on / off (on) | On: every MIDI note tunes the kick. Off: always plays the chosen note. |
| **NOTE** | C1 to C4 (C2) | Kick note when not following MIDI. Also used by PLAY and EXPORT WAV. |
| **TUNE** | -12 to +12 semitones (0) | Fine tune (0.01 = 1 cent). The note is already in tune; use it to sit with the bass. |
| **GLIDE** | 0 to 500 ms (0 ms) | If you play another note while it's sounding, the pitch slides from the previous one (808 style). |
| **VELOCITY** | 0 to 100 % (50 %) | How much MIDI velocity changes the level. 0 % = always the same. |

**PITCH** (pitch envelope)

| Control | Range (default) | What it does |
| --- | --- | --- |
| **START** | 0 to 72 semitones (36) | How many semitones above the note the kick starts. More = more punch. |
| **DECAY** | 5 to 1000 ms (60 ms) | How fast the pitch falls to the note. Short = tight (techno); long = 808. |
| **CURVE** | 0 to 100 % (50 %) | Shape of the fall: left drops fast then slow; right holds then drops. |

**AMP**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **ATTACK** | 0 to 20 ms (0.1 ms) | Fade-in. Keep it near 0 for a sharp hit. |
| **HOLD** | 0 to 300 ms (0 ms) | Time at full level before the decay. |
| **DECAY** | 20 to 3000 ms (400 ms) | Length of the kick until silence. |
| **CURVE** | 0 to 100 % (70 %) | 0 % = straight fade; 100 % = exponential (natural). |

**CLICK**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **TYPE** | Noise, Tick (Noise) | Noise: burst of filtered noise. Tick: short tone. |
| **LEVEL** | 0 to 100 % (30 %) | Click level. |
| **DECAY** | 1 to 60 ms (6 ms) | Click length. |
| **TONE** | 200 Hz to 12 kHz (2.5 kHz) | Noise: frequency the noise passes from. Tick: pitch of the tick. |

**DIRT & OUTPUT**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **DRIVE** | 0 to 36 dB (0 dB) | Saturation: rounds and adds harmonics. Peak stays at 0 dBFS. |
| **CLIP** | 0 to 100 % (0 %) | Clipping: flattens the peak so the kick hits harder. 100 % = hard. |
| **SUB** | -12 to +12 dB (0 dB) | Low boost or cut (60 Hz). |
| **HIGH CUT** | 200 Hz to 20 kHz (Off) | Removes brightness from the click and the drive. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |

**Buttons:** **PLAY** previews the kick at the chosen note. **EXPORT WAV** saves it as a 24-bit WAV to use as a sample.

### Recipes

- **House:** House preset. Round kick with a soft click and a bit of Sub.
- **Techno:** Techno preset. More Drive and some Clip so it cuts through.
- **Hardstyle:** Hardstyle preset. High Drive, Clip at 100 % and a long Hold: the famous distorted kick. Try a higher Start for more punch.
- **Trap / 808:** Trap preset. Long decay (900 ms) and Sub +4 dB. Turn on **GLIDE** (100–200 ms) and play overlapping notes so the 808 slides.
- **Kick tuned to the song:** turn off Follow MIDI and pick the song's key as the **NOTE** (e.g. F for a song in F minor).

### Tips

- A long kick clashes with the bass: if your bass already has sub, shorten the amp Decay.
- The **click** is what makes the kick audible on a phone. If you can't hear it, raise the click Level or Tone.
- Export the kick as a WAV once you're happy: it saves CPU and you can take it to other projects.
