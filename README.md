# ROMMUSER Music Plugins

Plugins de audio **gratis y open source** (VST3, AU, CLAP) para productores que están empezando y no tienen cómo pagar los plugins caros. Calidad profesional, interfaz simple, en español e inglés.

*Free, open-source audio plugins (VST3, AU, CLAP) for producers who are starting out and can't afford expensive tools. Pro-grade sound, simple UI, Spanish and English.*

Hechos por [ROMMUSER](https://rommuser.com). Licencia [GPL-3.0](LICENSE). Si quieres una mezcla o master profesional: [rommuser.com/estudio](https://rommuser.com/estudio).

## Plugins

Cada nombre dice qué hace el plugin, y la **R** es de ROMMUSER. *Each name says what the plugin does; the R stands for ROMMUSER.* Los proyectos guardados con el nombre anterior abren igual (mismo ID de plugin).

| Plugin | Antes | Qué hace | Estado |
| --- | --- | --- | --- |
| **Clipper-R** | Clip Ready | Clipper suave con sobremuestreo para drums, bajos y master | Alpha |
| **EQ-R** | Shape EQ | EQ paramétrico de 12 bandas con analizador, M/S y bell "matched" | Alpha |
| **Compressor-R** | Press Ready | Compresor con tres caracteres (Clean / Punch / Glue), sidechain y lookahead | Alpha |
| **Sidechain-R** | Duck Ready | Sidechain / volume shaper sincronizado al tempo, disparo por tempo, MIDI o bombo, modo multibanda | Alpha |
| **Multiband-R** | Band Push | Compresor multibanda de 3 bandas hacia arriba y hacia abajo (estilo OTT) con una perilla de profundidad | Alpha |
| **Saturator-R** | Warm Drive | Saturación de 1 a 4 bandas con cinco curvas (tape, tube, transistor, soft clip, rectify), auto gain y sobremuestreo | Alpha |
| **Gate-R** | Gate Ready | Gate y expansor con histéresis, hold, lookahead, filtros de sidechain con Escuchar y sidechain externo | Alpha |
| **Delay-R** | Echo Ready | Delay estéreo / ping-pong sincronizado al tempo, estilos Digital/Tape/Analog, filtros en el lazo, wow y ducking | Alpha |
| **Phaser-R** | Phase Ready | Phaser de 4 a 12 etapas con LFO libre o sincronizado (seno, triángulo, random), feedback y stereo spread | Alpha |
| **Chorus-R** | Wide Ready | Chorus de 1 a 3 voces (ensemble), flanger through-zero con feedback y micro-pitch estéreo | Alpha |
| **Reverb-R** | Space Ready | Reverb algorítmica FDN (Room, Plate, Hall, Ambience) con RT60 exacto, modulación, ducking y freeze | Alpha |
| **Resonance-R** | Res Tame | Supresor de resonancias dinámico (STFT) con Delta, Mid/Side y sidechain "unmask" | Alpha |
| **Stereo-R** | Low Mono | Imager estéreo con mono en graves (Linkwitz-Riley 24/48 dB/oct), goniómetro y correlación | Alpha |
| **Transient-R** | Punch Shape | Transient shaper (ataque / sustain) independiente del nivel, con filtro en el detector y clipper opcional | Alpha |
| **De-Esser-R** | Ess Clean | De-esser por banda o broadband con rango sobre el espectro, lookahead y Escuchar | Alpha |
| **Limiter-R** | Peak Ready | Limitador true-peak con lookahead, release adaptativo, tres estilos y medidor LUFS con objetivo | Alpha |
| **Meter-R** | Master Check Live | Medidor LUFS / LRA / true peak en tiempo real con los perfiles de plataforma de Master Check | Alpha |
| **Filter-R** | Filter Ready | Doble filtro resonante (LP/HP/BP/notch, 12/24 dB) en serie o paralelo con LFO sincronizado, envolvente, drive y carácter Analog | Alpha |
| **Kick-R** | Kick Ready | Sinte de bombos (instrumento MIDI): cuerpo con envolvente de tono, click, drive, clip, EQ, afinado a ±1 cent, exporta WAV | Alpha |
| **Tune-R** | Tune Ready | Afinación de voz en tiempo real (YIN + PSOLA con formantes): escala / tonalidad, velocidad, humanize, MIDI, modo fijo con tono y formante independientes | Alpha |
| **Vocoder-R** | Vox Synth | Armonías (hasta 4 voces por grados de escala, semitonos o MIDI), vocoder (8-32 bandas, portadora interna o sidechain, talkbox) y robot sobre el mismo motor YIN + PSOLA | Alpha |
| **Stutter-R** | Stutter Ready | Stutter / glitch sincronizado al tempo (1/4 a 1/128) con 8 gestos (buzz, pitch, reversa, tape stop, gate, filtro), disparo por MIDI o botones cuantizado a la rejilla, secuenciador de pasos y presets de builds | Alpha |
| **Pitch Delay-R** | Shimmer Ready | Delay granular con tono por repetición (grano sincronizado al tono detectado), reversa, feedback con filtros y freeze, y reverb shimmer integrada (motor de Reverb-R) | Alpha |
| **Synth-R** | Wave Ready | Sintetizador wavetable (instrumento MIDI): 2 osciladores con morph y unísono hasta 7 voces, sub, ruido, filtro SVF 12/24 dB, 2 envolventes, 2 LFOs sincronizados, mono/legato/glide, chorus, delay ping-pong, reverb y 13 presets | Alpha |
| **Chord-R** | Chord Ready | Herramienta MIDI (instrumento): una tecla = un acorde completo. Por escala (tonalidad + 10 escalas, 7ª/9ª, snap) o 20 tipos fijos, 4 voicings, inversiones, octavas, strum con dirección, suavizado y humanize, sonido interno para oírlo al instante y 10 presets | Alpha |
| **Arpeggiator-R** | Arp Ready | Arpegiador MIDI (instrumento): 7 modos (incl. acorde y aleatorio), 15 velocidades con puntillo/tresillo, 1-4 octavas, gate, swing, patrón de 16 pasos, latch, acento, sincronizado a la rejilla del DAW, sonido interno y 10 presets | Alpha |

El plan completo está en el [roadmap](https://github.com/axelfj/music-plugins/issues/1).

## Instalar

Descarga la última versión desde [Releases](https://github.com/axelfj/music-plugins/releases). Cada archivo lleva el nombre del plugin, por ejemplo `EQ-R-Setup-1.0.0-alpha.2.exe`.

**Windows:** abre `<Plugin>-Setup-<versión>.exe`. Instala el VST3 (y CLAP) en las carpetas estándar. Si Windows muestra "Windows protegió tu PC", haz clic en *Más información* y luego *Ejecutar de todas formas*: el instalador no está firmado todavía, pero el código es público en este repo.

**macOS:** abre `<Plugin>-<versión>.pkg`. Si macOS dice que no se puede abrir porque es de un desarrollador no identificado: clic derecho → *Abrir*, o ve a *Ajustes del Sistema → Privacidad y seguridad* y pulsa *Abrir de todas formas*. Después reinicia el DAW. (Logic/GarageBand usan el AU; Ableton y FL usan el VST3.)

**Linux:** descomprime el `.tar.gz` en `~/.vst3/` y `~/.clap/`.

Luego, en tu DAW, vuelve a escanear los plugins.

## Compilar desde el código

Necesitas CMake 3.22+, un compilador C++20 y, en Linux, los paquetes de desarrollo de ALSA, X11, FreeType, Fontconfig y OpenGL.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure   # pruebas de DSP
```

Los plugins quedan en `build/plugins/<Plugin>/<Plugin>_artefacts/Release/`. JUCE, clap-juce-extensions y Catch2 se descargan solos la primera vez.

## Estructura

```
shared/dsp/      DSP común, C++ puro sin JUCE (sobremuestreo, suavizado), probado en tests/
shared/ui/       Look & feel común y textos bilingües
plugins/<Name>/  Un plugin por carpeta: VERSION, CHANGELOG.md, Source/
tests/           Pruebas de DSP (Catch2): respuesta, aliasing, latencia, nulos
installer/       Inno Setup (Windows) y pkgbuild (macOS)
cmake/           music_plugin() helper
```

## Calidad

Cada plugin lleva pruebas automáticas de DSP que corren en CI en Windows, macOS y Linux, y pasa [pluginval](https://github.com/Tracktion/pluginval) en nivel estricto. Las pruebas de Clipper-R verifican, por ejemplo, que el pico de salida nunca supere el techo, que con Techo estricto la salida final nunca supere el techo con cualquier Mix o Salida, que con Mix al 0 % (sin techo estricto) la salida sea idéntica a la entrada, y que a 8x el aliasing quede por debajo de -100 dBFS.

## Publicar una versión

Cada plugin se versiona por separado: `plugins/<Name>/VERSION` guarda la versión completa (por ejemplo `1.0.0-alpha.2`; la parte numérica es la que ven el DAW y los instaladores) y `CHANGELOG.md` tiene una sección por versión. Para publicar, sube `VERSION` y el changelog, mergea, y corre el workflow **Build** manualmente (*Run workflow*) con la carpeta del plugin y exactamente esa versión, por ejemplo `ClipReady` (Clipper-R) y `1.0.0-alpha.3`. CI compila en los tres sistemas, valida, crea el tag `clip-ready-v1.0.0-alpha.3` y sube los instaladores al release.

## Contribuir

Issues y pull requests son bienvenidos. Antes de proponer un plugin nuevo revisa el [roadmap](https://github.com/axelfj/music-plugins/issues/1). Nunca copiamos nombres, logos ni diseño visual de plugins comerciales.
