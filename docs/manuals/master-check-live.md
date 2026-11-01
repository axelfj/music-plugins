# Meter-R — Manual

![Meter-R](../screenshots/master-check-live.png)

[Español](#español) · [English](#english) · [Primeros pasos / Getting started](primeros-pasos.md) · [Todos los manuales / All manuals](README.md)

---

## Español

### ¿Qué hace?

Meter-R **mide qué tan fuerte suena tu canción** y te dice qué va a pasar cuando la subas a Spotify, Apple Music, YouTube, el club y más. **No cambia el sonido**: solo mide. Usa los mismos números que la app [Master Check](https://github.com/axelfj/master-check).

### Empieza en 1 minuto

1. Ponlo **al final** del master, después del limitador (ver [Primeros pasos](primeros-pasos.md)).
2. Elige la **PLATAFORMA** donde vas a publicar.
3. Ve al inicio de la canción, pulsa **REINICIAR** y deja sonar la canción **completa**.
4. Lee el mensaje: por ejemplo, *"Spotify va a bajar tu canción 3.2 dB"*. También te avisa si el true peak está muy alto.

### Lo que ves

| Medida | Qué significa |
| --- | --- |
| **INTEGRADO** (LUFS) | Volumen de toda la canción desde el último reinicio. Es el número que miran las plataformas. |
| **CORTO PLAZO** (LUFS) | Volumen de los últimos 3 segundos. Útil para comparar partes (verso contra drop). |
| **MOMENTÁNEO** (LUFS) | Volumen de este instante. |
| **RANGO (LRA)** | Cuánto varía el volumen a lo largo de la canción. En EDM, 3–6 LU es normal. |
| **MÁX. CORTO PLAZO** | El momento más fuerte de la canción. |
| **TRUE PEAK** (dBTP) | El pico real, incluso entre muestras. Las plataformas piden −1 dBTP o menos. |
| **PLR** | Pico menos volumen: cuánta pegada queda. Menos de 8 dB = muy aplastado. |
| **Historial** | Gráfica de los últimos 60 segundos con la línea del objetivo de la plataforma. |

### Controles

| Control | Opciones | Qué hace |
| --- | --- | --- |
| **PLATAFORMA** | Spotify, Apple Music, YouTube, Amazon Music, TIDAL, Deezer, Beatport / club | Para qué destino estás revisando. Cambia el objetivo y el mensaje. |
| **REINICIAR** | botón | Empieza la medición de cero. Hazlo al inicio de la canción. |

### Recetas

- **¿Mi canción está lista para Spotify?** Plataforma Spotify, reinicia y deja sonar todo. Si dice que la va a bajar mucho, estás más fuerte de lo necesario y pierdes pegada sin ganar nada.
- **Comparar con una canción que te gusta:** pon esa canción en una pista, con Meter-R al final, y compara el Integrado y el PLR con los de tu master.
- **Para el club:** Plataforma Beatport / club. Lo normal es entre −10 y −6 LUFS.

### Tips

- El **Integrado** necesita unos segundos para aparecer, y solo es real si dejas sonar la canción entera.
- Si el **true peak** pasa de 0 dBTP, tu canción va a distorsionar al convertirse a MP3 o streaming: baja el techo del limitador.
- Como no toca el audio y no tiene latencia, puedes dejarlo puesto siempre.

---

## English

### What does it do?

Meter-R **measures how loud your song is** and tells you what will happen when you upload it to Spotify, Apple Music, YouTube, the club and more. **It doesn't change the sound**: it only measures. It uses the same numbers as the [Master Check](https://github.com/axelfj/master-check) app.

### Start in 1 minute

1. Put it **at the end** of the master, after the limiter (see [Getting started](primeros-pasos.md)).
2. Choose the **PLATFORM** you'll release on.
3. Go to the start of the song, press **RESET** and let the **whole** song play.
4. Read the message: for example, *"Spotify will turn your song down 3.2 dB"*. It also warns you if the true peak is too high.

### What you see

| Reading | What it means |
| --- | --- |
| **INTEGRATED** (LUFS) | Loudness of the whole song since the last reset. This is the number the platforms look at. |
| **SHORT-TERM** (LUFS) | Loudness of the last 3 seconds. Useful to compare sections (verse vs. drop). |
| **MOMENTARY** (LUFS) | Loudness right now. |
| **RANGE (LRA)** | How much the loudness varies through the song. In EDM, 3–6 LU is normal. |
| **MAX SHORT-TERM** | The loudest moment of the song. |
| **TRUE PEAK** (dBTP) | The real peak, even between samples. Platforms ask for −1 dBTP or less. |
| **PLR** | Peak minus loudness: how much punch is left. Under 8 dB = very squashed. |
| **History** | Graph of the last 60 seconds with the platform's target line. |

### Controls

| Control | Options | What it does |
| --- | --- | --- |
| **PLATFORM** | Spotify, Apple Music, YouTube, Amazon Music, TIDAL, Deezer, Beatport / club | Which destination you're checking for. Changes the target and the message. |
| **RESET** | button | Starts measuring from zero. Do it at the start of the song. |

### Recipes

- **Is my song ready for Spotify?** Platform Spotify, reset and let it all play. If it says it will turn it down a lot, you're louder than needed and losing punch for nothing.
- **Compare with a song you like:** put that song on a track with Meter-R at the end, and compare its Integrated and PLR with your master's.
- **For the club:** Platform Beatport / club. −10 to −6 LUFS is usual.

### Tips

- **Integrated** needs a few seconds to appear, and it's only accurate if you let the whole song play.
- If the **true peak** goes over 0 dBTP, your song will distort when converted to MP3 or streaming: lower the limiter's ceiling.
- Since it doesn't touch the audio and has no latency, you can leave it on all the time.
