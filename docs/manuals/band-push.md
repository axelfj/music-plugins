# Multiband-R — Manual

![Multiband-R](../screenshots/band-push.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Multiband-R divide el sonido en **3 bandas** (graves, medios y agudos) y a cada una le **baja lo fuerte y le sube lo suave**. El resultado es un sonido más denso, brillante y "en tu cara", el famoso sonido **OTT** del EDM. Todo se controla con una perilla principal: **PROFUNDIDAD**.

### Empieza en 1 minuto

1. Ponlo en un synth, un pad, un bus de drums o un vocal chop (ver [Primeros pasos](primeros-pasos.md)).
2. Empieza con **PROFUNDIDAD** en 30–50 %.
3. Si suena muy aplastado o con mucho ruido, baja la Profundidad o el **MIX**.
4. Ajusta **SALIDA** para que el volumen sea parecido con el plugin encendido y apagado.

### Controles

**Generales:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **PROFUNDIDAD** | 0 a 100 % (50 %) | La perilla principal. 0 % no hace nada; 100 % aplasta todo (el sonido OTT completo). |
| **TIEMPO** | 0.1x a 10x (1x) | Escala el ataque y el release de las tres bandas. Menos = más agresivo y con más distorsión; más = más suave. |
| **UMBRAL** | -48 a 0 dB (-24 dB) | El nivel al que se empuja cada banda: lo fuerte baja hasta aquí y lo suave sube hasta aquí. |
| **ENTRADA** | -24 a +24 dB (0 dB) | Ganancia antes del procesado. Subirla es como bajar el umbral. |
| **SALIDA** | -24 a +24 dB (0 dB) | Ganancia después del procesado. |
| **MIX** | 0 a 100 % (100 %) | Mezcla con la señal sin procesar. 100 % = solo procesada. |
| **CORTE GRAVE** | 40 a 1000 Hz (120 Hz) | Frecuencia que separa graves y medios. |
| **CORTE AGUDO** | 500 Hz a 12 kHz (2.5 kHz) | Frecuencia que separa medios y agudos. |

**Por banda (GRAVES, MEDIOS, AGUDOS):**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ARRIBA** | 0 a 100 % (100 %) | Cuánto sube las partes suaves de esta banda (compresión hacia arriba). |
| **ABAJO** | 0 a 100 % (100 %) | Cuánto baja las partes fuertes de esta banda (compresión hacia abajo). |
| **GANANCIA** | -24 a +24 dB (0 dB) | Volumen de la banda después del procesado. |

Cada banda tiene un medidor que muestra si la está subiendo (+) o bajando (-).

### Recetas

- **Synth lead o pad más brillante:** Profundidad 40 %, Mix 60 %.
- **OTT clásico al máximo:** Profundidad 100 %, Tiempo 1x, Mix 30–50 %. El Mix es el truco: aplastas mucho pero mezclas poco.
- **Drums más densos sin perder el golpe:** Profundidad 25 %, Tiempo 2x, Arriba de Graves 50 %.
- **Solo levantar detalles, sin aplastar:** Abajo 0 % en las tres bandas y Arriba 100 %.
- **Domar agudos que pican:** Ganancia de Agudos -3 dB, Arriba de Agudos 40 %.

### Tips

- Multiband-R sube las partes suaves, así que también sube el **ruido y las colas de reverb**. Si notas ruido, baja ARRIBA.
- La subida está limitada a +24 dB para no levantar el ruido de fondo sin fin.
- Úsalo en sonidos sueltos más que en el master.
- Cambiar los cortes no deja huecos: las bandas suman plano.

---

## English

### What does it do?

Multiband-R splits the sound into **3 bands** (lows, mids and highs) and on each one it **turns the loud parts down and the quiet parts up**. The result is a denser, brighter, "in your face" sound: the famous **OTT** sound in EDM. Everything is driven by one main knob: **DEPTH**.

### Start in 1 minute

1. Put it on a synth, a pad, a drum bus or a vocal chop (see [Getting started](primeros-pasos.md)).
2. Start with **DEPTH** at 30–50 %.
3. If it sounds too squashed or noisy, lower Depth or the **MIX**.
4. Set **OUTPUT** so the level is similar with the plugin on and off.

### Controls

**Global:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **DEPTH** | 0 to 100 % (50 %) | The main knob. 0 % does nothing; 100 % squashes everything (the full OTT sound). |
| **TIME** | 0.1x to 10x (1x) | Scales the attack and release of all three bands. Less = more aggressive with more distortion; more = smoother. |
| **THRESHOLD** | -48 to 0 dB (-24 dB) | Level every band is pushed towards: loud parts come down to it, quiet parts come up to it. |
| **INPUT** | -24 to +24 dB (0 dB) | Gain before processing. Turning it up is like lowering the threshold. |
| **OUTPUT** | -24 to +24 dB (0 dB) | Gain after processing. |
| **MIX** | 0 to 100 % (100 %) | Blend with the unprocessed signal. 100 % = processed only. |
| **LOW SPLIT** | 40 to 1000 Hz (120 Hz) | Frequency that splits lows from mids. |
| **HIGH SPLIT** | 500 Hz to 12 kHz (2.5 kHz) | Frequency that splits mids from highs. |

**Per band (LOW, MID, HIGH):**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **UP** | 0 to 100 % (100 %) | How much it lifts the quiet parts of this band (upward compression). |
| **DOWN** | 0 to 100 % (100 %) | How much it turns down the loud parts of this band (downward compression). |
| **GAIN** | -24 to +24 dB (0 dB) | The band's level after processing. |

Each band has a meter showing whether it's lifting (+) or reducing (-).

### Recipes

- **Brighter synth lead or pad:** Depth 40 %, Mix 60 %.
- **Classic OTT all the way:** Depth 100 %, Time 1x, Mix 30–50 %. The Mix is the trick: squash hard but blend in a little.
- **Denser drums without losing punch:** Depth 25 %, Time 2x, Low Up 50 %.
- **Only bring out details, no squashing:** Down 0 % on all three bands and Up 100 %.
- **Tame harsh highs:** High Gain -3 dB, High Up 40 %.

### Tips

- Multiband-R lifts the quiet parts, so it also lifts **noise and reverb tails**. If you hear noise, lower UP.
- The lift is capped at +24 dB so it doesn't raise the noise floor forever.
- Use it on individual sounds more than on the master.
- Moving the splits leaves no holes: the bands sum flat.
