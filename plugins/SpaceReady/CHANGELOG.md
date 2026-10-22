# Space Ready

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Reverb algorítmica sobre una red de 8 líneas de retardo realimentadas (FDN) con matriz de Hadamard (ortogonal, conserva la energía) y ganancias por línea calculadas para el RT60 pedido: el Decay medido coincide con la perilla en los cuatro modos (test: ±15 %).
- Modos **Room**, **Plate**, **Hall** y **Ambience** (espacio enorme, estilo "supermassive", hasta 30 s), con Size (50-200 %).
- Modulación lenta de las líneas contra resonancias metálicas (test: en el espectro de la cola el pico más fuerte queda a menos de 18 dB de la mediana).
- Pre-delay 0-200 ms (test: al sample), Damping (los agudos mueren antes), Diffusion de entrada (4 all-pass por canal), Width mid/side (test: 0 % = mono exacto, 100 % = canales decorrelados), corte de graves y agudos en la salida.
- **Ducking**: la reverb baja mientras suena la señal original (test: > 20 dB) y **Freeze**: la cola se mantiene constante (test: ±1 dB en 2.5 s) y la entrada se silencia.
- Mix 0 % deja la señal original idéntica (test). Sin latencia. Cola larga estable (test: 30 s, size 200 %, modulación máxima).
- Vista de la envolvente de decaimiento con el nivel real de la cola encima. Presets: Sala, Plate voz, Hall y Espacio.
- Pendiente para 1.1: comparación a oído contra reverbs de referencia (issue #13), early reflections separadas.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
