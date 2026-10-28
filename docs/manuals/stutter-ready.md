# Stutter-R — Manual

![Stutter-R](../screenshots/stutter-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Stutter-R **graba un pedacito del sonido y lo repite** al ritmo de la canción: "te-te-te-te-temazo". Es el efecto de los **builds** de EDM, los redobles que aceleran antes del drop, los glitches y el **tape stop** (cuando la música "se frena"). Tiene **8 gestos** (8 efectos distintos) que disparas con botones, con notas MIDI o con un **secuenciador** de pasos.

### Empieza en 1 minuto

1. Ponlo en el bus de toda la canción, en una voz o en unos drums (ver [Primeros pasos](primeros-pasos.md)).
2. Dale play en tu DAW.
3. **Mantén pulsado** un botón de gesto (por ejemplo **STUTTER** o **BUZZ**): suena mientras lo mantienes.
4. Para que suene solo: elige un preset de build (**BUILD 1 COMPÁS**, **BUILD 2 COMPASES**, **RELLENO**, **GLITCH**, **FRENAZO**) y enciende **SECUENCIADOR**.

### Los 8 gestos

| Gesto | Nota MIDI | Qué hace |
| --- | --- | --- |
| **STUTTER** | C | Repite un pedacito al ritmo. |
| **BUZZ** | C# | Redoble que acelera hasta un zumbido. |
| **PITCH +** | D | Las repeticiones suben de tono. |
| **PITCH −** | D# | Las repeticiones bajan de tono. |
| **REVERSA** | E | Las repeticiones suenan al revés. |
| **TAPE STOP** | F | La música se frena hasta pararse, como una cinta. |
| **GATE** | F# | Corta cada repetición (efecto *trance gate*). |
| **FILTRO** | G | Las repeticiones pasan por un filtro que se mueve. |

Funcionan las notas de **cualquier octava**. Haz clic en un gesto para **editarlo** abajo.

### Controles

**De cada gesto** (los cambias después de hacer clic en el gesto):

| Control | Rango | Qué hace |
| --- | --- | --- |
| **DIVISIÓN** | 1/4, 1/8, 1/16, 1/32, 1/64, 1/128 | Tamaño de cada repetición al empezar. |
| **HASTA** | 1/4 a 1/128 | Tamaño al final de la rampa. Igual = sin rampa; más corto = redoble que acelera. |
| **TONO** | -24 a +24 semitonos | Cuánto sube o baja el tono durante la rampa. |
| **FILTRO** | Off, Pasa bajos, Pasa altos | Filtro que barre durante la rampa. |
| **BARRIDO** | 0 a 100 % | Hasta dónde llega el barrido del filtro. |
| **REVERSA** | encendido / apagado | Las repeticiones suenan al revés. |
| **TAPE STOP** | encendido / apagado | La velocidad baja hasta cero durante la rampa. |
| **GATE** | 0 a 100 % | Corta la segunda mitad de cada repetición. |
| **PAN** | 0 a 100 % | Alterna las repeticiones izquierda / derecha. |
| **CRUSH** | 0 a 100 % | Reduce bits y frecuencia de muestreo (sonido lo-fi). |
| **ECO** | 0 a 95 % | Eco de las repeticiones, con realimentación. |

**Generales:**

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **RAMPA** | 1/4 a 16 tiempos (2 tiempos) | Duración de las rampas (división, tono, filtro, tape stop). |
| **CUANTIZAR** | Off, 1/4, 1/8, 1/16 (1/8) | Los gestos disparados a mano o por MIDI esperan a la siguiente línea de esta rejilla, para que entren a tiempo. |
| **SECUENCIADOR** | encendido / apagado (apagado) | Toca el patrón de pasos mientras el DAW reproduce. |
| **COMPASES** | 1, 2, 4 compases (1) | Largo del patrón del secuenciador. |
| **BORRAR PASOS** | botón | Vacía el secuenciador. |
| **MIX** | 0 a 100 % (100 %) | Mezcla con la señal original. |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |

**El secuenciador:** cada casilla es una semicorchea (1/16). Elige un gesto, haz **clic** en las casillas para pintarlo, **arrastra** para rellenar varias y usa **clic derecho** para borrar.

### Recetas

- **Build antes del drop:** preset Build 1 compás o Build 2 compases, Secuenciador encendido, en el bus de toda la canción. Pon el patrón en el último compás antes del drop.
- **Relleno en el último tiempo:** preset Relleno: dos golpes de stutter y reversa al final del compás.
- **Glitch en una voz:** preset Glitch con Mix 70 %.
- **Frenazo antes del drop:** preset Frenazo (tape stop en el último tiempo).
- **Tocarlo en vivo:** manda notas MIDI (C, C#, D...) al plugin y dispara los gestos desde tu teclado o un controlador. En Ableton: pista MIDI → *MIDI To* → la pista del plugin. En FL Studio: un canal *MIDI Out* con el mismo puerto que el *MIDI input port* del plugin.

### Tips

- Con el DAW parado, los gestos arrancan al instante y el secuenciador espera.
- Usa **CUANTIZAR** en 1/8 o 1/16 para que los gestos entren siempre a tiempo.
- Sin gestos sonando, el plugin no toca el sonido y no tiene latencia.
- El preset **LIMPIO** vacía el patrón para empezar el tuyo.

---

## English

### What does it do?

Stutter-R **grabs a little piece of the sound and repeats it** in time with the song: "ba-ba-ba-ba-banger". It's the effect behind EDM **builds**, rolls that speed up before the drop, glitches and the **tape stop** (when the music "brakes"). It has **8 gestures** (8 different effects) you trigger with buttons, MIDI notes or a **step sequencer**.

### Start in 1 minute

1. Put it on the whole song's bus, a vocal or drums (see [Getting started](primeros-pasos.md)).
2. Press play in your DAW.
3. **Hold** a gesture button (e.g. **STUTTER** or **BUZZ**): it plays while you hold it.
4. To make it play by itself: pick a build preset (**1 BAR BUILD**, **2 BAR BUILD**, **DROP FILL**, **GLITCH**, **TAPE OUT**) and turn on **SEQUENCER**.

### The 8 gestures

| Gesture | MIDI note | What it does |
| --- | --- | --- |
| **STUTTER** | C | Repeats a small piece in time. |
| **BUZZ** | C# | A roll that speeds up into a buzz. |
| **PITCH UP** | D | The repeats rise in pitch. |
| **PITCH DOWN** | D# | The repeats fall in pitch. |
| **REVERSE** | E | The repeats play backwards. |
| **TAPE STOP** | F | The music slows down to a stop, like tape. |
| **GATE** | F# | Chops each repeat (*trance gate* effect). |
| **FILTER** | G | The repeats go through a moving filter. |

Notes in **any octave** work. Click a gesture to **edit it** below.

### Controls

**Per gesture** (change them after clicking the gesture):

| Control | Range | What it does |
| --- | --- | --- |
| **DIVISION** | 1/4, 1/8, 1/16, 1/32, 1/64, 1/128 | Length of each repeat at the start. |
| **TO** | 1/4 to 1/128 | Length at the end of the ramp. Same = no ramp; shorter = speeding-up roll. |
| **PITCH** | -24 to +24 semitones | How far the pitch rises or falls over the ramp. |
| **FILTER** | Off, Low pass, High pass | Filter that sweeps over the ramp. |
| **SWEEP** | 0 to 100 % | How far the filter sweep goes. |
| **REVERSE** | on / off | Repeats play backwards. |
| **TAPE STOP** | on / off | Speed drops to zero over the ramp. |
| **GATE** | 0 to 100 % | Cuts the second half of each repeat. |
| **PAN** | 0 to 100 % | Alternates repeats left / right. |
| **CRUSH** | 0 to 100 % | Reduces bits and sample rate (lo-fi sound). |
| **ECO** (echo) | 0 to 95 % | Echo of the repeats, with feedback. |

**Global:**

| Control | Range (default) | What it does |
| --- | --- | --- |
| **RAMP** | 1/4 to 16 beats (2 beats) | Length of the ramps (division, pitch, filter, tape stop). |
| **QUANTIZE** | Off, 1/4, 1/8, 1/16 (1/8) | Hand or MIDI triggers wait for the next line of this grid, so they land in time. |
| **SEQUENCER** | on / off (off) | Plays the step pattern while the DAW is playing. |
| **BARS** | 1, 2, 4 bars (1) | Length of the sequencer pattern. |
| **CLEAR STEPS** | button | Empties the sequencer. |
| **MIX** | 0 to 100 % (100 %) | Blend with the original signal. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |

**The sequencer:** each cell is a sixteenth note (1/16). Pick a gesture, **click** cells to paint it, **drag** to fill several and **right-click** to erase.

### Recipes

- **Build before the drop:** 1 bar build or 2 bar build preset, Sequencer on, on the whole song's bus. Place the pattern in the bar just before the drop.
- **Fill on the last beat:** Drop fill preset: two hits of stutter and reverse at the end of the bar.
- **Glitchy vocal:** Glitch preset with Mix 70 %.
- **Brake before the drop:** Tape out preset (tape stop on the last beat).
- **Play it live:** send MIDI notes (C, C#, D...) to the plugin and trigger gestures from your keyboard or a controller. In Ableton: MIDI track → *MIDI To* → the plugin's track. In FL Studio: a *MIDI Out* channel on the same port as the plugin's *MIDI input port*.

### Tips

- With the DAW stopped, gestures start right away and the sequencer waits.
- Use **QUANTIZE** at 1/8 or 1/16 so gestures always land in time.
- With no gesture playing, the plugin doesn't touch the sound and has no latency.
- The **CLEAR** preset empties the pattern so you can start your own.
