# Echo Ready

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Delay estéreo y ping-pong con tiempo sincronizado al tempo del DAW (13 figuras, con punteados y tresillos) o libre en ms. El tiempo sincronizado es exacto al sample (test), con lectura fraccional por interpolación cúbica.
- Feedback hasta 110 % con saturación suave en el lazo (no explota), corte de graves y de agudos dentro del lazo (cada eco más oscuro que el anterior), difusión y bloqueo de DC.
- Tres estilos: **Digital** (limpio), **Tape** (saturación, agudos recortados, wow) y **Analog** (más oscuro, estilo BBD).
- Wow (vaivén lento del tiempo) ajustable.
- **Ducking**: los ecos bajan mientras suena la señal original y suben en los huecos (test: > 30 dB de reducción).
- Cambios de tiempo sin clics: **Crossfade** (60 ms entre el tiempo viejo y el nuevo, sin cambio de tono; test) o **Glide** (el tono se dobla como en una cinta; test).
- Vista de las repeticiones (nivel y lado), presets Slapback, 1/8 punteado para voz y Dub.
- Mix 0 % deja la señal original idéntica (test). Sin latencia.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
