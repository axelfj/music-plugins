# Kick Ready

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Instrumento (MIDI) para diseñar bombos desde cero: cuerpo sinusoidal con envolvente de tono (inicio en semitonos, caída, curva) y envolvente de volumen (ataque, hold, caída, curva lineal a exponencial).
- La nota siempre empieza en fase 0: sin clic al iniciar (test). Retocar mientras suena hace un fade de 2 ms al bombo anterior y arranca el nuevo limpio (test: ningún salto mayor a la pendiente de la señal).
- Afinado a la nota MIDI a ±1 cent (test en C0, C1, F1, C2 con y sin barrido de tono); afinación fina en centésimas de semitono; Glide entre notas; sensibilidad a velocity.
- Click: ráfaga de ruido filtrado (pasa-altos) o tick sinusoidal, con nivel, duración y tono propios (test: la energía vive en los primeros ms y por encima del tono elegido).
- Drive (tanh con pico normalizado a 0 dBFS), Clip (suave a duro, nunca pasa del techo; test), Sub (shelf de 60 Hz), Corte alto y Salida.
- Vista de la forma de onda del bombo con la envolvente de tono encima, botón PLAY y EXPORTAR WAV (24 bits, a la frecuencia de muestreo del proyecto) para usarlo como sample.
- Presets: House, Techno, Hardstyle, Trap.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS). Funciona como instrumento en FL Studio, Ableton y cualquier DAW con VST3/AU/CLAP.

Pendiente para 1.0.x: capa de sample propio para el click, envolvente de tono dibujable por puntos, capa de sub separada.
