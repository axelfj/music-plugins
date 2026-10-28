# Clipper-R — Manual

![Clipper-R](../screenshots/clip-ready.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Clipper-R **corta los picos** del sonido para que no pasen de un nivel máximo (el *techo*). Así puedes subir el volumen de tus drums, tu bajo o tu canción entera sin que se rompa. Si lo usas suave, casi no se nota; si lo empujas, suena más fuerte y más agresivo.

### Empieza en 1 minuto

1. Ponlo en la pista de drums, en el bajo o al final del master (ver [Primeros pasos](primeros-pasos.md)).
2. Deja **TECHO** en -0.3 dB.
3. Sube **ENTRADA** poco a poco mientras suena el tema. Mira el medidor: ves cuánto se está recortando.
4. Activa **LINK** para comparar al mismo volumen. Si suena igual o mejor, vas bien.
5. Activa **DELTA** un momento: escuchas solo lo que se quita. Si suena a distorsión fuerte, baja la entrada.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **ENTRADA** | -12 a +24 dB (0 dB) | Cuánto empujas el sonido contra el clipper. Más entrada = más recorte y más volumen. |
| **TECHO** | -24 a 0 dB (-0.3 dB) | El nivel máximo. Nada pasa de aquí. En el master usa -0.3 dB o menos. |
| **SUAVIDAD** | 0 a 100 % (35 %) | 0 % = clip duro, más agresivo. 100 % = curva suave, más cálida, empieza a saturar antes. |
| **SALIDA** | -24 a +12 dB (0 dB) | Volumen final después del clipper. |
| **MIX** | 0 a 100 % (100 %) | Mezcla entre el original y el recortado (clipping en paralelo). |
| **CALIDAD** | 1x, 2x, 4x, 8x (4x) | Sobremuestreo. Evita que los agudos se ensucien. 4x es buen balance, 8x para el master, 1x gasta menos CPU. |
| **LINK** | encendido / apagado (apagado) | Baja la salida lo mismo que subes la entrada, para comparar a igual volumen. |
| **DELTA** | encendido / apagado (apagado) | Escuchas solo lo que el clipper quita. Es para revisar, no para dejarlo encendido. |
| **TECHO ESTRICTO** | encendido / apagado (encendido) | Encendido: nada sale por encima del techo, aunque subas Salida o uses Mix. Apagado: un poco más limpio, pero se puede pasar. DELTA nunca se limita. |

En la pantalla ves la **curva** (cómo se transforma el sonido) y los medidores de **entrada**, **salida** y **cuánto se recorta**.

### Recetas

- **Drums con más golpe:** en el bus de drums, Entrada +3 a +6 dB, Suavidad 20 %, Calidad 4x. Usa Link para comparar.
- **Bajo 808 más presente:** Entrada +4 dB, Suavidad 60 % (más cálido), Mix 70 %.
- **Master más fuerte:** al final de la cadena, Techo -0.3 dB, Calidad 8x, Techo estricto encendido, Entrada +1 a +3 dB. Si necesitas más volumen, mejor usa [Limiter-R](peak-ready.md) después.

### Tips

- Recortar 1 a 3 dB casi no se escucha. Más de 6 dB empieza a sonar distorsionado.
- Si el sonido pierde el "golpe", baja la Entrada o sube la Suavidad.
- En el master, deja siempre **Techo estricto** encendido.
- Si tu computadora va lenta, baja la Calidad a 2x mientras produces y súbela al exportar.

---

## English

### What does it do?

Clipper-R **cuts off the peaks** of a sound so they never go above a maximum level (the *ceiling*). That lets you make your drums, bass or whole song louder without breaking. Used gently, you barely hear it; pushed hard, it sounds louder and more aggressive.

### Start in 1 minute

1. Put it on your drum track, your bass, or at the end of the master (see [Getting started](primeros-pasos.md)).
2. Leave **CEILING** at -0.3 dB.
3. Turn **INPUT** up slowly while the song plays. Watch the meter to see how much is being clipped.
4. Turn on **LINK** to compare at the same loudness. If it sounds the same or better, you're good.
5. Turn on **DELTA** for a moment: you hear only what gets removed. If it sounds like heavy distortion, lower the input.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **INPUT** | -12 to +24 dB (0 dB) | How hard you push the sound into the clipper. More input = more clipping and more loudness. |
| **CEILING** | -24 to 0 dB (-0.3 dB) | The maximum level. Nothing goes above it. On the master, use -0.3 dB or lower. |
| **SOFTNESS** | 0 to 100 % (35 %) | 0 % = hard clip, more aggressive. 100 % = soft curve, warmer, starts saturating earlier. |
| **OUTPUT** | -24 to +12 dB (0 dB) | Final volume after the clipper. |
| **MIX** | 0 to 100 % (100 %) | Blend between the original and the clipped sound (parallel clipping). |
| **QUALITY** | 1x, 2x, 4x, 8x (4x) | Oversampling. Keeps the highs clean. 4x is a good balance, 8x for the master, 1x uses the least CPU. |
| **LINK** | on / off (off) | Lowers the output as much as you raise the input, so you compare at equal loudness. |
| **DELTA** | on / off (off) | Hear only what the clipper removes. It's for checking, not for leaving on. |
| **STRICT CEILING** | on / off (on) | On: nothing leaves above the ceiling, even with Output or Mix. Off: slightly cleaner, but it can go over. DELTA is never limited. |

The display shows the **curve** (how the sound is shaped) and meters for **input**, **output** and **how much is clipped**.

### Recipes

- **Punchier drums:** on the drum bus, Input +3 to +6 dB, Softness 20 %, Quality 4x. Use Link to compare.
- **More present 808:** Input +4 dB, Softness 60 % (warmer), Mix 70 %.
- **Louder master:** at the end of the chain, Ceiling -0.3 dB, Quality 8x, Strict ceiling on, Input +1 to +3 dB. If you need more loudness, use [Limiter-R](peak-ready.md) after it.

### Tips

- Clipping 1 to 3 dB is almost inaudible. More than 6 dB starts to sound distorted.
- If the sound loses its punch, lower the Input or raise the Softness.
- On the master, always keep **Strict ceiling** on.
- If your computer is slow, use Quality 2x while producing and raise it when you export.
