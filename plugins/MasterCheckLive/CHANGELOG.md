# Master Check Live

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Medidor de loudness en tiempo real dentro del DAW, con los mismos números que la app Master Check: **LUFS** momentáneo, corto plazo e integrado (ITU-R BS.1770, gating −70 / −10 LU), **LRA** (EBU Tech 3342), **true peak** (sobremuestreo 4x), pico de muestra y **PLR**.
- Tests con señales tipo EBU Tech 3341/3342: seno a −23 dBFS = −23.0 LUFS (±0.1); −36 / −23 / −36 integra −23.0 (la parte floja se descarta por el gate); escalones −20 / −30 dan LRA 10 LU (±1); el true peak de un seno que cae entre muestras se mide bien (±0.1 dB).
- **Plataformas** con los mismos perfiles que Master Check (Spotify, Apple Music, YouTube, Amazon, TIDAL, Deezer, Beatport / club) y la explicación simple: "Spotify va a bajar tu canción 3.2 dB" (test de la lógica de ganancia: las que no suben nunca aplican ganancia positiva; Spotify sube solo hasta −1 dBTP).
- Historial de 60 s del loudness de corto plazo con la línea del objetivo y botón de reinicio.
- El audio pasa sin tocarse (test: salida idéntica a la entrada, bit a bit). Sin latencia.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
