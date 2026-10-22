# Punch Shape

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Transient shaper: **Ataque** (±24 dB) sube o suaviza los golpes y **Sustain** (±24 dB) alarga o acorta las colas.
- El detector compara dos envolventes en dB, así que el resultado no depende del nivel de entrada: un golpe a −20 dBFS recibe exactamente la misma forma que uno a −6 dBFS (test: < 0.05 dB de diferencia).
- **Velocidad** (cuánto dura un "golpe" para el detector, 5–100 ms) y **Cola** (cuánto tiempo sigue escuchando el sustain, 50–2000 ms).
- Filtro del detector: Todo / Graves (pasa-bajos 150 Hz) / Agudos (pasa-altos 2 kHz), para trabajar solo el bombo o solo los platos (test: un golpe de 5 kHz encima de un bajo sostenido no mueve el detector en modo Graves, y al revés en Agudos). Como el detector no depende del nivel, el filtro decide a qué parte de la mezcla reacciona; no silencia un sonido que suena solo.
- Un solo detector para los dos canales: la imagen estéreo no se mueve.
- **Clip**: clipper suave opcional a −0.3 dBFS en la salida (la misma curva que Clip Ready, sin sobremuestreo) para que los ataques subidos no pasen de 0 dBFS (test).
- Con Ataque y Sustain en 0 la salida es idéntica a la entrada, bit a bit (test). Sin latencia.
- Presets: Más pegada, Suavizar, Más cola, Bombo seco.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
