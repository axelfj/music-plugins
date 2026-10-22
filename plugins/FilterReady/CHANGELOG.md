# Filter Ready

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Dos filtros resonantes (A y B) en **serie o paralelo**: pasa-bajos, pasa-altos, pasa-banda y notch, de 12 o 24 dB/oct, con cutoff y resonancia (Q 0.5–20).
- Filtro de estado variable (TPT) a **2x de sobremuestreo**: en modo Limpio la respuesta coincide con la teórica del biquad bilineal al doble de frecuencia de muestreo (test: ±0.2 dB de 100 Hz a 15 kHz, varias resonancias; notch > 40 dB en el centro; 24 dB = cuadrado de 12 dB).
- **Carácter Analógico**: saturación suave de los integradores (la resonancia se comprime al subir el nivel) y **Drive** de entrada con tanh. Con resonancia máxima y entrada a 0 dBFS el filtro se mantiene estable y acotado (test: sin NaN, pico limitado).
- Cutoff suavizado en el dominio logarítmico y coeficientes recalculados en cada muestra: sin zipper al mover el cutoff de golpe (test).
- Modulación: **LFO** libre (0.02–20 Hz) o sincronizado al tempo (4 compases a 1/16, con tresillos y punteados; test: la fase sigue el ppq del DAW) con seno, triángulo, sierra, cuadrado (suavizado) y random; **seguidor de envolvente** con ataque, release y sensibilidad (test: una entrada fuerte sube el cutoff). Cantidades en octavas (±5) por filtro.
- **Pad XY** en el espectro: arrastra para cutoff (x) y resonancia (y) del filtro seleccionado; curva de respuesta combinada dibujada en tiempo real.
- Mix 0 % deja la señal original idéntica (test). Latencia pequeña (filtro de sobremuestreo) reportada al DAW.
- Presets: Subida (build), Wobble, Ácido, Teléfono.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
- Pendiente para una próxima versión: hasta 4 filtros, modelo ladder y modulación por MIDI (issue #29).
