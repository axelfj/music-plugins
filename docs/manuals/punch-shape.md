# Transient-R — Manual

![Transient-R](../screenshots/punch-shape.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Transient-R cambia **la forma de cada golpe**. El **ataque** es el inicio del golpe (el "tac" de una caja); el **sustain** es lo que suena después (la cola, el ambiente). Con dos perillas puedes darle más pegada a un bombo, secar una caja con mucho cuarto, o suavizar unos platos que pican. Funciona igual a cualquier volumen, así que no hay que ajustar umbrales.

### Empieza en 1 minuto

1. Ponlo en un bombo, una caja, un loop de batería o una guitarra (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un preset: **MÁS PEGADA**, **SUAVIZAR**, **MÁS COLA** o **BOMBO SECO**.
3. Ajusta **ATAQUE** y **SUSTAIN** a oído.
4. Si subes mucho el ataque, activa **CLIP** para que los picos no pasen de 0 dB.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ATAQUE** | -24 a +24 dB (0 dB) | Sube (+) o suaviza (−) los golpes. +6 dB = más pegada; −6 dB = más suave y lejano. |
| **SUSTAIN** | -24 a +24 dB (0 dB) | Alarga (+) o acorta (−) las colas. −8 dB seca una caja con mucho ambiente; +6 dB le da más cuerpo. |
| **VELOCIDAD** | 5 a 100 ms (20 ms) | Cuánto dura un "golpe" para el detector. Corto para hi-hats, largo para bombos y bajos. |
| **COLA** | 50 a 2000 ms (300 ms) | Cuánto tiempo sigue escuchando el sustain después del golpe. |
| **DETECTOR** | Todo, Graves, Agudos (Todo) | Qué escucha el detector. Graves = reacciona solo al bombo y al bajo. Agudos = solo a platos y hi-hats. Todo el sonido se procesa igual. |
| **CLIP** | encendido / apagado (apagado) | Clipper suave a −0.3 dBFS en la salida, para que los ataques subidos no pasen de 0. |
| **SALIDA** | -24 a +12 dB (0 dB) | Volumen de salida. |

Con Ataque y Sustain en 0 no cambia nada. La pantalla muestra el nivel y cuánto se sube el ataque o el sustain en cada golpe.

### Recetas

- **Bombo o caja con más pegada:** preset Más pegada (Ataque +8 dB, Sustain −4 dB).
- **Bombo seco y corto:** preset Bombo seco (Detector Graves, Sustain −10 dB).
- **Caja de batería real con mucho cuarto:** Sustain −8 dB, Ataque +3 dB.
- **Platos o hi-hats que pican:** preset Suavizar (Ataque −8 dB) o Detector Agudos.
- **Drums con más ambiente y cuerpo:** preset Más cola (Sustain +8 dB).
- **Guitarra acústica más rítmica:** Ataque +4 dB, Velocidad 10 ms.
- **Loop completo, solo el bombo:** Detector Graves: reacciona al bombo y empuja ese momento de todo el loop.

### Tips

- Para drums electrónicos, +3 a +6 dB de ataque suele bastar.
- Subir el ataque sube los picos: activa **CLIP** o pon un [Limiter-R](peak-ready.md) después.
- Bajar el sustain es una forma rápida de "limpiar" un loop con mucha reverb.

---

## English

### What does it do?

Transient-R changes **the shape of each hit**. The **attack** is the start of the hit (a snare's "crack"); the **sustain** is what rings after (the tail, the room). With two knobs you can give a kick more punch, dry up a roomy snare, or soften harsh cymbals. It works the same at any level, so there are no thresholds to set.

### Start in 1 minute

1. Put it on a kick, a snare, a drum loop or a guitar (see [Getting started](primeros-pasos.md)).
2. Pick a preset: **MORE PUNCH**, **SOFTEN**, **MORE TAIL** or **TIGHT KICK**.
3. Set **ATTACK** and **SUSTAIN** by ear.
4. If you push the attack a lot, turn on **CLIP** so the peaks never go over 0 dB.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **ATTACK** | -24 to +24 dB (0 dB) | Boosts (+) or softens (−) the hits. +6 dB = more punch; −6 dB = softer, further away. |
| **SUSTAIN** | -24 to +24 dB (0 dB) | Lengthens (+) or shortens (−) the tails. −8 dB dries a roomy snare; +6 dB gives it more body. |
| **SPEED** | 5 to 100 ms (20 ms) | How long a "hit" lasts for the detector. Short for hi-hats, long for kicks and bass. |
| **TAIL** | 50 to 2000 ms (300 ms) | How long the sustain stage keeps listening after the hit. |
| **DETECTOR** | All, Lows, Highs (All) | What the detector listens to. Lows = reacts only to kick and bass. Highs = only to cymbals and hi-hats. The whole sound is still processed. |
| **CLIP** | on / off (off) | Soft clipper at −0.3 dBFS on the output, so boosted attacks never go over 0. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Output level. |

With Attack and Sustain at 0 nothing changes. The display shows the level and how much the attack or sustain is lifted on each hit.

### Recipes

- **Kick or snare with more punch:** More punch preset (Attack +8 dB, Sustain −4 dB).
- **Short, tight kick:** Tight kick preset (Detector Lows, Sustain −10 dB).
- **Real-kit snare with too much room:** Sustain −8 dB, Attack +3 dB.
- **Harsh cymbals or hi-hats:** Soften preset (Attack −8 dB) or Detector Highs.
- **Drums with more room and body:** More tail preset (Sustain +8 dB).
- **More rhythmic acoustic guitar:** Attack +4 dB, Speed 10 ms.
- **Whole loop, kick only:** Detector Lows: it reacts to the kick and pushes that moment of the whole loop.

### Tips

- For electronic drums, +3 to +6 dB of attack is usually enough.
- Boosting the attack raises the peaks: turn on **CLIP** or put a [Limiter-R](peak-ready.md) after it.
- Lowering the sustain is a quick way to "clean up" a loop with a lot of reverb.
