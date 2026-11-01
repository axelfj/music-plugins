# De-Esser-R — Manual

![De-Esser-R](../screenshots/ess-clean.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Cuando grabas una voz, las letras **"s"**, **"z"**, **"ch"** y a veces la **"t"** pueden sonar muy fuertes y molestas (sibilancia). De-Esser-R **baja solo esas "s"** en el momento en que aparecen y deja el resto de la voz igual.

### Empieza en 1 minuto

1. Ponlo en la pista de la voz, después del EQ y el compresor (ver [Primeros pasos](primeros-pasos.md)).
2. Elige el preset según la voz: **VOZ GRAVE**, **VOZ MEDIA** o **VOZ AGUDA**.
3. Baja el **UMBRAL** hasta que el medidor de **REDUCCIÓN** se mueva solo en las "s".
4. Activa **ESCUCHAR**: deberías oír solo "sss". Si oyes la voz entera, mueve el rango o sube el umbral. Después apágalo.

### Controles

| Control | Rango (inicio) | Qué hace |
| --- | --- | --- |
| **DESDE** | 2 a 12 kHz (4.5 kHz) | Dónde empieza la zona de la "s". Voces graves: 3–4 kHz; agudas: 6 kHz. |
| **HASTA** | 3 a 16 kHz (9 kHz) | Dónde termina la zona de la "s". Normalmente 8–12 kHz. |
| **UMBRAL** | -60 a 0 dB (-24 dB) | Nivel de la zona a partir del cual se baja. Bájalo hasta que solo se muevan las "s". |
| **REDUCCIÓN** | 0 a 24 dB (10 dB) | Lo máximo que puede bajar. 6–10 dB suele bastar; más suena a ceceo. |
| **ATAQUE** | 0.1 a 20 ms (1 ms) | Qué tan rápido reacciona. Muy rápido (o usa Lookahead) para no dejar pasar el inicio de la "s". |
| **RELEASE** | 10 a 500 ms (80 ms) | Qué tan rápido vuelve. Corto = natural en voces rápidas; largo = más suave. |
| **MODO** | Banda, Todo (Banda) | Banda: solo baja la zona de la "s" (transparente). Todo: baja la voz completa un momento (clásico, más notorio). |
| **LOOKAHEAD** | Off, 1, 2, 5 ms (Off) | Mira unos milisegundos adelante para bajar la "s" desde su primer instante. Añade latencia. |
| **ESCUCHAR** | encendido / apagado (apagado) | Escuchas solo lo que se está quitando. Apágalo después. |

En la pantalla ves el espectro de la voz con el **rango** marcado: puedes **arrastrarlo** con el mouse en vez de usar las perillas.

### Recetas

- **Voz de rap o pop:** preset según tu voz, Modo Banda, Reducción 6–8 dB.
- **Voz muy brillante que pica:** Modo Banda, Hasta 12 kHz, Lookahead 1 ms.
- **Coros o dobles con muchas "s":** Modo Todo, Reducción 8 dB, Release 60 ms.
- **Hi-hats o platos que pican:** también funciona: Desde 6 kHz, Hasta 14 kHz, Modo Banda.

### Tips

- Si la voz empieza a sonar como si tuviera la lengua trabada (ceceo), **baja la Reducción**.
- Por debajo del umbral no cambia absolutamente nada.
- Si después del de-esser agregas brillo con un EQ o un saturador, las "s" pueden volver: pon un segundo De-Esser-R suave al final de la cadena.

---

## English

### What does it do?

When you record a vocal, **"s"**, **"z"**, **"sh"** and sometimes **"t"** sounds can come out too loud and annoying (sibilance). De-Esser-R **turns down only those "s" sounds** the moment they appear and leaves the rest of the vocal alone.

### Start in 1 minute

1. Put it on the vocal track, after the EQ and compressor (see [Getting started](primeros-pasos.md)).
2. Pick the preset for the voice: **LOW VOICE**, **MID VOICE** or **HIGH VOICE**.
3. Lower the **THRESHOLD** until the **REDUCTION** meter only moves on the "s" sounds.
4. Turn on **LISTEN**: you should hear only "sss". If you hear the whole vocal, move the range or raise the threshold. Then turn it off.

### Controls

| Control | Range (default) | What it does |
| --- | --- | --- |
| **FROM** | 2 to 12 kHz (4.5 kHz) | Where the "s" zone starts. Low voices: 3–4 kHz; high voices: 6 kHz. |
| **TO** | 3 to 16 kHz (9 kHz) | Where the "s" zone ends. Usually 8–12 kHz. |
| **THRESHOLD** | -60 to 0 dB (-24 dB) | Level of the zone above which it turns down. Lower it until only the "s" sounds move. |
| **RANGE** | 0 to 24 dB (10 dB) | The most it can turn down. 6–10 dB is usually enough; more sounds lispy. |
| **ATTACK** | 0.1 to 20 ms (1 ms) | How fast it reacts. Very fast (or use Lookahead) so the start of the "s" doesn't slip through. |
| **RELEASE** | 10 to 500 ms (80 ms) | How fast it recovers. Short = natural on fast vocals; long = smoother. |
| **MODE** | Band, Broadband (Band) | Band: only turns down the "s" zone (transparent). Broadband: turns the whole vocal down for a moment (classic, more audible). |
| **LOOKAHEAD** | Off, 1, 2, 5 ms (Off) | Looks a few milliseconds ahead to catch the "s" from its very first instant. Adds latency. |
| **LISTEN** | on / off (off) | Hear only what's being removed. Turn it off afterwards. |

The display shows the vocal's spectrum with the **range** marked: you can **drag it** with the mouse instead of using the knobs.

### Recipes

- **Rap or pop vocal:** preset for your voice, Band mode, Range 6–8 dB.
- **Very bright, harsh vocal:** Band mode, To 12 kHz, Lookahead 1 ms.
- **Backing vocals or doubles with lots of "s":** Broadband mode, Range 8 dB, Release 60 ms.
- **Harsh hi-hats or cymbals:** it works there too: From 6 kHz, To 14 kHz, Band mode.

### Tips

- If the vocal starts sounding lispy, **lower the Range**.
- Below the threshold absolutely nothing changes.
- If you add brightness with an EQ or saturator after the de-esser, the "s" sounds can come back: put a second, gentle De-Esser-R at the end of the chain.
