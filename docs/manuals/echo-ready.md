# Delay-R — Manual

![Delay-R](../screenshots/echo-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Delay-R hace **ecos**: repite el sonido un rato después, como cuando gritas en una montaña. Los ecos pueden ir al ritmo de la canción, saltar de izquierda a derecha (**ping-pong**) y sonar limpios, como cinta vieja o analógicos y oscuros.

### Empieza en 1 minuto

1. Ponlo en una voz, un synth o una guitarra (ver [Primeros pasos](primeros-pasos.md)).
2. Deja **SYNC** encendido y elige la **FIGURA**: 1/4 o 1/8 son buenos para empezar.
3. Sube **FEEDBACK** para más repeticiones (40 % = unas cuantas).
4. Ajusta **MIX**: entre 15 y 35 % suele ser suficiente.
5. Prueba los presets **SLAPBACK**, **1/8 PUNTEADO VOZ** y **DUB**.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **SYNC** | encendido / apagado (encendido) | Sigue el tempo del DAW con la figura elegida. Apagado: usa el tiempo en ms. |
| **FIGURA** | 1/1, 1/2, 1/2 T, 1/4 D, 1/4, 1/4 T, 1/8 D, 1/8, 1/8 T, 1/16 D, 1/16, 1/16 T, 1/32 (1/8) | Tiempo del eco en figuras del tempo (D = puntillo, T = tresillo). |
| **TIEMPO** | 1 a 2000 ms (375 ms) | Tiempo del eco en milisegundos (con SYNC apagado). |
| **MODO** | Stereo, Ping-Pong (Stereo) | Stereo: cada lado repite lo suyo. Ping-Pong: los ecos saltan de izquierda a derecha. |
| **ESTILO** | Digital, Tape, Analog (Digital) | Digital: limpio. Tape: saturación, agudos recortados y wow. Analog: más oscuro, estilo BBD. |
| **CAMBIO** | Crossfade, Glide (Crossfade) | Qué pasa al cambiar el tiempo: Crossfade (sin clics ni cambio de tono) o Glide (el tono se dobla, como en una cinta). |
| **FEEDBACK** | 0 a 110 % (40 %) | Cuánto de cada repetición vuelve a entrar. Más de 100 % se auto-oscila (con límite suave, no explota). |
| **CORTE GRAVES** | 20 Hz a 2 kHz (Off) | Quita graves de las repeticiones para que no ensucien el bajo. |
| **CORTE AGUDOS** | 500 Hz a 20 kHz (Off) | Quita agudos: cada eco es más oscuro que el anterior, como una cinta. |
| **DIFUSIÓN** | 0 a 100 % (0 %) | Emborrona un poco cada repetición para que suene menos digital. |
| **WOW** | 0 a 100 % (0 %) | Vaivén lento del tiempo, como una cinta vieja. |
| **DUCKING** | 0 a 100 % (0 %) | Baja los ecos mientras suena la señal original y los sube en los huecos. Ideal para voces. |
| **MIX** | 0 a 100 % (35 %) | Mezcla entre la señal original y los ecos. |
| **SALIDA** | -24 a +12 dB (0 dB) | Ganancia final. |

La pantalla muestra cada repetición: qué tan fuerte es y de qué lado suena.

### Recetas

- **Slapback de rock o rap:** preset Slapback: un eco corto de 95 ms estilo Tape, casi sin feedback.
- **Voz pop/EDM:** preset 1/8 punteado voz: 1/8 D en Ping-Pong, Corte graves 250 Hz y Ducking 60 %. Los ecos se oyen en los huecos y no tapan la letra.
- **Dub / reggae:** preset Dub: 1/4 en Tape con Feedback 65 %, Corte agudos 2.5 kHz y Wow. Pon Cambio en Glide y mueve la Figura en vivo.
- **Eco que crece en un build:** automatiza Feedback hacia 100 % y Mix hacia arriba en los últimos compases.
- **Ancho estéreo sutil:** Ping-Pong, 1/16, Feedback 20 %, Mix 15 %.

### Tips

- Corta siempre los graves de los ecos (200–400 Hz) para que la mezcla no se embarre.
- El **Ducking** es tu mejor amigo con voces: ecos grandes sin que se pierda la letra.
- Si lo usas en un canal de envío (send/return), pon **MIX en 100 %**.
- Feedback más de 100 % crece solo: úsalo para efectos y bájalo a tiempo.

---

## English

### What does it do?

Delay-R makes **echoes**: it repeats the sound a moment later, like shouting in the mountains. The echoes can follow the song's tempo, bounce from left to right (**ping-pong**) and sound clean, like old tape, or dark and analog.

### Start in 1 minute

1. Put it on a vocal, a synth or a guitar (see [Getting started](primeros-pasos.md)).
2. Leave **SYNC** on and pick the **DIVISION**: 1/4 or 1/8 are good to start.
3. Raise **FEEDBACK** for more repeats (40 % = a few).
4. Set **MIX**: 15 to 35 % is usually enough.
5. Try the presets **SLAPBACK**, **DOTTED 1/8 VOCAL** and **DUB**.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **SYNC** | on / off (on) | Follows the DAW tempo with the chosen division. Off: uses the time in ms. |
| **DIVISION** | 1/1, 1/2, 1/2 T, 1/4 D, 1/4, 1/4 T, 1/8 D, 1/8, 1/8 T, 1/16 D, 1/16, 1/16 T, 1/32 (1/8) | Echo time in note values (D = dotted, T = triplet). |
| **TIME** | 1 to 2000 ms (375 ms) | Echo time in milliseconds (with SYNC off). |
| **MODE** | Stereo, Ping-Pong (Stereo) | Stereo: each side repeats its own. Ping-Pong: the echoes bounce left to right. |
| **STYLE** | Digital, Tape, Analog (Digital) | Digital: clean. Tape: saturation, rolled-off highs and wow. Analog: darker, BBD style. |
| **TIME CHANGE** | Crossfade, Glide (Crossfade) | What happens when the time changes: Crossfade (no clicks, no pitch change) or Glide (the pitch bends, like tape). |
| **FEEDBACK** | 0 to 110 % (40 %) | How much of each repeat is fed back in. Above 100 % it self-oscillates (softly limited, it won't blow up). |
| **LOW CUT** | 20 Hz to 2 kHz (Off) | Removes lows from the repeats so they don't muddy the bass. |
| **HIGH CUT** | 500 Hz to 20 kHz (Off) | Removes highs: each echo is darker than the last, like tape. |
| **DIFFUSION** | 0 to 100 % (0 %) | Smears each repeat a little so it sounds less digital. |
| **WOW** | 0 to 100 % (0 %) | Slow wobble of the time, like old tape. |
| **DUCKING** | 0 to 100 % (0 %) | Turns the echoes down while the original plays and up in the gaps. Great on vocals. |
| **MIX** | 0 to 100 % (35 %) | Blend between the original and the echoes. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final gain. |

The display shows each repeat: how loud it is and which side it's on.

### Recipes

- **Rock or rap slapback:** Slapback preset: one short 95 ms Tape echo, almost no feedback.
- **Pop/EDM vocal:** Dotted 1/8 vocal preset: 1/8 D in Ping-Pong, Low cut 250 Hz and Ducking 60 %. The echoes fill the gaps without covering the lyrics.
- **Dub / reggae:** Dub preset: 1/4 on Tape with 65 % Feedback, High cut 2.5 kHz and Wow. Set Time change to Glide and move the Division live.
- **Echo that grows in a build:** automate Feedback towards 100 % and Mix upwards over the last bars.
- **Subtle stereo width:** Ping-Pong, 1/16, Feedback 20 %, Mix 15 %.

### Tips

- Always cut the lows from the echoes (200–400 Hz) so the mix doesn't get muddy.
- **Ducking** is your best friend on vocals: big echoes without losing the words.
- If you use it on a send/return channel, set **MIX to 100 %**.
- Feedback above 100 % keeps growing on its own: use it for effects and pull it back in time.
