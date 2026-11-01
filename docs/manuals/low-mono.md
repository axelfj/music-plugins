# Stereo-R — Manual

![Stereo-R](../screenshots/low-mono.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Stereo-R controla **qué tan ancho** suena tu sonido (de mono a muy abierto) y pone **los graves en mono**. Los graves en mono hacen que el bombo y el bajo suenen firmes en cualquier parlante: en un club, en un celular o en audífonos. Además te muestra un **goniómetro** y un **medidor de correlación** para saber si tu mezcla va a sonar bien en mono.

### Empieza en 1 minuto

1. Ponlo al final del master o en un bus (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un preset: **MASTER SEGURO**, **CLUB**, **ANCHO** o **SOLO MEDIR**.
3. Mira la **CORRELACIÓN**: si está a la derecha (cerca de +1) todo bien; si baja de 0, hay partes que desaparecen en mono.
4. Ajusta **ANCHO** a tu gusto.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ANCHO** | 0 a 200 % (100 %) | 100 % = sin cambios, 0 % = mono, 200 % = el doble de lados. |
| **MONO DEBAJO DE** | Off a 500 Hz (120 Hz) | Los graves por debajo de esta frecuencia pasan a mono. Off = sin separar. |
| **ANCHO GRAVES** | 0 a 100 % (0 %) | Cuánto estéreo dejar en los graves. 0 % = mono total. |
| **48 dB/oct** | encendido / apagado (apagado) | Separa los graves de forma más abrupta (48 en vez de 24 dB/oct): más preciso, pero mueve más la fase. |

**Lo que ves:**

- **Goniómetro** (el cuadro con L, R, M, S): una línea vertical = mono; una nube ancha = muy estéreo.
- **CORRELACIÓN:** +1 = mono perfecto; 0 = muy ancho; menos de 0 = cuidado, parte de la mezcla se cancela en mono. El plugin te avisa con un mensaje.

### Recetas

- **Master para cualquier lugar:** preset Master seguro (mono debajo de 120 Hz).
- **Música para club:** preset Club (mono debajo de 150 Hz con 48 dB/oct, un poquito más ancho).
- **Pad o synth más abierto:** preset Ancho (Ancho 150 %, deja algo de estéreo en los graves).
- **Solo revisar la mezcla:** preset Solo medir (no cambia nada, solo mide).
- **Un synth que suena raro en mono:** baja el Ancho a 70–80 % hasta que la correlación quede por encima de 0.

### Tips

- No exageres con el Ancho: más de 150 % puede sonar "hueco" en mono.
- Revisa siempre en mono antes de exportar (muchos celulares y parlantes de fiesta son mono).
- Los graves por debajo de 100–150 Hz casi nunca necesitan estéreo.
- No tiene latencia: puedes dejarlo en el master todo el tiempo.

---

## English

### What does it do?

Stereo-R controls **how wide** your sound is (from mono to very open) and puts **the lows in mono**. Mono lows keep the kick and bass solid on any speaker: in a club, on a phone or in headphones. It also shows a **goniometer** and a **correlation meter** so you know whether your mix will hold up in mono.

### Start in 1 minute

1. Put it at the end of the master or on a bus (see [Getting started](primeros-pasos.md)).
2. Pick a preset: **SAFE MASTER**, **CLUB**, **WIDE** or **METER ONLY**.
3. Watch the **CORRELATION**: if it's on the right (near +1), all good; if it drops below 0, some parts disappear in mono.
4. Set **WIDTH** to taste.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **WIDTH** | 0 to 200 % (100 %) | 100 % = unchanged, 0 % = mono, 200 % = twice the sides. |
| **MONO BELOW** | Off to 500 Hz (120 Hz) | Lows below this frequency become mono. Off = no split. |
| **LOW WIDTH** | 0 to 100 % (0 %) | How much stereo to leave in the lows. 0 % = fully mono. |
| **48 dB/oct** | on / off (off) | Splits the lows more steeply (48 instead of 24 dB/oct): more precise, but shifts the phase more. |

**What you see:**

- **Goniometer** (the square with L, R, M, S): a vertical line = mono; a wide cloud = very stereo.
- **CORRELATION:** +1 = perfect mono; 0 = very wide; below 0 = careful, part of the mix cancels in mono. The plugin warns you with a message.

### Recipes

- **Master for anywhere:** Safe master preset (mono below 120 Hz).
- **Club music:** Club preset (mono below 150 Hz at 48 dB/oct, slightly wider).
- **More open pad or synth:** Wide preset (Width 150 %, leaves some stereo in the lows).
- **Just check the mix:** Meter only preset (changes nothing, only measures).
- **A synth that sounds odd in mono:** lower Width to 70–80 % until the correlation stays above 0.

### Tips

- Don't overdo Width: more than 150 % can sound "hollow" in mono.
- Always check in mono before exporting (many phones and party speakers are mono).
- Lows below 100–150 Hz almost never need stereo.
- It has no latency: you can leave it on the master all the time.
