# Res Tame

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Supresor de resonancias dinámico: analiza el espectro (STFT de 2048 puntos, ventana Hann, 75 % de solape) y, bin por bin, atenúa solo lo que sobresale de la envolvente espectral suavizada, con ataque y release. Latencia reportada al DAW: 2048 muestras (43 ms a 48 kHz; se compensa automáticamente).
- Controles: Depth (hasta 24 dB), Sharpness (qué tan estrechos quedan los cortes), Selectivity (ancho de la envolvente: solo picos finos o también bultos), Threshold, Attack, Release, rango Desde/Hasta, canal Stereo / Mid / Side, Delta.
- **Unmask**: con sidechain externo el detector escucha la otra pista y corta aquí las frecuencias donde ella suena fuerte (hacerle espacio a la voz en los synths).
- Tests: Depth 0 devuelve la entrada exacta (retardada por la latencia); ruido rosa sin resonancias pierde menos de 0.5 dB; un seno 12 dB sobre el ruido se atenúa al menos 0.8 × Depth; fuera del rango no se toca; Delta es exactamente lo que se quita; unmask baja > 8 dB la banda del sidechain y menos de 0.7 dB el total; Side deja intacta una señal mono.
- Pendiente para 1.1: curva de rango editable (ahora dos límites), tamaño de FFT seleccionable (menos latencia), prueba a oído contra referencias en voces y synths ("musical noise").
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
