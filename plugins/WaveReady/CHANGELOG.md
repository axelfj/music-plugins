# Wave Ready

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Sintetizador wavetable (instrumento MIDI) pensado para aprender síntesis: todo a la vista, sin menús escondidos.
- 2 osciladores con 10 ondas (seno, triángulo, sierra, cuadrada, pulsos, órgano, formante, metal, digital), MORPH entre ondas vecinas, octava / semitono / fino, unísono hasta 7 voces con desafine y ancho estéreo, fase inicial con retrig o fase libre. Tablas con 11 niveles de mip para que no haya aliasing (test: sierra en C7 con todo el ruido de aliasing por debajo de -80 dB; cada onda afinada a ±1 cent en A4).
- Sub (seno o cuadrada, 1 o 2 octavas abajo) y capa de ruido blanco / rosa.
- Filtro SVF (Simper): LP 12, LP 24 (dos etapas reales), HP 12, BP 12, con resonancia, drive, seguimiento de teclado, envolvente y velocity al corte (test: LP 24 a 500 Hz atenúa el armónico 15 más de 60 dB).
- Envolvente AMP y MOD (ADSR exponenciales; test de tiempos y silencio exacto al final), MOD también al tono (808, hoover).
- 2 LFOs (seno, triángulo, sierra, cuadrada, S&H) libres o sincronizados al tempo (4 compases a 1/64), con fade y retrig; LFO 1 a tono / corte / volumen, LFO 2 a morph / corte / tono (test: trémolo cuadrado a 1/4 a 120 BPM cae en el tiempo exacto).
- Polifonía de 16 voces con robo de voz, modo mono con legato y glide (test), pitch bend con rango configurable, rueda de modulación a corte y vibrato, pedal de sostenido.
- Efectos: drive, chorus, delay ping-pong sincronizado con tono, reverb (motor de Space Ready, sala o plate) y salida con limitador suave (test: nunca pasa de 0 dBFS).
- Osciloscopio en vivo, contador de voces, teclado en pantalla, 13 presets (Supersaw Lead, Pluck, Reese Bass, Sub 808, Future Chord, Pad, Organ, Arp Pluck, Acid, Hoover, Bell, Wobble).
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS). Funciona como instrumento en FL Studio, Ableton y cualquier DAW con VST3/AU/CLAP.

Pendiente para 1.0.x: importar wavetables propias (.wav), matriz de modulación libre, arpegiador integrado.
