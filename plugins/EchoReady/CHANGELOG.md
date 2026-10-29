# Delay-R (antes Echo Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Delay-R** (antes Echo Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Delay-R** (formerly Echo Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Echo Ready deberían abrir con Delay-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Echo Ready should open with Delay-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Echo Ready" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Echo Ready" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).
- Menos CPU: las líneas de delay usan índices sin división (0.44 % → 0.38 % de un núcleo). / Less CPU: delay lines index without divisions.

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
