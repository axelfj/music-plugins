# Reverb-R — Manual

![Reverb-R](../screenshots/space-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Una reverb pone el sonido **dentro de un espacio**: un cuarto, una sala de conciertos o un lugar gigante que no existe. Hace que las voces y los instrumentos no suenen "pegados a la cara" y les da profundidad. Reverb-R tiene cuatro espacios: **Room**, **Plate**, **Hall** y **Ambience**.

### Empieza en 1 minuto

1. Ponlo en una voz, un synth o un canal de envío (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un preset: **SALA**, **PLATE VOZ**, **HALL** o **ESPACIO**.
3. Ajusta **DECAY**: cuánto dura la cola.
4. Ajusta **MIX**: 15–30 % en una pista; 100 % si está en un canal de envío.
5. Sube **CORTE GRAVES** a 150–300 Hz para que la reverb no embarre el bajo.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **MODO** | Room, Plate, Hall, Ambience (Hall) | Room: sala pequeña. Plate: brillante y denso para voces. Hall: sala grande. Ambience: espacio enorme, estilo *supermassive*. |
| **TAMAÑO** | 50 a 200 % (100 %) | Tamaño del espacio: cambia las distancias de los rebotes. |
| **DECAY** | 0.1 a 30 s (2 s) | Cuánto tarda la cola en apagarse (RT60). |
| **PRE-DELAY** | 0 a 200 ms (10 ms) | Silencio antes de que empiece la reverb. Separa la voz de la cola. |
| **AMORTIGUACIÓN** | 1 a 20 kHz (6 kHz) | Los agudos mueren antes que los graves, como en una sala real. Más bajo = más oscuro. |
| **DIFUSIÓN** | 0 a 100 % (60 %) | Bajo = ecos sueltos; alto = una nube densa. |
| **MODULACIÓN** | 0 a 100 % (50 %) | Vaivén lento dentro de la reverb: evita el sonido metálico y le da vida a la cola. |
| **ANCHO** | 0 a 100 % (100 %) | Ancho estéreo de la cola. 0 = mono. |
| **CORTE GRAVES** | 20 Hz a 1 kHz (Off) | Quita graves de la reverb para que no ensucie el bajo. |
| **CORTE AGUDOS** | 1 a 20 kHz (Off) | Quita agudos de la reverb para que no pise la voz. |
| **DUCKING** | 0 a 100 % (0 %) | Baja la reverb mientras suena el original y la sube en los huecos. |
| **MIX** | 0 a 100 % (30 %) | Mezcla entre el original y la reverb. |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |
| **FREEZE** | encendido / apagado (apagado) | Congela la cola para siempre y silencia lo nuevo que entra. |

La pantalla muestra la forma de la cola y, encima, el nivel real de la reverb mientras suena.

### Recetas

- **Voz pop o rap:** preset Plate voz (1.8 s, Pre-delay 20 ms, Ducking 40 %). La letra se entiende y la reverb llena los huecos.
- **Drums con espacio natural:** preset Sala (0.6 s, Mix 25 %).
- **Balada o pad cinematográfico:** preset Hall (2.8 s).
- **Pad infinito / ambient:** preset Espacio (Ambience, 14 s, Mix 50 %).
- **Transición o drop:** activa **FREEZE** en el último golpe antes del drop: la cola se queda sonando como un pad.
- **"Reverb en reversa" simple:** pon la reverb en un sample, expórtalo, dale la vuelta en tu DAW y ponlo antes del golpe.

### Tips

- Usar la reverb en un **canal de envío** (send) con Mix 100 % te deja compartir el mismo espacio entre varias pistas y ahorra CPU.
- **Pre-delay** de 20–40 ms hace que la voz siga adelante aunque la reverb sea grande.
- Si todo suena lejos y confuso, baja el Mix o el Decay, y sube el Corte graves.
- El modo Ambience con Decay largo puede llenar todo: úsalo con cuidado.

---

## English

### What does it do?

A reverb puts the sound **inside a space**: a room, a concert hall or a giant place that doesn't exist. It keeps vocals and instruments from sounding "stuck to your face" and gives them depth. Reverb-R has four spaces: **Room**, **Plate**, **Hall** and **Ambience**.

### Start in 1 minute

1. Put it on a vocal, a synth or a send channel (see [Getting started](primeros-pasos.md)).
2. Pick a preset: **ROOM**, **VOCAL PLATE**, **HALL** or **SUPERMASSIVE**.
3. Set **DECAY**: how long the tail lasts.
4. Set **MIX**: 15–30 % on a track; 100 % on a send channel.
5. Raise **LOW CUT** to 150–300 Hz so the reverb doesn't muddy the bass.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **MODE** | Room, Plate, Hall, Ambience (Hall) | Room: small room. Plate: bright and dense for vocals. Hall: large hall. Ambience: huge space, *supermassive* style. |
| **SIZE** | 50 to 200 % (100 %) | Size of the space: changes the distances of the reflections. |
| **DECAY** | 0.1 to 30 s (2 s) | How long the tail takes to die away (RT60). |
| **PRE-DELAY** | 0 to 200 ms (10 ms) | Silence before the reverb starts. Separates the vocal from the tail. |
| **DAMPING** | 1 to 20 kHz (6 kHz) | Highs die before lows, like in a real room. Lower = darker. |
| **DIFFUSION** | 0 to 100 % (60 %) | Low = distinct echoes; high = a dense cloud. |
| **MODULATION** | 0 to 100 % (50 %) | Slow drift inside the reverb: avoids metallic ringing and brings the tail to life. |
| **WIDTH** | 0 to 100 % (100 %) | Stereo width of the tail. 0 = mono. |
| **LOW CUT** | 20 Hz to 1 kHz (Off) | Removes lows from the reverb so it doesn't muddy the bass. |
| **HIGH CUT** | 1 to 20 kHz (Off) | Removes highs from the reverb so it doesn't step on the vocal. |
| **DUCKING** | 0 to 100 % (0 %) | Turns the reverb down while the original plays and up in the gaps. |
| **MIX** | 0 to 100 % (30 %) | Blend between the original and the reverb. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |
| **FREEZE** | on / off (off) | Holds the tail forever and mutes new input. |

The display shows the tail's shape with the reverb's real level on top while it plays.

### Recipes

- **Pop or rap vocal:** Vocal plate preset (1.8 s, 20 ms Pre-delay, 40 % Ducking). The lyrics stay clear and the reverb fills the gaps.
- **Drums with natural space:** Room preset (0.6 s, 25 % Mix).
- **Ballad or cinematic pad:** Hall preset (2.8 s).
- **Endless pad / ambient:** Supermassive preset (Ambience, 14 s, 50 % Mix).
- **Transition or drop:** turn on **FREEZE** on the last hit before the drop: the tail keeps ringing like a pad.
- **Simple "reverse reverb":** put the reverb on a sample, export it, reverse it in your DAW and place it before the hit.

### Tips

- Using the reverb on a **send channel** with Mix 100 % lets several tracks share the same space and saves CPU.
- A **Pre-delay** of 20–40 ms keeps the vocal up front even with a big reverb.
- If everything sounds far away and blurry, lower the Mix or the Decay, and raise the Low cut.
- Ambience mode with a long Decay can fill everything: use it carefully.
