# Shimmer Ready

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Delay con cambio de tono por repetición (−24 a +24 semitonos): cada eco sube (o baja) respecto al anterior, así que +12 convierte una nota en una escalera de octavas. Con tono 0 y mix 100 % es un delay normal, exacto a la muestra y sincronizado al tempo (test: 1/8 a 120 BPM = 12000 muestras, y con feedback 50 % la segunda repetición es exactamente la mitad).
- Pitch shifter granular de dos granos con **grano sincronizado al tono**: detecta la nota que circula por el bucle (YIN, el mismo detector de Tune Ready) y ajusta el tamaño del grano para que los dos granos queden en fase (sin batidos ni cancelaciones) y la nota desplazada caiga exacta. Test: la primera repetición desplazada +12 queda a menos de 5 cents de 440 Hz, y la siguiente a menos de 10 cents de 880; −12 con grano de 30 ms también dentro de 5 cents. Sin tono detectable usa el tamaño de grano elegido.
- Reversa (cada eco al revés; test: una rampa ascendente vuelve descendente), feedback hasta 110 % con limitador suave (test: 10 s acotados), filtros de graves y agudos en el bucle (test: el pasa bajos de 1 kHz quita más de 25 dB a 6 kHz en la segunda vuelta), freeze (test: el nivel se mantiene dentro de 1 dB y la nueva entrada no entra).
- Modo shimmer: reverb de Space Ready (hall, cola de 0,5 a 30 s) añadida a los ecos (test: hay cola después de los ecos solo con shimmer > 0). Ancho estéreo por desplazamiento del eco derecho.
- Mix 0 deja la señal bit a bit (test). Sin latencia.
- Vista de los ecos como escalera con la nota de cada repetición y la cola de la reverb; tono detectado en el bucle.
- Presets: Pad octava, Quintas, Voz etérea, Reversa, Transición, Delay limpio.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).

Pendiente para 1.0.x: granos sincronizados también para acordes (hoy solo para una nota a la vez; con acordes usa el tamaño fijo), modulación del tiempo, ducking por la señal seca.
