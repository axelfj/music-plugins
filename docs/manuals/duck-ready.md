# Sidechain-R — Manual

![Sidechain-R](../screenshots/duck-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Sidechain-R **baja el volumen al ritmo de la canción** y lo vuelve a subir. Es el famoso "bombeo" del EDM y el house: el bajo o los pads se agachan en cada bombo para dejarle espacio. No necesitas conectar nada: por defecto sigue el tempo de tu DAW solo. También puede dispararse con notas MIDI o con el bombo real por sidechain.

### Empieza en 1 minuto

1. Ponlo en el bajo, los pads o los acordes (ver [Primeros pasos](primeros-pasos.md)).
2. Dale play. Con **VELOCIDAD** en 1/4 ya bombea en cada bombo de un 4/4.
3. Elige una curva lista en el menú de presets (**Clásico**, **Suave**, **Corto**).
4. Ajusta **PROFUNDIDAD** (cuánto baja) y **RELEASE** (cuánto tarda en volver).
5. Si el bombo suena tapado, sube un poco el **OFFSET** o el **HOLD**.

También puedes **arrastrar la curva**: arriba/abajo cambia la profundidad, izquierda/derecha el release. Doble clic vuelve a la curva clásica.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **PROFUNDIDAD** | 0 a 100 % (85 %) | Cuánto baja el volumen en cada golpe. 100 % = silencio total. |
| **ATAQUE** | 0.5 a 50 % del ciclo (2 %) | Qué tan rápido baja. Corto = golpe seco; más largo = suave. |
| **HOLD** | 0 a 90 % del ciclo (5 %) | Cuánto tiempo se queda abajo antes de volver a subir. |
| **RELEASE** | 1 a 100 % del ciclo (55 %) | Cuánto tarda en volver al volumen normal. Es lo que da la sensación de bombeo. |
| **CURVA** | -1 a +1 (0.3) | Forma de la bajada y la subida. Negativo = empieza rápido; positivo = empieza lento; 0 = recta. |
| **OFFSET** | -50 a +50 % (0 %) | Mueve toda la curva dentro del ciclo. Útil para que el bombo respire antes de que baje el volumen. |
| **VELOCIDAD** | 1 bar, 1/2, 1/4, 1/4 T, 1/8 D, 1/8, 1/8 T, 1/16, 1/16 T, 1/32 (1/4) | Cada cuánto se repite la curva. 1/4 = en cada bombo de un 4/4. |
| **DISPARO** | Tempo, MIDI, Sidechain (Tempo) | Tempo: sigue la rejilla del DAW. MIDI: cada nota reinicia la curva. Sidechain: el bombo que entra por sidechain la reinicia. |
| **UMBRAL SC** | -60 a 0 dB (-24 dB) | Solo con disparo Sidechain: el nivel del bombo a partir del cual se dispara. |
| **BANDA** | Full, Low, High (Full) | Full: todo el sonido. Low: solo bajan los graves (espacio para el bombo sin apagar lo demás). High: solo los agudos. |
| **Crossover** | 40 a 2000 Hz (200 Hz) | Solo en Low o High: la frecuencia que separa graves de agudos. |

Los tiempos están en **% del ciclo**: si cambias la velocidad, la forma de la curva se mantiene.

### Recetas

- **Bombeo clásico de EDM en el bajo:** Velocidad 1/4, preset Clásico, Profundidad 90 %, Release 55 %.
- **Pads que respiran suave:** preset Suave, Profundidad 50 %, Curva +0.5.
- **Solo los graves del bajo se apartan:** Banda Low, Crossover 150 Hz, Profundidad 100 %. Los agudos del bajo siguen sonando.
- **Seguir un bombo con ritmo irregular:** Disparo Sidechain, manda el bombo al sidechain ([cómo](primeros-pasos.md#sidechain)) y ajusta el Umbral SC hasta que dispare en cada golpe.
- **Disparar con un patrón MIDI:** Disparo MIDI y manda notas al plugin; cada nota reinicia la curva.
- **Efecto "trance gate":** Velocidad 1/16, Profundidad 100 %, Ataque 1 %, Hold 30 %, Release 10 %.

### Tips

- Con disparo Tempo no hace falta ruteo: es lo más fácil y siempre queda perfecto en la rejilla.
- Si escuchas un clic al cambiar de compás, sube un poco el Ataque.
- Pon el mismo Sidechain-R en varias pistas (bajo, pads, FX) para que todo bombee junto.

---

## English

### What does it do?

Sidechain-R **turns the volume down in time with the song** and brings it back up. It's the famous EDM and house "pump": the bass or pads duck on every kick to make room. You don't need to route anything: by default it follows your DAW's tempo on its own. It can also be triggered by MIDI notes or by the real kick via sidechain.

### Start in 1 minute

1. Put it on the bass, pads or chords (see [Getting started](primeros-pasos.md)).
2. Press play. With **RATE** at 1/4 it already pumps on every kick of a 4/4 beat.
3. Pick a ready-made curve from the preset menu (**Classic**, **Soft**, **Short**).
4. Adjust **DEPTH** (how far it drops) and **RELEASE** (how long it takes to come back).
5. If the kick sounds buried, raise the **OFFSET** or the **HOLD** a little.

You can also **drag the curve**: up/down changes depth, left/right the release. Double-click returns to the classic curve.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **DEPTH** | 0 to 100 % (85 %) | How far the volume drops on every hit. 100 % = complete silence. |
| **ATTACK** | 0.5 to 50 % of the cycle (2 %) | How fast it drops. Short = tight hit; longer = softer. |
| **HOLD** | 0 to 90 % of the cycle (5 %) | How long it stays down before coming back up. |
| **RELEASE** | 1 to 100 % of the cycle (55 %) | How long it takes to come back to full volume. This is what gives the pump feel. |
| **CURVE** | -1 to +1 (0.3) | Shape of the drop and the rise. Negative = fast start; positive = slow start; 0 = straight. |
| **OFFSET** | -50 to +50 % (0 %) | Shifts the whole curve within the cycle. Useful to let the kick breathe before the volume drops. |
| **RATE** | 1 bar, 1/2, 1/4, 1/4 T, 1/8 D, 1/8, 1/8 T, 1/16, 1/16 T, 1/32 (1/4) | How often the curve repeats. 1/4 = on every kick in 4/4. |
| **TRIGGER** | Tempo, MIDI, Sidechain (Tempo) | Tempo: follows the DAW grid. MIDI: every note restarts the curve. Sidechain: the kick on the sidechain input restarts it. |
| **SC THRESHOLD** | -60 to 0 dB (-24 dB) | Sidechain trigger only: the kick level above which it fires. |
| **BAND** | Full, Low, High (Full) | Full: the whole sound. Low: only the lows duck (room for the kick without killing the rest). High: only the highs. |
| **Crossover** | 40 to 2000 Hz (200 Hz) | Low or High only: the frequency that splits lows from highs. |

Times are in **% of the cycle**: if you change the rate, the curve keeps its shape.

### Recipes

- **Classic EDM bass pump:** Rate 1/4, Classic preset, Depth 90 %, Release 55 %.
- **Softly breathing pads:** Soft preset, Depth 50 %, Curve +0.5.
- **Only the bass's lows duck:** Band Low, Crossover 150 Hz, Depth 100 %. The bass's highs keep playing.
- **Follow a kick with an irregular rhythm:** Trigger Sidechain, send the kick to the sidechain ([how](primeros-pasos.md#sidechain-en)) and set SC threshold until it fires on every hit.
- **Trigger from a MIDI pattern:** Trigger MIDI and send notes to the plugin; every note restarts the curve.
- **"Trance gate" effect:** Rate 1/16, Depth 100 %, Attack 1 %, Hold 30 %, Release 10 %.

### Tips

- With Tempo trigger there's no routing: it's the easiest option and always lands perfectly on the grid.
- If you hear a click when the bar changes, raise the Attack a little.
- Put the same Sidechain-R on several tracks (bass, pads, FX) so everything pumps together.
