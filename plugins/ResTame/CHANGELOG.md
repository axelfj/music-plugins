# Resonance-R (antes Res Tame)

## Próxima versión / Next

- Ventana con zoom vectorial: arrastra la esquina o usa el botón de colores (arriba a la derecha) para elegir de 75 % a 250 %; texto, perillas y curvas se ven igual de nítidos a cualquier tamaño. / Vector zoom: drag the corner or use the colour-dots button (top right) for 75 % to 250 %; text, knobs and curves stay sharp at any size.
- Temas: 7 incluidos (ROMMUSER, Medianoche, Neón, Bosque, Atardecer, Claro, Alto contraste) y editor para crear los tuyos, solo para este plugin o para todos. El plugin recuerda su tamaño y su tema. / Themes: 7 built in plus an editor to make your own, for this plugin only or for all. The plugin remembers its size and theme.
- Esquina superior derecha: "R - V1 - FREE". / Top-right corner: "R - V1 - FREE".

## 1.0.0-alpha.3

- Menos CPU: ~2x (1.6 % → 0.7 % de un núcleo): FFT nueva sin llamadas de librería por multiplicación y los dos canales en una sola FFT compleja. Mismo resultado (tests). / Less CPU: ~2x, faster FFT and both channels in one complex FFT.

## 1.0.0-alpha.2

- Nuevo nombre: **Resonance-R** (antes Res Tame). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Resonance-R** (formerly Res Tame): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Res Tame deberían abrir con Resonance-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Res Tame should open with Resonance-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Res Tame" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Res Tame" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Supresor de resonancias dinámico: analiza el espectro (STFT de 2048 puntos, ventana Hann, 75 % de solape) y, bin por bin, atenúa solo lo que sobresale de la envolvente espectral suavizada, con ataque y release. Latencia reportada al DAW: 2048 muestras (43 ms a 48 kHz; se compensa automáticamente).
- Controles: Depth (hasta 24 dB), Sharpness (qué tan estrechos quedan los cortes), Selectivity (ancho de la envolvente: solo picos finos o también bultos), Threshold, Attack, Release, rango Desde/Hasta, canal Stereo / Mid / Side, Delta.
- **Unmask**: con sidechain externo el detector escucha la otra pista y corta aquí las frecuencias donde ella suena fuerte (hacerle espacio a la voz en los synths).
- Tests: Depth 0 devuelve la entrada exacta (retardada por la latencia); ruido rosa sin resonancias pierde menos de 0.5 dB; un seno 12 dB sobre el ruido se atenúa al menos 0.8 × Depth; fuera del rango no se toca; Delta es exactamente lo que se quita; unmask baja > 8 dB la banda del sidechain y menos de 0.7 dB el total; Side deja intacta una señal mono.
- Pendiente para 1.1: curva de rango editable (ahora dos límites), tamaño de FFT seleccionable (menos latencia), prueba a oído contra referencias en voces y synths ("musical noise").
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
