# Gate-R — Manual

![Gate-R](../screenshots/gate-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Gate-R es una **puerta de ruido**: deja pasar el sonido cuando es fuerte y lo **baja o lo silencia cuando es suave**. Sirve para quitar el ruido de fondo de una voz, limpiar lo que se cuela en el micrófono de un bombo o una caja, o cortar las colas de un sonido para que quede más seco.

### Empieza en 1 minuto

1. Ponlo en la pista que tiene ruido (ver [Primeros pasos](primeros-pasos.md)).
2. Elige un punto de partida: **LIMPIAR BOMBO**, **LIMPIAR CAJA** o **RUIDO DE VOZ**.
3. Arrastra la **línea naranja** (el umbral) en la gráfica: ponla justo encima del ruido y debajo de lo que quieres que suene.
4. Si corta el final de las notas, alarga **HOLD** o **RELEASE**.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **UMBRAL** | -80 a 0 dB (-40 dB) | Nivel por encima del cual la puerta se abre. Lo que queda debajo se baja. |
| **RATIO** | 1:1 a Gate (Gate) | 1:2 a 1:10 = expansor suave (baja lo flojo poco a poco). Gate = corta de golpe hasta el Rango. |
| **ATAQUE** | 0.1 a 100 ms (1 ms) | Qué tan rápido abre. Muy rápido para drums (0.1–1 ms), más lento para voces. |
| **HOLD** | 0 a 1000 ms (50 ms) | Cuánto se queda abierta después de que la señal baja, antes de empezar a cerrar. |
| **RELEASE** | 1 a 3000 ms (100 ms) | Qué tan rápido cierra. Corto = seco; largo = natural. |
| **RANGO** | -100 a 0 dB (-80 dB) | Cuánto baja cuando está cerrada. -80 dB = silencio; -20 dB = solo atenúa. |
| **KNEE** | 0 a 24 dB (0 dB) | Suaviza la transición alrededor del umbral. |
| **HISTÉRESIS** | 0 a 12 dB (3 dB) | Abre en el umbral pero solo cierra cuando la señal baja estos dB más. Evita que "tartamudee". |
| **LOOKAHEAD** | Off, 1, 2, 5, 10 ms (Off) | Mira unos milisegundos adelante para abrir antes del golpe (no se come el ataque). Añade latencia. |
| **SC GRAVES** | Off, 60, 120, 250, 500 Hz, 1, 2 kHz (Off) | Quita graves de lo que escucha el detector. Por ejemplo, para que el bombo no abra la puerta de la caja. |
| **SC AGUDOS** | Off, 500 Hz, 1, 2, 4, 8 kHz (Off) | Quita agudos de lo que escucha el detector. Para que los platos no abran la puerta del bombo. |
| **SC EXTERNO** | encendido / apagado (apagado) | El detector escucha otra pista por [sidechain](primeros-pasos.md#sidechain) en vez de esta. |
| **ESCUCHAR** | encendido / apagado (apagado) | Escuchas lo que oye el detector (con sus filtros) para ajustar los cortes. Apágalo después. |

La gráfica muestra el nivel, el umbral (naranja, arrastrable), la línea donde cierra y la ganancia aplicada.

### Recetas

- **Voz grabada en casa con ruido:** preset Ruido de voz: Rango -30 dB (baja el ruido sin silenciarlo) y Release largo. Si aún suena cortado, prueba Ratio 1:4 (expansor).
- **Bombo de batería real:** preset Limpiar bombo: ya trae SC agudos en 2 kHz para que los platos no lo abran. Agrega Lookahead 1 ms para no perder el golpe.
- **Caja:** preset Limpiar caja: ya trae SC graves en 250 Hz para que el bombo no la abra.
- **Cortar la cola de una reverb (gated reverb):** después de la reverb, Ratio Gate, Hold 150 ms, Release 20 ms.
- **Efecto de "corte" rítmico:** SC externo con un hi-hat o un patrón, para que la puerta abra al ritmo.

### Tips

- Si la puerta abre y cierra muy rápido (suena como tartamudeo), sube la **Histéresis** o el **Hold**.
- Si se come el inicio de cada golpe, activa **Lookahead**.
- Con voces, un expansor (Ratio 1:2 a 1:4) suena más natural que un gate duro.
- Usa **ESCUCHAR** para encontrar los filtros correctos, y no olvides apagarlo.

---

## English

### What does it do?

Gate-R is a **noise gate**: it lets the sound through when it's loud and **turns it down or silences it when it's quiet**. Use it to remove background noise from a vocal, clean up bleed in a kick or snare mic, or cut a sound's tail so it's tighter.

### Start in 1 minute

1. Put it on the noisy track (see [Getting started](primeros-pasos.md)).
2. Pick a starting point: **CLEAN KICK**, **CLEAN SNARE** or **VOCAL NOISE**.
3. Drag the **orange line** (the threshold) on the graph: put it just above the noise and below what you want to hear.
4. If it cuts the end of the notes, lengthen **HOLD** or **RELEASE**.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **THRESHOLD** | -80 to 0 dB (-40 dB) | Level above which the gate opens. Whatever stays below gets turned down. |
| **RATIO** | 1:1 to Gate (Gate) | 1:2 to 1:10 = gentle expander (turns quiet parts down gradually). Gate = cuts straight to Range. |
| **ATTACK** | 0.1 to 100 ms (1 ms) | How fast it opens. Very fast for drums (0.1–1 ms), slower for vocals. |
| **HOLD** | 0 to 1000 ms (50 ms) | How long it stays open after the signal drops, before it starts closing. |
| **RELEASE** | 1 to 3000 ms (100 ms) | How fast it closes. Short = tight; long = natural. |
| **RANGE** | -100 to 0 dB (-80 dB) | How much it turns down when closed. -80 dB = silence; -20 dB = just attenuates. |
| **KNEE** | 0 to 24 dB (0 dB) | Softens the transition around the threshold. |
| **HYSTERESIS** | 0 to 12 dB (3 dB) | Opens at the threshold but only closes once the signal drops this many dB further. Stops the "chattering". |
| **LOOKAHEAD** | Off, 1, 2, 5, 10 ms (Off) | Looks a few milliseconds ahead to open before the hit (keeps the attack). Adds latency. |
| **SC LOW CUT** | Off, 60, 120, 250, 500 Hz, 1, 2 kHz (Off) | Removes lows from what the detector hears. E.g. so the kick doesn't open the snare's gate. |
| **SC HIGH CUT** | Off, 500 Hz, 1, 2, 4, 8 kHz (Off) | Removes highs from what the detector hears. So the cymbals don't open the kick's gate. |
| **EXT SIDECHAIN** | on / off (off) | The detector listens to another track via [sidechain](primeros-pasos.md#sidechain-en) instead of this one. |
| **LISTEN** | on / off (off) | Hear what the detector hears (with its filters) to set the cuts. Turn it off afterwards. |

The graph shows the level, the threshold (orange, draggable), the closing line and the gain being applied.

### Recipes

- **Noisy home-recorded vocal:** Vocal noise preset: Range -30 dB (lowers the noise without silencing it) and a long Release. If it still sounds choppy, try Ratio 1:4 (expander).
- **Real drum kit kick:** Clean kick preset: it already has SC high cut at 2 kHz so the cymbals don't open it. Add Lookahead 1 ms to keep the hit.
- **Snare:** Clean snare preset: it already has SC low cut at 250 Hz so the kick doesn't open it.
- **Cut a reverb tail (gated reverb):** after the reverb, Ratio Gate, Hold 150 ms, Release 20 ms.
- **Rhythmic "chop" effect:** Ext sidechain from a hi-hat or a pattern, so the gate opens in rhythm.

### Tips

- If the gate opens and closes too fast (it stutters), raise **Hysteresis** or **Hold**.
- If it eats the start of each hit, turn on **Lookahead**.
- On vocals, an expander (Ratio 1:2 to 1:4) sounds more natural than a hard gate.
- Use **LISTEN** to find the right filters, and don't forget to turn it off.
