# Compressor-R — Manual

![Compressor-R](../screenshots/press-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Un compresor **baja el volumen de las partes fuertes** para que el sonido sea más parejo. Así una voz se entiende siempre, unos drums suenan más pegados y un bajo no salta de volumen. Compressor-R tiene tres caracteres (**Clean**, **Punch**, **Glue**), sidechain y lookahead.

### Empieza en 1 minuto

1. Ponlo en una voz, un bajo o un bus de drums (ver [Primeros pasos](primeros-pasos.md)).
2. Baja el **UMBRAL** hasta que el medidor de **REDUCCIÓN** marque entre 3 y 6 dB en las partes fuertes.
3. Deja **RATIO** en 4:1.
4. Activa **AUTO GAIN** para que el volumen quede parecido y compares sin engañarte.
5. Prueba los tres **CARÁCTER** y quédate con el que más te guste.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **UMBRAL** | -60 a 0 dB (-18 dB) | Nivel a partir del cual empieza a bajar el volumen. Más bajo = comprime más. |
| **RATIO** | 1:1 a ∞:1 (4:1) | Cuánto baja lo que pasa del umbral. 2:1 suave, 4:1 normal, 10:1 fuerte, ∞:1 limitador. |
| **ATAQUE** | 0.05 a 300 ms (10 ms) | Qué tan rápido reacciona. Rápido (1–5 ms) controla picos; lento (20–50 ms) deja pasar el golpe y da punch. |
| **RELEASE** | 5 a 3000 ms (120 ms) | Qué tan rápido suelta. Corto = más fuerte y agresivo; largo = más suave y natural. |
| **AUTO RELEASE** | encendido / apagado (apagado) | El release se adapta: suelta rápido los golpes cortos y despacio lo sostenido. |
| **KNEE** | 0 a 24 dB (6 dB) | Transición alrededor del umbral. 0 dB = codo duro (preciso); 12 dB o más = muy suave (musical). |
| **MAKEUP** | -12 a +24 dB (0 dB) | Volumen que devuelves después de comprimir. |
| **AUTO GAIN** | encendido / apagado (apagado) | Sube el volumen automáticamente para compensar la compresión. |
| **MIX** | 0 a 100 % (100 %) | Mezcla con la señal sin comprimir (compresión paralela). |
| **CARÁCTER** | Clean, Punch, Glue (Clean) | Clean: transparente. Punch: estilo FET, agarra rápido y colorea. Glue: estilo bus, lento y suave, para pegar una mezcla o un grupo. |
| **DETECTOR** | Peak, RMS (Peak) | Peak reacciona a los golpes. RMS reacciona al volumen percibido (más suave, mejor para voces y buses). |
| **FILTRO SC** | Off, 60, 100, 150, 250, 400 Hz (Off) | Quita graves de lo que "escucha" el compresor, para que el bombo o el bajo no hagan bombear todo. |
| **LOOKAHEAD** | Off, 1, 2, 5, 10 ms (Off) | Mira unos milisegundos hacia adelante para atrapar picos antes de que lleguen. Añade latencia (el DAW la compensa). |
| **SC EXTERNO** | encendido / apagado (apagado) | El compresor escucha otra pista (por ejemplo el bombo) en vez de esta. Ver [sidechain](primeros-pasos.md#sidechain). |

En la pantalla ves la **curva** con un punto que muestra dónde está trabajando, los medidores de **entrada** y **salida**, y la **reducción** en grande.

### Recetas

- **Voz pareja:** Carácter Clean, Detector RMS, Ratio 3:1, Ataque 5 ms, Auto release, reducción de 4–6 dB.
- **Drums con punch:** Carácter Punch, Ratio 4:1, Ataque 20–30 ms (deja pasar el golpe), Release 80 ms.
- **Pegar el master o un bus:** Carácter Glue, Ratio 2:1, Ataque 30 ms, Auto release, Filtro SC 100 Hz, solo 1–2 dB de reducción.
- **Compresión paralela (New York):** Ratio 10:1, Umbral bajo, mucha reducción, y Mix al 30–40 %.
- **Bajo que se aparta del bombo:** SC externo con el bombo, Ratio 6:1, Ataque 1 ms, Release 100 ms. (Para un bombeo marcado y a tempo, usa [Sidechain-R](duck-ready.md).)

### Tips

- Si el sonido pierde el golpe, **alarga el ataque**.
- Si se escucha que "respira" o bombea sin querer, **alarga el release** o activa el **Filtro SC**.
- Más de 10 dB de reducción casi siempre es demasiado, salvo en paralelo.
- Compara siempre con **Auto Gain** o ajustando el Makeup al mismo volumen.

---

## English

### What does it do?

A compressor **turns down the loud parts** so the sound is more even. That way a vocal is always clear, drums feel tighter and a bass doesn't jump in level. Compressor-R has three characters (**Clean**, **Punch**, **Glue**), sidechain and lookahead.

### Start in 1 minute

1. Put it on a vocal, a bass or a drum bus (see [Getting started](primeros-pasos.md)).
2. Lower the **THRESHOLD** until the **REDUCTION** meter shows 3 to 6 dB on the loud parts.
3. Leave **RATIO** at 4:1.
4. Turn on **AUTO GAIN** so the level stays similar and you compare fairly.
5. Try all three **CHARACTER** settings and keep the one you like.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **THRESHOLD** | -60 to 0 dB (-18 dB) | Level above which it starts turning the volume down. Lower = more compression. |
| **RATIO** | 1:1 to ∞:1 (4:1) | How much of what goes over the threshold gets turned down. 2:1 gentle, 4:1 normal, 10:1 hard, ∞:1 limiter. |
| **ATTACK** | 0.05 to 300 ms (10 ms) | How fast it reacts. Fast (1–5 ms) controls peaks; slow (20–50 ms) lets the hit through for punch. |
| **RELEASE** | 5 to 3000 ms (120 ms) | How fast it lets go. Short = louder and more aggressive; long = smoother and more natural. |
| **AUTO RELEASE** | on / off (off) | Program-dependent release: lets go of short hits fast and of sustained sounds slowly. |
| **KNEE** | 0 to 24 dB (6 dB) | Transition around the threshold. 0 dB = hard knee (precise); 12 dB or more = very soft (musical). |
| **MAKEUP** | -12 to +24 dB (0 dB) | Volume you give back after compressing. |
| **AUTO GAIN** | on / off (off) | Turns the volume back up automatically to make up for the compression. |
| **MIX** | 0 to 100 % (100 %) | Blend with the uncompressed signal (parallel compression). |
| **CHARACTER** | Clean, Punch, Glue (Clean) | Clean: transparent. Punch: FET-style, grabs fast and colours. Glue: bus-style, slow and soft, to glue a mix or a group. |
| **DETECTOR** | Peak, RMS (Peak) | Peak reacts to hits. RMS reacts to perceived loudness (smoother, better for vocals and buses). |
| **SC FILTER** | Off, 60, 100, 150, 250, 400 Hz (Off) | Removes lows from what the compressor "hears", so the kick or bass don't make everything pump. |
| **LOOKAHEAD** | Off, 1, 2, 5, 10 ms (Off) | Looks a few milliseconds ahead to catch peaks before they arrive. Adds latency (the DAW compensates). |
| **EXT SIDECHAIN** | on / off (off) | The compressor listens to another track (e.g. the kick) instead of this one. See [sidechain](primeros-pasos.md#sidechain-en). |

The display shows the **curve** with a dot where it's working right now, **input** and **output** meters, and a big **reduction** meter.

### Recipes

- **Even vocal:** Character Clean, Detector RMS, Ratio 3:1, Attack 5 ms, Auto release, 4–6 dB of reduction.
- **Punchy drums:** Character Punch, Ratio 4:1, Attack 20–30 ms (lets the hit through), Release 80 ms.
- **Glue the master or a bus:** Character Glue, Ratio 2:1, Attack 30 ms, Auto release, SC filter 100 Hz, only 1–2 dB of reduction.
- **Parallel (New York) compression:** Ratio 10:1, low Threshold, lots of reduction, Mix at 30–40 %.
- **Bass that ducks under the kick:** Ext sidechain from the kick, Ratio 6:1, Attack 1 ms, Release 100 ms. (For an obvious, in-tempo pump, use [Sidechain-R](duck-ready.md).)

### Tips

- If the sound loses its punch, **lengthen the attack**.
- If you hear it "breathing" or pumping when you don't want it, **lengthen the release** or turn on the **SC filter**.
- More than 10 dB of reduction is almost always too much, except in parallel.
- Always compare with **Auto Gain** or by matching the Makeup to the same loudness.
