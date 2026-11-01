# Chorus-R — Manual

![Chorus-R](../screenshots/wide-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Chorus-R hace que un sonido suene **más ancho y más grande**, como si lo tocaran varias personas a la vez. Tiene tres modos:

- **Chorus:** copias del sonido con un vaivén lento. Más ancho y brillante.
- **Flanger:** un barrido metálico tipo "avión" (jet).
- **Micro-pitch:** dos copias un poquito desafinadas, una a cada lado. Ancho, pero sin que se note que "se mueve". Perfecto para voces.

### Empieza en 1 minuto

1. Ponlo en un synth, un pad, una guitarra o una voz (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un preset: **SYNTH CHORUS**, **PAD ENSEMBLE**, **VOZ ANCHA** o **FLANGER JET**.
3. Ajusta **MIX**: 50 % es el clásico; menos si lo quieres sutil.
4. Sube o baja **ESTÉREO** para más o menos anchura.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **MODO** | Chorus, Flanger, Micro-pitch (Chorus) | Chorus: voces con retardo largo. Flanger: retardo corto con feedback. Micro-pitch: dos voces desafinadas, sin vaivén. |
| **VOCES** | 1, 2, 3 (2) | Número de voces del chorus. |
| **ENSEMBLE** | encendido / apagado (apagado) | Cada voz va a su propia velocidad: más rico, estilo *string machine*. |
| **VELOCIDAD** | 0.05 a 10 Hz (0.4 Hz) | Velocidad del vaivén. |
| **PROFUNDIDAD** | 0 a 100 % (50 %) | Cuánto se mueve el retardo. En Flanger, 100 % con through-zero cruza el cero. |
| **RETARDO** | 0.1 a 40 ms (15 ms) | Chorus: retardo base (5–40 ms suena a coro). Flanger: retardo manual (0.1–10 ms). |
| **FEEDBACK** | -95 a +95 % (0 %) | Flanger: realimenta la salida para un sonido más metálico. Negativo cambia el timbre. |
| **THRU-ZERO** | encendido / apagado (apagado) | Retrasa también la señal original para que el barrido la cruce: el flanger de cinta clásico. |
| **INVERTIR** | encendido / apagado (apagado) | Invierte la polaridad de las voces. Con through-zero se cancela al cruzar (efecto más fuerte). |
| **ESTÉREO** | 0 a 100 % (50 %) | Desfase del vaivén entre izquierda y derecha. 0 = mono. |
| **DESAFINE** | 0 a 50 cents (12 cents) | Micro-pitch: la izquierda sube y la derecha baja. Ancho sin vaivén. |
| **MIX** | 0 a 100 % (50 %) | Mezcla entre el original y las voces. 50 % es el clásico. |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |

La pantalla muestra las voces moviéndose en el tiempo, a la izquierda y a la derecha.

### Recetas

- **Synth o guitarra más ancha:** preset Synth chorus (2 voces, 0.4 Hz).
- **Pad gigante:** preset Pad ensemble (3 voces con Ensemble y Estéreo alto).
- **Voz de coro más ancha:** preset Voz ancha (Micro-pitch, 9 cents). Usa Mix 20–30 % para que la voz siga clara en el centro.
- **Flanger de avión:** preset Flanger jet (through-zero, invertido, feedback 75 %). Automatiza la Profundidad para un barrido en un build.
- **Bajo con chorus pero firme:** Chorus, 1 voz, Mix 30 %. Si el bajo pierde fuerza, pon después [Stereo-R](low-mono.md) para dejar los graves en mono.

### Tips

- Revisa siempre en **mono**: si el sonido desaparece o se vuelve débil, baja ESTÉREO o MIX.
- En el bajo y el bombo, mejor no usar chorus en los graves.
- Micro-pitch es el más "invisible": úsalo cuando quieras ancho sin efecto obvio.

---

## English

### What does it do?

Chorus-R makes a sound **wider and bigger**, as if several people were playing it at once. It has three modes:

- **Chorus:** copies of the sound with a slow wobble. Wider and shinier.
- **Flanger:** a metallic "jet plane" sweep.
- **Micro-pitch:** two slightly detuned copies, one on each side. Wide, but without an audible wobble. Perfect for vocals.

### Start in 1 minute

1. Put it on a synth, a pad, a guitar or a vocal (see [Getting started](primeros-pasos.md)).
2. Pick a preset: **SYNTH CHORUS**, **PAD ENSEMBLE**, **WIDE VOCAL** or **JET FLANGER**.
3. Set **MIX**: 50 % is the classic; less if you want it subtle.
4. Raise or lower **SPREAD** for more or less width.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **MODE** | Chorus, Flanger, Micro-pitch (Chorus) | Chorus: voices with a long delay. Flanger: short delay with feedback. Micro-pitch: two detuned voices, no wobble. |
| **VOICES** | 1, 2, 3 (2) | Number of chorus voices. |
| **ENSEMBLE** | on / off (off) | Each voice runs at its own speed: richer, string-machine style. |
| **RATE** | 0.05 to 10 Hz (0.4 Hz) | Speed of the wobble. |
| **DEPTH** | 0 to 100 % (50 %) | How far the delay moves. In Flanger, 100 % with through-zero crosses zero. |
| **DELAY** | 0.1 to 40 ms (15 ms) | Chorus: base delay (5–40 ms sounds like a choir). Flanger: manual delay (0.1–10 ms). |
| **FEEDBACK** | -95 to +95 % (0 %) | Flanger: feeds the output back for a more metallic sound. Negative changes the timbre. |
| **THRU-ZERO** | on / off (off) | Delays the original too so the sweep crosses it: the classic tape flanger. |
| **INVERT** | on / off (off) | Flips the voices' polarity. With through-zero it cancels at the crossing (stronger effect). |
| **SPREAD** | 0 to 100 % (50 %) | Wobble offset between left and right. 0 = mono. |
| **DETUNE** | 0 to 50 cents (12 cents) | Micro-pitch: left goes up, right goes down. Width without wobble. |
| **MIX** | 0 to 100 % (50 %) | Blend between the original and the voices. 50 % is the classic. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |

The display shows the voices moving over time, on the left and the right.

### Recipes

- **Wider synth or guitar:** Synth chorus preset (2 voices, 0.4 Hz).
- **Huge pad:** Pad ensemble preset (3 voices with Ensemble and high Spread).
- **Wider backing vocals:** Wide vocal preset (Micro-pitch, 9 cents). Use Mix 20–30 % so the vocal stays clear in the centre.
- **Jet flanger:** Jet flanger preset (through-zero, inverted, 75 % feedback). Automate Depth for a sweep in a build.
- **Bass with chorus that stays solid:** Chorus, 1 voice, Mix 30 %. If the bass loses power, put [Stereo-R](low-mono.md) after it to keep the lows mono.

### Tips

- Always check in **mono**: if the sound disappears or gets weak, lower SPREAD or MIX.
- On bass and kick, avoid chorus in the lows.
- Micro-pitch is the most "invisible": use it when you want width without an obvious effect.
