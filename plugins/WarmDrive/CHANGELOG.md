# Warm Drive

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Saturación de 1 a 4 bandas (crossovers Linkwitz-Riley de 4.º orden con compensación de fase: las bandas suman plano, test < 0.05 dB).
- Cinco curvas por banda: **Tape** (tanh, armónicos impares), **Tube** (asimétrica, armónicos pares), **Transistor** (codo más duro), **Soft Clip** (limpia hasta recortar) y **Rectify** (pliegue, octava arriba). Todas con pendiente 1 en cero: lo suave pasa casi intacto y el Drive dice cuánto golpea lo fuerte.
- Por banda: tipo, Drive 0–36 dB, Tono (paso-bajo de 1 kHz a Off) y Mix. Luz en la curva que muestra cuánto se está saturando.
- Auto Gain: compensa la ganancia de cada curva (medida con un seno a −12 dBFS) para comparar a igual volumen (test: ±1 dB).
- Sobremuestreo 1x / 2x / 4x con filtros half-band de −110 dB; los crossovers corren a la frecuencia alta. Aliasing a 4x con 18 dB de drive: < −75 dBFS en Tape/Tube/Transistor/Soft Clip, < −60 dBFS en Rectify (es el modo sucio por diseño).
- Bloqueo de DC tras las curvas asimétricas (test: sin DC a la salida).
- Entrada, Salida y Mix global (el lado limpio del mix es la suma de bandas, así que no hay cancelaciones de fase).
- Tres puntos de partida: Calentar bajo, Brillo en voces, Empuje de drums.
- Interfaz en español o inglés según el idioma del sistema, redimensionable. Latencia reportada al DAW.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
