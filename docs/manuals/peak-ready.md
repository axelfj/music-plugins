# Limiter-R — Manual

![Limiter-R](../screenshots/peak-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Limiter-R es el **último plugin del master**. Sube el volumen de tu canción hasta el nivel que quieres y **nunca deja que pase del techo**, ni siquiera entre muestras (*true peak*). Trae un **medidor LUFS** con un objetivo, así sabes cuándo tu canción está lista para Spotify, YouTube o el club.

### Empieza en 1 minuto

1. Ponlo **al final** de la cadena del master (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un destino: **STREAMING −14**, **CLUB −8** o **EDM −6**.
3. Pulsa **REINICIAR LUFS** y dale play a la parte más fuerte de la canción (el drop o el coro).
4. Sube **ENTRADA** hasta que el medidor LUFS llegue al **OBJETIVO**. El plugin te dice cuántos dB faltan.
5. Mira la **reducción**: si pasa de 3–6 dB seguido, estás aplastando la mezcla. Baja un poco la Entrada o el objetivo.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ENTRADA** | -12 a +24 dB (0 dB) | Cuánto empujas la mezcla contra el techo. Más entrada = más volumen y más reducción. |
| **TECHO** | -12 a 0 dB (-1 dB) | Nivel máximo real de salida (true peak). −1 dB es seguro para streaming. |
| **RELEASE** | 10 a 1000 ms (200 ms) | Qué tan rápido vuelve después de limitar. Corto = más fuerte pero puede distorsionar graves. |
| **AUTO** (release) | encendido / apagado (encendido) | El release se adapta solo a cuánto se está limitando. |
| **ESTILO** | Transparente, Punchy, Agresivo (Transparente) | Transparente: limpio. Punchy: deja respirar los golpes. Agresivo: muy rápido + clipper, para EDM fuerte. |
| **TRUE PEAK** | encendido / apagado (encendido) | Mide los picos entre muestras. Apágalo solo para ahorrar CPU. |
| **OBJETIVO** | -24 a -4 LUFS (-9 LUFS) | El volumen al que quieres llegar. Streaming: −14 LUFS. Club / EDM: −8 a −6 LUFS. |
| **REINICIAR LUFS** | botón | Empieza de cero la medición integrada. Hazlo al inicio de la canción. |

**El medidor LUFS** muestra tres números: **M** (momentáneo, lo que suena ahora), **C** (corto plazo, últimos 3 segundos) e **I** (integrado, toda la canción desde que reiniciaste). El que importa para las plataformas es el **I**.

### Recetas

- **Para Spotify, Apple Music, YouTube:** preset Streaming −14. Techo −1 dB, estilo Transparente. Si quedas más fuerte que −14, las plataformas te bajan el volumen solas, así que no ganas nada aplastando.
- **Para tocar en el club:** preset Club −8, estilo Punchy.
- **EDM fuerte (big room, hardstyle, dubstep):** preset EDM −6, estilo Agresivo. Revisa que el bombo no pierda el golpe.
- **Solo proteger los picos (sin subir volumen):** Entrada 0 dB, Techo −1 dB. Solo actúa en los picos que pasen del techo.

### Tips

- Un limitador **no arregla** una mezcla mala: si necesitas más de 6 dB de reducción, revisa la mezcla primero.
- Antes del limitador puedes poner [Clipper-R](clip-ready.md) para quitar 1–2 dB de picos con menos esfuerzo.
- Para revisar tu master contra cada plataforma con más detalle, usa [Meter-R](master-check-live.md) después del limitador.
- El limitador añade unos milisegundos de latencia (el DAW la compensa).

---

## English

### What does it do?

Limiter-R is **the last plugin on the master**. It turns your song up to the level you want and **never lets it go over the ceiling**, not even between samples (*true peak*). It has a **LUFS meter** with a target, so you know when your song is ready for Spotify, YouTube or the club.

### Start in 1 minute

1. Put it **at the end** of the master chain (see [Getting started](primeros-pasos.md)).
2. Pick a destination: **STREAMING −14**, **CLUB −8** or **EDM −6**.
3. Press **RESET LUFS** and play the loudest part of the song (the drop or the chorus).
4. Raise **INPUT** until the LUFS meter reaches the **TARGET**. The plugin tells you how many dB are left.
5. Watch the **reduction**: if it's above 3–6 dB most of the time, you're squashing the mix. Lower the Input or the target a little.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **INPUT** | -12 to +24 dB (0 dB) | How hard you push the mix into the ceiling. More input = more loudness and more reduction. |
| **CEILING** | -12 to 0 dB (-1 dB) | Maximum true-peak output level. −1 dB is safe for streaming. |
| **RELEASE** | 10 to 1000 ms (200 ms) | How fast it recovers after limiting. Short = louder but may distort the lows. |
| **AUTO** (release) | on / off (on) | The release adapts by itself to how much limiting is happening. |
| **STYLE** | Transparent, Punchy, Aggressive (Transparent) | Transparent: clean. Punchy: lets the hits breathe. Aggressive: very fast + clipper, for loud EDM. |
| **TRUE PEAK** | on / off (on) | Measures the peaks between samples. Turn it off only to save CPU. |
| **TARGET** | -24 to -4 LUFS (-9 LUFS) | The loudness you want to reach. Streaming: −14 LUFS. Club / EDM: −8 to −6 LUFS. |
| **RESET LUFS** | button | Restarts the integrated measurement. Do it at the start of the song. |

**The LUFS meter** shows three numbers: **M** (momentary, what's playing now), **S** (short-term, last 3 seconds) and **I** (integrated, the whole song since you reset). The one platforms care about is **I**.

### Recipes

- **For Spotify, Apple Music, YouTube:** Streaming −14 preset. Ceiling −1 dB, Transparent style. If you're louder than −14, the platforms turn you down anyway, so squashing gains you nothing.
- **For club play:** Club −8 preset, Punchy style.
- **Loud EDM (big room, hardstyle, dubstep):** EDM −6 preset, Aggressive style. Check that the kick keeps its punch.
- **Just protect the peaks (no extra loudness):** Input 0 dB, Ceiling −1 dB. It only acts on peaks above the ceiling.

### Tips

- A limiter **doesn't fix** a bad mix: if you need more than 6 dB of reduction, check the mix first.
- Before the limiter you can put [Clipper-R](clip-ready.md) to shave 1–2 dB of peaks with less effort.
- To check your master against each platform in more detail, use [Meter-R](master-check-live.md) after the limiter.
- The limiter adds a few milliseconds of latency (the DAW compensates).
