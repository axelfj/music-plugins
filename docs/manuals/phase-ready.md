# Phaser-R — Manual

![Phaser-R](../screenshots/phase-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Un phaser hace que el sonido **"gire" o "nade"**: unas muescas recorren las frecuencias de arriba a abajo y le dan movimiento. Es el efecto clásico de guitarras de los 70, de pads que se mueven lento y de synths que respiran al ritmo.

### Empieza en 1 minuto

1. Ponlo en una guitarra, un pad, un synth o unos hi-hats (ver [Primeros pasos](primeros-pasos.md)).
2. Prueba un preset: **GUITARRA CLÁSICA**, **PAD LENTO** o **SYNTH 1/4**.
3. Ajusta la **VELOCIDAD** del barrido o activa **SYNC** para que vaya al ritmo.
4. Sube **FEEDBACK** para un sonido más metálico y marcado.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ETAPAS** | 4, 6, 8, 10, 12 (6) | Más etapas = más muescas y un sonido más denso. Cambia sin clics. |
| **SYNC** | encendido / apagado (apagado) | Sigue el tempo del DAW con la figura elegida. Apagado: usa la velocidad en Hz. |
| **FIGURA** | 4 bars a 1/16, con D y T (1 bar) | Duración de un ciclo del barrido en el tempo del DAW. |
| **VELOCIDAD** | 0.01 a 10 Hz (0.5 Hz) | Velocidad del barrido (con SYNC apagado). |
| **PROFUNDIDAD** | 0 a 100 % (70 %) | Cuánto se mueve el barrido: hasta ±2.5 octavas alrededor del centro. |
| **FEEDBACK** | -95 a +95 % (30 %) | Realimenta la salida: más resonante y metálico. Negativo cambia dónde caen las muescas. |
| **CENTRO** | 100 Hz a 5 kHz (800 Hz) | Frecuencia alrededor de la que gira el barrido. |
| **ESTÉREO** | 0 a 100 % (50 %) | Desfase entre izquierda y derecha, hasta 180°. 0 = mono. |
| **FORMA** | Sine, Triangle, Random (Sine) | Forma del barrido: seno (suave), triángulo (lineal) o random suave. |
| **MIX** | 0 a 100 % (50 %) | 50 % = phaser clásico con muescas. 100 % = solo el giro de fase (sin muescas). |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |

La pantalla muestra las muescas moviéndose sobre el espectro y la forma del barrido.

### Recetas

- **Guitarra de los 70:** preset Guitarra clásica (4 etapas, 0.4 Hz, en mono). Sube Estéreo si la quieres más ancha.
- **Pad que se mueve lento:** preset Pad lento (8 etapas, 0.08 Hz, muy lento). Si quieres que vaya al tempo, activa Sync en 4 bars.
- **Synth que respira al ritmo:** preset Synth 1/4 (sincronizado a negras). Sube Feedback a 60 % para que se marque más.
- **Hi-hats con movimiento:** Centro 3 kHz, Sync 2 bars, Mix 40 %.
- **Efecto "jet" intenso:** 12 etapas, Feedback 85 %, Profundidad 100 %.

### Tips

- El **Mix al 50 %** es el sonido de phaser clásico. Más o menos que eso suaviza las muescas.
- Feedback alto + muchas etapas puede sonar muy resonante: baja la Salida si pica.
- **ESTÉREO** al máximo da mucha anchura, pero revisa en mono que no desaparezca.

---

## English

### What does it do?

A phaser makes the sound **"swirl" or "swim"**: notches sweep up and down through the frequencies and give it movement. It's the classic sound of 70s guitars, slowly moving pads and synths that breathe in time.

### Start in 1 minute

1. Put it on a guitar, a pad, a synth or hi-hats (see [Getting started](primeros-pasos.md)).
2. Try a preset: **CLASSIC GUITAR**, **SLOW PAD** or **SYNTH 1/4**.
3. Set the sweep **RATE**, or turn on **SYNC** so it follows the beat.
4. Raise **FEEDBACK** for a more metallic, obvious sound.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **STAGES** | 4, 6, 8, 10, 12 (6) | More stages = more notches and a thicker sound. Switches without clicks. |
| **SYNC** | on / off (off) | Follows the DAW tempo with the chosen division. Off: uses the rate in Hz. |
| **DIVISION** | 4 bars to 1/16, with D and T (1 bar) | Length of one sweep cycle in DAW tempo. |
| **RATE** | 0.01 to 10 Hz (0.5 Hz) | Sweep speed (with SYNC off). |
| **DEPTH** | 0 to 100 % (70 %) | How far the sweep moves: up to ±2.5 octaves around the centre. |
| **FEEDBACK** | -95 to +95 % (30 %) | Feeds the output back in: more resonant and metallic. Negative moves where the notches fall. |
| **CENTER** | 100 Hz to 5 kHz (800 Hz) | Frequency the sweep revolves around. |
| **SPREAD** | 0 to 100 % (50 %) | Offset between left and right, up to 180°. 0 = mono. |
| **SHAPE** | Sine, Triangle, Random (Sine) | Sweep shape: sine (smooth), triangle (linear) or smooth random. |
| **MIX** | 0 to 100 % (50 %) | 50 % = classic phaser with notches. 100 % = phase shift only (no notches). |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |

The display shows the notches moving over the spectrum and the sweep's shape.

### Recipes

- **70s guitar:** Classic guitar preset (4 stages, 0.4 Hz, mono). Raise Spread if you want it wider.
- **Slowly moving pad:** Slow pad preset (8 stages, 0.08 Hz, very slow). To lock it to the tempo, turn on Sync at 4 bars.
- **Synth breathing in time:** Synth 1/4 preset (synced to quarter notes). Raise Feedback to 60 % to make it more obvious.
- **Hi-hats with movement:** Center 3 kHz, Sync 2 bars, Mix 40 %.
- **Intense "jet" effect:** 12 stages, Feedback 85 %, Depth 100 %.

### Tips

- **Mix at 50 %** is the classic phaser sound. More or less than that softens the notches.
- High feedback + many stages can get very resonant: lower the Output if it gets harsh.
- **SPREAD** all the way up gives a lot of width, but check in mono that it doesn't disappear.
