# Saturator-R — Manual

![Saturator-R](../screenshots/warm-drive.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Saturator-R **calienta y ensucia** el sonido agregando armónicos, como una cinta, un tubo o un amplificador. Un poco hace que el bajo se escuche en parlantes pequeños y que la voz brille; mucho lo convierte en distorsión. Puede saturar **de 1 a 4 bandas** por separado, cada una con su propia curva.

### Empieza en 1 minuto

1. Ponlo en un bajo, un bus de drums o una voz (ver [Primeros pasos](primeros-pasos.md)).
2. Prueba un punto de partida: **CALENTAR BAJO**, **BRILLO EN VOCES** o **EMPUJE DE DRUMS**.
3. Sube o baja el **DRIVE** a gusto.
4. Deja **AUTO GAIN** encendido: así el volumen no cambia y escuchas solo el color.

### Controles

**Generales:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **BANDAS** | 1, 2, 3, 4 (1) | Cuántas bandas de frecuencia se saturan por separado. 1 = toda la señal junta. |
| **CORTE 1 / 2 / 3** | 40 Hz–2 kHz (150 Hz) / 200 Hz–8 kHz (800 Hz) / 1–16 kHz (4 kHz) | Frecuencias que separan las bandas. |
| **AUTO GAIN** | encendido / apagado (encendido) | Compensa el volumen que añade cada curva, para comparar sin engañarte. |
| **ENTRADA** | -24 a +24 dB (0 dB) | Ganancia antes de saturar. Subirla es como subir todos los drives. |
| **SALIDA** | -24 a +24 dB (0 dB) | Ganancia final. |
| **MIX** | 0 a 100 % (100 %) | Mezcla con la señal limpia (saturación paralela). |
| **CALIDAD** | 1x, 2x, 4x (2x) | Sobremuestreo. Más = menos brillo digital falso (aliasing) pero más CPU. 2x va bien casi siempre. |

**Por banda:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **Tipo** | Tape, Tube, Transistor, Soft Clip, Rectify (Tape) | Tape: suave y redondo. Tube: cálido, con armónicos pares. Transistor: más duro y brillante. Soft Clip: limpio hasta que recorta. Rectify: sucio, con una octava arriba. |
| **DRIVE** | 0 a 36 dB (6 dB) | Cuánto empuja la señal contra la curva. 0 dB apenas la toca; 36 dB la aplasta. |
| **TONO** | 1 kHz a Off (Off) | Filtro que oscurece los armónicos nuevos de esta banda. |
| **MIX** | 0 a 100 % (100 %) | Mezcla de esta banda: saturada contra limpia. |

La luz en cada curva te muestra cuánto se está saturando.

### Recetas

- **808 que se escucha en el celular:** 2 bandas, Corte 1 en 120 Hz. Banda 1 Tape con Drive 3 dB (el sub queda limpio); Banda 2 Tube con Drive 12–18 dB. Los armónicos de la banda 2 hacen que el bajo "aparezca" en parlantes pequeños.
- **Voz con brillo y presencia:** 1 banda, Tube, Drive 6–9 dB, Tono 10 kHz, Mix 40 %.
- **Drums más gordos:** Tape, Drive 9 dB, Mix 60 %.
- **Distorsión agresiva para un lead:** Transistor o Rectify, Drive 24 dB, Tono 6 kHz para que no pique.

### Tips

- Con Auto Gain encendido el volumen no sube, así que si "no oyes nada", sube más el Drive.
- **Rectify** es sucio a propósito. Úsalo con Mix bajo.
- Para un bajo, no satures mucho la banda más grave: el sub se vuelve confuso.
- Si los agudos suenan ásperos, baja el **TONO** o sube la **CALIDAD** a 4x.

---

## English

### What does it do?

Saturator-R **warms up and dirties** the sound by adding harmonics, like tape, a tube or an amp. A little makes the bass audible on small speakers and the vocal shine; a lot turns it into distortion. It can saturate **1 to 4 bands** separately, each with its own curve.

### Start in 1 minute

1. Put it on a bass, a drum bus or a vocal (see [Getting started](primeros-pasos.md)).
2. Try a starting point: **WARM BASS**, **VOCAL SHINE** or **DRUM PUNCH**.
3. Turn the **DRIVE** up or down to taste.
4. Leave **AUTO GAIN** on: the level stays the same and you hear only the colour.

### Controls

**Global:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **BANDS** | 1, 2, 3, 4 (1) | How many frequency bands are saturated separately. 1 = the whole signal together. |
| **SPLIT 1 / 2 / 3** | 40 Hz–2 kHz (150 Hz) / 200 Hz–8 kHz (800 Hz) / 1–16 kHz (4 kHz) | Frequencies that split the bands. |
| **AUTO GAIN** | on / off (on) | Compensates the level each curve adds, so you compare fairly. |
| **INPUT** | -24 to +24 dB (0 dB) | Gain before saturating. Turning it up is like turning up every drive. |
| **OUTPUT** | -24 to +24 dB (0 dB) | Final gain. |
| **MIX** | 0 to 100 % (100 %) | Blend with the clean signal (parallel saturation). |
| **QUALITY** | 1x, 2x, 4x (2x) | Oversampling. More = less fake digital brightness (aliasing) but more CPU. 2x is fine almost always. |

**Per band:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **Type** | Tape, Tube, Transistor, Soft Clip, Rectify (Tape) | Tape: smooth and round. Tube: warm, with even harmonics. Transistor: harder and brighter. Soft Clip: clean until it clips. Rectify: dirty, with an octave up. |
| **DRIVE** | 0 to 36 dB (6 dB) | How hard the signal is pushed into the curve. 0 dB barely touches it; 36 dB crushes it. |
| **TONE** | 1 kHz to Off (Off) | Filter that darkens this band's new harmonics. |
| **MIX** | 0 to 100 % (100 %) | This band's blend: saturated vs. clean. |

The light on each curve shows how hard it's saturating.

### Recipes

- **808 you can hear on a phone:** 2 bands, Split 1 at 120 Hz. Band 1 Tape with 3 dB Drive (the sub stays clean); Band 2 Tube with 12–18 dB Drive. Band 2's harmonics make the bass "show up" on small speakers.
- **Vocal with shine and presence:** 1 band, Tube, Drive 6–9 dB, Tone 10 kHz, Mix 40 %.
- **Fatter drums:** Tape, Drive 9 dB, Mix 60 %.
- **Aggressive lead distortion:** Transistor or Rectify, Drive 24 dB, Tone 6 kHz so it isn't harsh.

### Tips

- With Auto Gain on the level doesn't go up, so if you "can't hear anything", push the Drive further.
- **Rectify** is dirty on purpose. Use it with a low Mix.
- On a bass, don't saturate the lowest band too much: the sub gets muddy.
- If the highs sound rough, lower the **TONE** or raise **QUALITY** to 4x.
