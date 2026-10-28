# Filter-R — Manual

![Filter-R](../screenshots/filter-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Un filtro **deja pasar unas frecuencias y quita otras**. Con un pasa-bajos el sonido se vuelve oscuro y apagado; al abrirlo, brilla. Moverlo es la base de muchísimos efectos de EDM: **builds** que suben, **wobbles** de dubstep, el sonido **ácido** del 303 o una voz de **teléfono**. Filter-R tiene **dos filtros** (A y B), un **LFO** que los mueve al ritmo y un **envolvente** que los abre con cada golpe (auto-wah).

### Empieza en 1 minuto

1. Ponlo en un synth, un loop o el bus de toda la canción (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un preset: **SUBIDA (BUILD)**, **WOBBLE**, **ÁCIDO** o **TELÉFONO**.
3. **Arrastra el punto A** en la gráfica: izquierda/derecha cambia el **CORTE**, arriba/abajo la **RESONANCIA**.
4. Para que se mueva solo, sube **LFO** del filtro A y elige la **FIGURA** (con **SYNC** encendido).

### Controles

**Filtro A y Filtro B** (cada uno tiene los suyos):

| Control | Rango (inicio A / B) | Qué hace |
| --- | --- | --- |
| **ON** | encendido / apagado (A encendido / B apagado) | Activa o apaga este filtro. |
| **Tipo** | Low-pass, High-pass, Band-pass, Notch (A: Low-pass / B: High-pass) | Pasa-bajos (quita agudos), pasa-altos (quita graves), pasa-banda (solo el centro) o notch (quita el centro). |
| **Pendiente** | 12 dB, 24 dB (A: 24 dB / B: 12 dB) | 12 dB suave; 24 dB más marcada (estilo sinte). |
| **CORTE** | 20 Hz a 20 kHz (A: 1.2 kHz / B: 150 Hz) | Frecuencia de corte del filtro. |
| **RESONANCIA** | 0 a 100 % (A: 40 % / B: 20 %) | Realza el corte. Arriba del 70 % silba. |
| **LFO** | -5 a +5 octavas (0) | Cuánto mueve el LFO el corte. Negativo lo invierte. |
| **ENV** | -5 a +5 octavas (0) | Cuánto abre el envolvente el corte cuando entra un golpe fuerte (auto-wah). |

**Generales:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **RUTA** | Series, Parallel (Series) | Serie: B filtra lo que sale de A. Paralelo: A y B se suman. |
| **CARÁCTER** | Clean, Analog (Clean) | Clean: respuesta exacta. Analog: la resonancia satura suavemente al subir el nivel. |
| **DRIVE** | 0 a 36 dB (0 dB) | Saturación antes de los filtros: más agresivo. |
| **MIX** | 0 a 100 % (100 %) | Mezcla entre la señal original y la filtrada. |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |

**LFO** (mueve los cortes solo):

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **SYNC** | encendido / apagado (encendido) | Sigue el tempo del DAW. Apagado: usa la velocidad en Hz. |
| **FIGURA** | 4 bars a 1/16, con D y T (1 bar) | Duración de un ciclo del LFO. |
| **VELOCIDAD** | 0.02 a 20 Hz (0.5 Hz) | Velocidad del LFO (con SYNC apagado). |
| **FORMA** | Sine, Triangle, Saw, Square, Random (Sine) | Forma del movimiento: seno, triángulo, sierra (baja), cuadrada o random. |

**ENVOLVENTE** (abre el filtro con cada golpe):

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ATAQUE** | 1 a 200 ms (10 ms) | Qué tan rápido reacciona a un golpe. |
| **CAÍDA** | 10 a 2000 ms (200 ms) | Qué tan rápido vuelve cuando la señal baja. |
| **SENSIBILIDAD** | 0 a 36 dB (12 dB) | Súbela si la señal es muy baja y el envolvente no reacciona. |

### Recetas

- **Build antes del drop:** preset Subida (build). Automatiza el **CORTE** de A desde 300 Hz hasta 20 kHz durante 8 o 16 compases.
- **Wobble de dubstep:** preset Wobble en un bajo. Cambia la **FIGURA** del LFO (1/4, 1/8, 1/8 T) en distintas partes para variar el ritmo.
- **Bajo ácido (303):** preset Ácido. Sube la Resonancia al 80–90 % y mueve el corte en vivo.
- **Voz de teléfono o radio:** preset Teléfono (pasa-altos + pasa-bajos dejando solo el centro).
- **"Quitar los graves" antes del drop:** filtro A High-pass, corte en 400 Hz durante el último compás; vuelve a 20 Hz en el drop.
- **Auto-wah en guitarra o clavinet:** ENV del filtro A en +3, Tipo Band-pass, Resonancia 60 %.

### Tips

- Resonancia alta + corte bajo puede subir mucho el volumen: vigila la Salida.
- En el master, usa solo la pendiente de 12 dB y movimientos lentos.
- Con **Analog** y un poco de **Drive**, el filtro suena más "de hardware".

---

## English

### What does it do?

A filter **lets some frequencies through and removes others**. With a low-pass the sound gets dark and muffled; open it and it shines. Moving it is the basis of tons of EDM effects: rising **builds**, dubstep **wobbles**, the **acid** 303 sound or a **telephone** voice. Filter-R has **two filters** (A and B), an **LFO** that moves them in time and an **envelope** that opens them on each hit (auto-wah).

### Start in 1 minute

1. Put it on a synth, a loop or the whole song's bus (see [Getting started](primeros-pasos.md)).
2. Pick a preset: **SWEEP BUILD**, **WOBBLE**, **ACID** or **TELEPHONE**.
3. **Drag point A** on the graph: left/right changes the **CUTOFF**, up/down the **RESONANCE**.
4. To make it move by itself, raise filter A's **LFO** and pick the **DIVISION** (with **SYNC** on).

### Controls

**Filter A and Filter B** (each has its own):

| Control | Range (default A / B) | What it does |
| --- | --- | --- |
| **ON** | on / off (A on / B off) | Turns this filter on or off. |
| **Type** | Low-pass, High-pass, Band-pass, Notch (A: Low-pass / B: High-pass) | Low-pass (removes highs), high-pass (removes lows), band-pass (only the middle) or notch (removes the middle). |
| **Slope** | 12 dB, 24 dB (A: 24 dB / B: 12 dB) | 12 dB gentle; 24 dB steeper (synth style). |
| **CUTOFF** | 20 Hz to 20 kHz (A: 1.2 kHz / B: 150 Hz) | The filter's cutoff frequency. |
| **RESONANCE** | 0 to 100 % (A: 40 % / B: 20 %) | Boosts the cutoff. Above 70 % it whistles. |
| **LFO** | -5 to +5 octaves (0) | How far the LFO moves the cutoff. Negative inverts it. |
| **ENV** | -5 to +5 octaves (0) | How far the envelope opens the cutoff on a loud hit (auto-wah). |

**Global:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **ROUTING** | Series, Parallel (Series) | Series: B filters A's output. Parallel: A and B are summed. |
| **CHARACTER** | Clean, Analog (Clean) | Clean: exact response. Analog: the resonance saturates softly as the level rises. |
| **DRIVE** | 0 to 36 dB (0 dB) | Saturation before the filters: bites harder. |
| **MIX** | 0 to 100 % (100 %) | Blend between the original and filtered signal. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |

**LFO** (moves the cutoffs on its own):

| Control | Range (default) | What it does |
| --- | --- | --- |
| **SYNC** | on / off (on) | Follows the DAW tempo. Off: uses the rate in Hz. |
| **DIVISION** | 4 bars to 1/16, with D and T (1 bar) | Length of one LFO cycle. |
| **RATE** | 0.02 to 20 Hz (0.5 Hz) | LFO speed (with SYNC off). |
| **SHAPE** | Sine, Triangle, Saw, Square, Random (Sine) | Movement shape: sine, triangle, saw (down), square or random. |

**ENVELOPE** (opens the filter on each hit):

| Control | Range (default) | What it does |
| --- | --- | --- |
| **ATTACK** | 1 to 200 ms (10 ms) | How fast it reacts to a hit. |
| **RELEASE** | 10 to 2000 ms (200 ms) | How fast it falls back when the signal drops. |
| **SENSITIVITY** | 0 to 36 dB (12 dB) | Raise it if the signal is quiet and the envelope doesn't react. |

### Recipes

- **Build before the drop:** Sweep build preset. Automate filter A's **CUTOFF** from 300 Hz up to 20 kHz over 8 or 16 bars.
- **Dubstep wobble:** Wobble preset on a bass. Change the LFO **DIVISION** (1/4, 1/8, 1/8 T) in different sections to vary the rhythm.
- **Acid bass (303):** Acid preset. Push Resonance to 80–90 % and move the cutoff live.
- **Telephone or radio vocal:** Telephone preset (high-pass + low-pass leaving only the middle).
- **"Pull the lows" before the drop:** filter A High-pass, cutoff at 400 Hz for the last bar; back to 20 Hz on the drop.
- **Auto-wah on guitar or clav:** filter A ENV at +3, Type Band-pass, Resonance 60 %.

### Tips

- High resonance + low cutoff can boost the level a lot: watch the Output.
- On the master, stick to the 12 dB slope and slow moves.
- With **Analog** and a bit of **Drive**, the filter sounds more "hardware".
