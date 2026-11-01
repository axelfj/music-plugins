# EQ-R — Manual

![EQ-R](../screenshots/shape-eq.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

EQ-R es un **ecualizador**: sube o baja frecuencias. Sirve para quitar lo que molesta (graves que retumban, un pitido, agudos que pican) y para resaltar lo bueno (el golpe del bombo, el brillo de la voz). Tiene hasta **12 bandas** y un **analizador** que te muestra el sonido en tiempo real.

### Empieza en 1 minuto

1. Ponlo en cualquier pista (ver [Primeros pasos](primeros-pasos.md)).
2. **Doble clic** en la gráfica para crear una banda donde hiciste clic.
3. **Arrastra** el punto: izquierda/derecha cambia la frecuencia, arriba/abajo la ganancia.
4. Gira la **rueda del mouse** sobre el punto para hacer la banda más ancha o más angosta (Q).
5. **Clic derecho** en el punto para cambiar el tipo (Bell, Shelf, Cut...) o quitar la banda.
6. Activa **BYPASS** para escuchar con y sin EQ.

### Controles

**Por banda** (selecciona un punto para verlos abajo):

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ACTIVA / APAGADA** | encendido / apagado (apagada) | Activa o desactiva esta banda. |
| **Tipo** | Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch, Band Pass, Tilt, All Pass (Bell) | Bell: sube o baja alrededor de una frecuencia. Shelf: todo lo de arriba o lo de abajo. Cut: elimina graves o agudos. Notch: quita una frecuencia exacta. Tilt: inclina todo el espectro. |
| **FRECUENCIA** | 10 Hz a 30 kHz | Frecuencia central de la banda. |
| **GANANCIA** | -30 a +30 dB (0 dB) | Cuánto sube (+) o baja (-) la banda. |
| **Q** | 0.025 a 40 (1.0) | Ancho de la banda. Q bajo = ancho y musical. Q alto = quirúrgico. En cortes de 12 dB controla la resonancia. |
| **Pendiente** | 6, 12, 18, 24, 36, 48 dB/oct (12 dB) | Qué tan fuerte corta un filtro Cut. 12 dB es suave; 48 dB es casi un muro. |
| **Canal** | Stereo, Left, Right, Mid, Side (Stereo) | Dónde actúa: todo, solo izquierda o derecha, solo el centro (Mid) o solo los lados (Side). |

**Generales:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **SALIDA** | -24 a +24 dB (0 dB) | Volumen final después del EQ. |
| **AUTO GAIN** | encendido / apagado (apagado) | Compensa el volumen que la curva sube o baja, para comparar sin engañarte. |
| **BYPASS** | encendido / apagado (apagado) | Escucha la señal sin el EQ. |
| **ANALIZADOR** | Off, Pre, Post, Pre + Post (Pre + Post) | Pre muestra el sonido antes del EQ, Post después. Apágalo para ahorrar CPU. |
| **Escala** | ±30 dB / ±12 dB | Escala de la gráfica. ±12 dB para ver mejor los ajustes finos. |

### Recetas

- **Limpiar una voz:** Low Cut en 80–100 Hz, 24 dB. Si suena "nasal", baja 2–3 dB con un Bell en 800 Hz–1 kHz. Para brillo, High Shelf +2 dB en 10 kHz.
- **Bombo con más golpe:** Bell +3 dB en 60 Hz (cuerpo) y Bell +2 dB en 3–4 kHz (click).
- **Hacer espacio para el bombo en el bajo:** Bell -3 dB en la frecuencia del bombo, Q 1.5.
- **Quitar un pitido:** Notch o Bell con Q alto (10–20). Busca el pitido subiendo mucho la ganancia y moviendo la frecuencia; cuando lo encuentres, bájala.
- **Graves en mono:** Low Cut a 120 Hz con Canal **Side** quita los graves de los lados y deja el centro intacto.

### Tips

- **Bajar es mejor que subir.** Antes de subir algo, prueba bajar lo que sobra.
- Con el analizador ves el sonido, pero decide con el oído.
- Si subes mucho y todo suena más fuerte, usa **Auto Gain** para saber si de verdad mejoró.
- Un Low Cut en casi todas las pistas (menos bombo y bajo) limpia mucho la mezcla.

---

## English

### What does it do?

EQ-R is an **equalizer**: it turns frequencies up or down. Use it to remove what bothers you (boomy lows, a whistle, harsh highs) and to bring out the good stuff (the kick's punch, the vocal's sparkle). It has up to **12 bands** and an **analyzer** that shows the sound in real time.

### Start in 1 minute

1. Put it on any track (see [Getting started](primeros-pasos.md)).
2. **Double-click** the graph to create a band where you clicked.
3. **Drag** the point: left/right changes the frequency, up/down the gain.
4. Use the **mouse wheel** over the point to make the band wider or narrower (Q).
5. **Right-click** the point to change its type (Bell, Shelf, Cut...) or remove it.
6. Turn on **BYPASS** to hear with and without the EQ.

### Controls

**Per band** (select a point to see them below):

| Control | Range (default) | What it does |
| --- | --- | --- |
| **ON / OFF** | on / off (off) | Turns this band on or off. |
| **Type** | Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch, Band Pass, Tilt, All Pass (Bell) | Bell: boost or cut around a frequency. Shelf: everything above or below. Cut: removes lows or highs. Notch: removes one exact frequency. Tilt: tilts the whole spectrum. |
| **FREQUENCY** | 10 Hz to 30 kHz | Centre frequency of the band. |
| **GAIN** | -30 to +30 dB (0 dB) | How much the band boosts (+) or cuts (-). |
| **Q** | 0.025 to 40 (1.0) | Band width. Low Q = wide and musical. High Q = surgical. On 12 dB cuts it controls the resonance. |
| **Slope** | 6, 12, 18, 24, 36, 48 dB/oct (12 dB) | How steeply a Cut filter cuts. 12 dB is gentle; 48 dB is almost a wall. |
| **Placement** | Stereo, Left, Right, Mid, Side (Stereo) | Where it acts: everything, left or right only, the centre (Mid) or the sides (Side). |

**Global:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **OUTPUT** | -24 to +24 dB (0 dB) | Final level after the EQ. |
| **AUTO GAIN** | on / off (off) | Compensates the loudness the curve adds or removes, so you compare fairly. |
| **BYPASS** | on / off (off) | Hear the signal without the EQ. |
| **ANALYZER** | Off, Pre, Post, Pre + Post (Pre + Post) | Pre shows the sound before the EQ, Post after. Turn it off to save CPU. |
| **Scale** | ±30 dB / ±12 dB | Graph scale. ±12 dB to see fine moves better. |

### Recipes

- **Clean up a vocal:** Low Cut at 80–100 Hz, 24 dB. If it sounds "nasal", cut 2–3 dB with a Bell at 800 Hz–1 kHz. For sparkle, High Shelf +2 dB at 10 kHz.
- **Punchier kick:** Bell +3 dB at 60 Hz (body) and Bell +2 dB at 3–4 kHz (click).
- **Make room for the kick in the bass:** Bell -3 dB at the kick's frequency, Q 1.5.
- **Remove a whistle:** Notch, or a Bell with high Q (10–20). Find it by boosting a lot and sweeping the frequency; when you hear it jump out, turn it down.
- **Mono lows:** a Low Cut at 120 Hz with Placement **Side** removes lows from the sides and leaves the centre untouched.

### Tips

- **Cutting beats boosting.** Before boosting something, try cutting what's in the way.
- The analyzer shows you the sound, but decide with your ears.
- If you boost a lot and everything just gets louder, use **Auto Gain** to know whether it really got better.
- A Low Cut on almost every track (except kick and bass) cleans up a mix a lot.
