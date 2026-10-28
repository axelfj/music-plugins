# ROMMUSER Music Plugins

Plugins de audio **gratis y open source** (VST3, AU, CLAP) para productores que están empezando y no tienen cómo pagar los plugins caros. Calidad profesional, interfaz simple, en español e inglés.

*Free, open-source audio plugins (VST3, AU, CLAP) for producers who are starting out and can't afford expensive tools. Pro-grade sound, simple UI, Spanish and English.*

Hechos por [ROMMUSER](https://rommuser.com). Licencia [GPL-3.0](LICENSE). Si quieres una mezcla o master profesional: [rommuser.com/estudio](https://rommuser.com/estudio).

## Plugins

Cada nombre dice qué hace el plugin, y la **R** es de ROMMUSER. *Each name says what the plugin does; the R stands for ROMMUSER.* Los proyectos guardados con el nombre anterior deberían abrir igual, porque el ID del plugin no cambió; si tu DAW no lo encuentra, vuelve a escanear los plugins.

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
| **Pitch-R** | — | Cambiador de tono limpio para cualquier cosa (mezclas, acordes, voces, bajos, drums): ±24 st y cents, sin tonos fantasma ni pre-eco, conservar formantes o moverlos, modos Limpio / Voz / Rápido, A/B con el original | Alpha |

El plan completo está en el [roadmap](https://github.com/axelfj/music-plugins/issues/1).

## Vista previa

Así se ven por dentro antes de instalarlos. Son capturas reales de cada plugin con audio pasando, generadas desde el código con [`tools/screenshot`](tools/screenshot/render.sh). Haz clic en una para verla en grande.

*What each plugin looks like before you install it: real renders of every editor with audio running. Click one to see it full size.*

<table>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/clip-ready.png"><img src="docs/screenshots/clip-ready.png" alt="Clipper-R"></a><br><b>Clipper-R</b> · Clipper</td>
<td width="50%" align="center"><a href="docs/screenshots/shape-eq.png"><img src="docs/screenshots/shape-eq.png" alt="EQ-R"></a><br><b>EQ-R</b> · EQ</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/press-ready.png"><img src="docs/screenshots/press-ready.png" alt="Compressor-R"></a><br><b>Compressor-R</b> · Compresor</td>
<td width="50%" align="center"><a href="docs/screenshots/duck-ready.png"><img src="docs/screenshots/duck-ready.png" alt="Sidechain-R"></a><br><b>Sidechain-R</b> · Sidechain</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/band-push.png"><img src="docs/screenshots/band-push.png" alt="Multiband-R"></a><br><b>Multiband-R</b> · Multibanda</td>
<td width="50%" align="center"><a href="docs/screenshots/warm-drive.png"><img src="docs/screenshots/warm-drive.png" alt="Saturator-R"></a><br><b>Saturator-R</b> · Saturación</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/gate-ready.png"><img src="docs/screenshots/gate-ready.png" alt="Gate-R"></a><br><b>Gate-R</b> · Gate</td>
<td width="50%" align="center"><a href="docs/screenshots/echo-ready.png"><img src="docs/screenshots/echo-ready.png" alt="Delay-R"></a><br><b>Delay-R</b> · Delay</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/phase-ready.png"><img src="docs/screenshots/phase-ready.png" alt="Phaser-R"></a><br><b>Phaser-R</b> · Phaser</td>
<td width="50%" align="center"><a href="docs/screenshots/wide-ready.png"><img src="docs/screenshots/wide-ready.png" alt="Chorus-R"></a><br><b>Chorus-R</b> · Chorus / flanger</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/space-ready.png"><img src="docs/screenshots/space-ready.png" alt="Reverb-R"></a><br><b>Reverb-R</b> · Reverb</td>
<td width="50%" align="center"><a href="docs/screenshots/res-tame.png"><img src="docs/screenshots/res-tame.png" alt="Resonance-R"></a><br><b>Resonance-R</b> · Resonancias</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/low-mono.png"><img src="docs/screenshots/low-mono.png" alt="Stereo-R"></a><br><b>Stereo-R</b> · Imagen estéreo</td>
<td width="50%" align="center"><a href="docs/screenshots/punch-shape.png"><img src="docs/screenshots/punch-shape.png" alt="Transient-R"></a><br><b>Transient-R</b> · Transientes</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/ess-clean.png"><img src="docs/screenshots/ess-clean.png" alt="De-Esser-R"></a><br><b>De-Esser-R</b> · De-esser</td>
<td width="50%" align="center"><a href="docs/screenshots/peak-ready.png"><img src="docs/screenshots/peak-ready.png" alt="Limiter-R"></a><br><b>Limiter-R</b> · Limitador</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/master-check-live.png"><img src="docs/screenshots/master-check-live.png" alt="Meter-R"></a><br><b>Meter-R</b> · Medidor LUFS</td>
<td width="50%" align="center"><a href="docs/screenshots/filter-ready.png"><img src="docs/screenshots/filter-ready.png" alt="Filter-R"></a><br><b>Filter-R</b> · Filtro</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/kick-ready.png"><img src="docs/screenshots/kick-ready.png" alt="Kick-R"></a><br><b>Kick-R</b> · Sinte de bombos</td>
<td width="50%" align="center"><a href="docs/screenshots/tune-ready.png"><img src="docs/screenshots/tune-ready.png" alt="Tune-R"></a><br><b>Tune-R</b> · Afinación de voz</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/vox-synth.png"><img src="docs/screenshots/vox-synth.png" alt="Vocoder-R"></a><br><b>Vocoder-R</b> · Armonías / vocoder</td>
<td width="50%" align="center"><a href="docs/screenshots/stutter-ready.png"><img src="docs/screenshots/stutter-ready.png" alt="Stutter-R"></a><br><b>Stutter-R</b> · Stutter / glitch</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/shimmer-ready.png"><img src="docs/screenshots/shimmer-ready.png" alt="Pitch Delay-R"></a><br><b>Pitch Delay-R</b> · Delay con tono</td>
<td width="50%" align="center"><a href="docs/screenshots/wave-ready.png"><img src="docs/screenshots/wave-ready.png" alt="Synth-R"></a><br><b>Synth-R</b> · Sintetizador</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/chord-ready.png"><img src="docs/screenshots/chord-ready.png" alt="Chord-R"></a><br><b>Chord-R</b> · Acordes MIDI</td>
<td width="50%" align="center"><a href="docs/screenshots/arp-ready.png"><img src="docs/screenshots/arp-ready.png" alt="Arpeggiator-R"></a><br><b>Arpeggiator-R</b> · Arpegiador MIDI</td>
</tr>
<tr>
<td width="50%" align="center"><a href="docs/screenshots/pitch-r.png"><img src="docs/screenshots/pitch-r.png" alt="Pitch-R"></a><br><b>Pitch-R</b> · Cambio de tono</td>
<td width="50%"></td>
</tr>
</table>

## Instalar

Descarga la última versión desde [Releases](https://github.com/axelfj/music-plugins/releases). Cada archivo lleva el nombre del plugin, por ejemplo `EQ-R-Setup-1.0.0-alpha.2.exe`.

**Windows:** abre `<Plugin>-Setup-<versión>.exe`. Instala el VST3 (y CLAP) en las carpetas estándar. Si Windows muestra "Windows protegió tu PC", haz clic en *Más información* y luego *Ejecutar de todas formas*: el instalador no está firmado todavía, pero el código es público en este repo.

**macOS:** abre `<Plugin>-<versión>.pkg`. Si macOS dice que no se puede abrir porque es de un desarrollador no identificado: clic derecho → *Abrir*, o ve a *Ajustes del Sistema → Privacidad y seguridad* y pulsa *Abrir de todas formas*. Después reinicia el DAW. (Logic/GarageBand usan el AU; Ableton y FL usan el VST3.)

**Linux:** descomprime el `.tar.gz` en `~/.vst3/` y `~/.clap/`. Si tenías la versión con el nombre anterior (por ejemplo `Clip Ready.vst3`), bórrala de esas carpetas para que el DAW no muestre el plugin dos veces.

Luego, en tu DAW, vuelve a escanear los plugins.

## Compilar desde el código

Necesitas CMake 3.22+, un compilador C++20 y, en Linux, los paquetes de desarrollo de ALSA, X11, FreeType, Fontconfig y OpenGL.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure   # pruebas de DSP
```

Los plugins quedan en `build/plugins/<Plugin>/<Plugin>_artefacts/Release/`. JUCE, clap-juce-extensions y Catch2 se descargan solos la primera vez.

Si cambias una interfaz, vuelve a generar las capturas de la vista previa con `tools/screenshot/render.sh` (en Linux usa `xvfb-run` si no hay pantalla). Compila una pequeña app por plugin que abre el editor, le pasa audio de prueba (o un acorde a los instrumentos) y lo guarda en `docs/screenshots/<plugin>.png`.

## Estructura

```
shared/dsp/      DSP común, C++ puro sin JUCE (sobremuestreo, suavizado, cambio de tono espectral), probado en tests/
shared/ui/       Look & feel común y textos bilingües
plugins/<Name>/  Un plugin por carpeta: VERSION, CHANGELOG.md, Source/
tests/           Pruebas de DSP (Catch2): respuesta, aliasing, latencia, nulos
installer/       Inno Setup (Windows) y pkgbuild (macOS)
tools/screenshot Renderiza la interfaz de cada plugin a PNG para la vista previa
docs/screenshots Las capturas que muestra este README
cmake/           music_plugin() helper
```

## Calidad

Cada plugin lleva pruebas automáticas de DSP que corren en CI en Windows, macOS y Linux, y pasa [pluginval](https://github.com/Tracktion/pluginval) en nivel estricto. Las pruebas de Clipper-R verifican, por ejemplo, que el pico de salida nunca supere el techo, que con Techo estricto la salida final nunca supere el techo con cualquier Mix o Salida, que con Mix al 0 % (sin techo estricto) la salida sea idéntica a la entrada, y que a 8x el aliasing quede por debajo de -100 dBFS.

## Publicar una versión

Cada plugin se versiona por separado: `plugins/<Name>/VERSION` guarda la versión completa (por ejemplo `1.0.0-alpha.2`; la parte numérica es la que ven el DAW y los instaladores) y `CHANGELOG.md` tiene una sección por versión. Para publicar, sube `VERSION` y el changelog, mergea, y corre el workflow **Build** manualmente (*Run workflow*) con la carpeta del plugin y exactamente esa versión, por ejemplo `ClipReady` (Clipper-R) y `1.0.0-alpha.3`. CI compila en los tres sistemas, valida, crea el tag `clip-ready-v1.0.0-alpha.3` y sube los instaladores al release.

## Contribuir

Issues y pull requests son bienvenidos. Antes de proponer un plugin nuevo revisa el [roadmap](https://github.com/axelfj/music-plugins/issues/1). Nunca copiamos nombres, logos ni diseño visual de plugins comerciales.
