# Resonance-R — Manual

![Resonance-R](../screenshots/res-tame.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Algunas notas o frecuencias "pitan" o molestan más que las demás: una voz que se vuelve chillona, un synth que zumba, una guitarra que pica. Resonance-R **encuentra esos picos solo y los baja solo cuando aparecen**. Es como un EQ que se mueve por sí mismo. También puede **hacerle espacio a otra pista** (modo *Unmask*): por ejemplo, bajar en los synths las frecuencias donde suena la voz.

### Empieza en 1 minuto

1. Ponlo en una voz, un synth, una guitarra o un bus (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un preset: **VOZ**, **SYNTH / GUITARRA**, **SIBILANCIA** o **UNMASK (SIDECHAIN)**.
3. Mira la gráfica: muestra cuánto se baja en cada frecuencia.
4. Activa **DELTA** un momento para escuchar solo lo que se quita. Debería sonar a "pitidos", no a la voz entera.
5. Ajusta **PROFUNDIDAD** hasta que el sonido deje de molestar sin perder brillo.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **PROFUNDIDAD** | 0 a 24 dB (8 dB) | Cuántos dB como máximo se le quitan a cada resonancia. |
| **FINURA** | 0 a 100 % (70 %) | Qué tan estrechos son los cortes. Alto = cortes finos; bajo = más anchos y suaves. |
| **SELECTIVIDAD** | 0 a 100 % (50 %) | Qué cuenta como resonancia. Alto = solo picos muy estrechos; bajo = también bultos anchos. |
| **UMBRAL** | 2 a 12 dB (6 dB) | Cuánto debe sobresalir un pico sobre lo que lo rodea para tocarlo. |
| **ATAQUE** | 1 a 200 ms (20 ms) | Qué tan rápido entra la reducción cuando aparece la resonancia. |
| **RELEASE** | 20 a 2000 ms (200 ms) | Qué tan rápido suelta cuando la resonancia desaparece. |
| **DESDE** | 20 Hz a 2 kHz (80 Hz) | Límite de abajo: no toca nada más grave que esto. |
| **HASTA** | 1 a 20 kHz (16 kHz) | Límite de arriba: no toca nada más agudo que esto. |
| **CANAL** | Stereo, Mid, Side (Stereo) | Stereo: los dos canales igual. Mid: solo el centro. Side: solo los lados. |
| **SIDECHAIN** (Unmask) | encendido / apagado (apagado) | El detector escucha otra pista por [sidechain](primeros-pasos.md#sidechain) y corta aquí donde ella suena fuerte. |
| **DELTA** | encendido / apagado (apagado) | Escuchas solo lo que se está quitando. |

**Latencia:** Resonance-R analiza el sonido por bloques, así que retrasa el audio unos 43 ms. Tu DAW lo compensa solo, pero si grabas con él puesto vas a notar el retraso: actívalo después de grabar.

### Recetas

- **Voz que suena chillona en las notas altas:** preset Voz (150 Hz a 12 kHz, 8 dB).
- **Synth o guitarra que zumba:** preset Synth / guitarra (cortes más finos y rápidos).
- **Agudos ásperos, "s" fuertes:** preset Sibilancia (de 2 a 12 kHz, cortes más anchos). Para "s" muy marcadas usa también [De-Esser-R](ess-clean.md).
- **Que los synths le dejen espacio a la voz:** pon Resonance-R en el bus de synths, preset Unmask, y manda la voz al sidechain. Cuando la voz canta, los synths se apartan solo en las frecuencias de la voz.
- **Master más suave:** Profundidad 3–4 dB, Canal Mid, Selectividad 70 %. Muy poquito.

### Tips

- Con mucha Profundidad el sonido se vuelve opaco. Menos es más: empieza en 4–6 dB.
- Usa **DESDE** y **HASTA** para trabajar solo donde está el problema.
- **DELTA** es la mejor forma de saber si te pasaste: si escuchas la melodía entera en el Delta, baja la Profundidad.

---

## English

### What does it do?

Some notes or frequencies "ring" or bother you more than the rest: a vocal that gets shrill, a synth that buzzes, a harsh guitar. Resonance-R **finds those peaks by itself and turns them down only when they appear**. It's like an EQ that moves on its own. It can also **make room for another track** (*Unmask* mode): for example, ducking the synths at the frequencies where the vocal sits.

### Start in 1 minute

1. Put it on a vocal, a synth, a guitar or a bus (see [Getting started](primeros-pasos.md)).
2. Pick a preset: **VOCAL**, **SYNTH / GUITAR**, **HARSHNESS** or **UNMASK (SIDECHAIN)**.
3. Look at the graph: it shows how much is reduced at each frequency.
4. Turn on **DELTA** for a moment to hear only what's removed. It should sound like "ringing", not the whole vocal.
5. Set **DEPTH** until the sound stops bothering you without losing its sparkle.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **DEPTH** | 0 to 24 dB (8 dB) | The most dB taken from each resonance. |
| **SHARPNESS** | 0 to 100 % (70 %) | How narrow the cuts are. High = fine cuts; low = wider, gentler cuts. |
| **SELECTIVITY** | 0 to 100 % (50 %) | What counts as a resonance. High = only very narrow peaks; low = broader bumps too. |
| **THRESHOLD** | 2 to 12 dB (6 dB) | How far a peak must stick out above its surroundings before it's touched. |
| **ATTACK** | 1 to 200 ms (20 ms) | How fast the reduction kicks in when the resonance appears. |
| **RELEASE** | 20 to 2000 ms (200 ms) | How fast it lets go when the resonance disappears. |
| **LOW** | 20 Hz to 2 kHz (80 Hz) | Lower limit: nothing below this is touched. |
| **HIGH** | 1 to 20 kHz (16 kHz) | Upper limit: nothing above this is touched. |
| **PLACEMENT** | Stereo, Mid, Side (Stereo) | Stereo: both channels the same. Mid: centre only. Side: sides only. |
| **SIDECHAIN** (Unmask) | on / off (off) | The detector listens to another track via [sidechain](primeros-pasos.md#sidechain-en) and cuts here where it's loud. |
| **DELTA** | on / off (off) | Hear only what's being removed. |

**Latency:** Resonance-R analyses the sound in blocks, so it delays the audio by about 43 ms. Your DAW compensates automatically, but if you record through it you'll notice the delay: turn it on after recording.

### Recipes

- **Vocal that gets shrill on high notes:** Vocal preset (150 Hz to 12 kHz, 8 dB).
- **Buzzing synth or guitar:** Synth / guitar preset (finer, faster cuts).
- **Harsh highs, loud "s" sounds:** Harshness preset (2 to 12 kHz, wider cuts). For very strong "s" sounds also use [De-Esser-R](ess-clean.md).
- **Make the synths leave room for the vocal:** put Resonance-R on the synth bus, Unmask preset, and send the vocal to the sidechain. When the vocal sings, the synths step aside only at the vocal's frequencies.
- **Smoother master:** Depth 3–4 dB, Placement Mid, Selectivity 70 %. Just a little.

### Tips

- Too much Depth makes the sound dull. Less is more: start at 4–6 dB.
- Use **LOW** and **HIGH** to work only where the problem is.
- **DELTA** is the best way to know if you went too far: if you can hear the whole melody in the Delta, lower the Depth.
