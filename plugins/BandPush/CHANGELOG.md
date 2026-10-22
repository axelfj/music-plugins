# Band Push

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Compresor multibanda de 3 bandas con compresión hacia arriba y hacia abajo por banda (el sonido "OTT").
- Una perilla principal, **Profundidad**, que escala toda la acción dinámica; **Tiempo** escala ataque (15 ms) y release (150 ms) de 0.1x a 10x; **Umbral** global.
- Por banda: Arriba (%), Abajo (%) y Ganancia; medidor de ganancia con signo por banda.
- Crossovers Linkwitz-Riley de 4.º orden ajustables (graves/medios y medios/agudos) con compensación de fase: las bandas suman plano (test: < 0.05 dB en todo el espectro).
- Detector de pico con retención adaptada a cada banda (20 / 5 / 2 ms) para que los graves no bombeen entre picos.
- La subida de las partes suaves se limita a +24 dB para no levantar el ruido de fondo.
- Entrada, Salida y Mix (la señal "seca" del mix es la suma de bandas, así que no hay cancelaciones de fase).
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
