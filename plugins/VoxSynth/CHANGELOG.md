# Vocoder-R (antes Vox Synth)

## 1.0.0-alpha.3

- Corrección: las armonías y el robot usaban el mismo cambio de tono que Tune-R, que leía cada grano corrido por la fracción de muestra al revés y ponía las marcas en muestras enteras. Las voces sonaban ásperas. Ahora el ruido de una armonía a +7 baja de −8 dB a −36 dB. / Fix: harmonies and robot share Tune-R's pitch shifter, which read each grain's fractional offset the wrong way round and put marks on whole samples; a +7 harmony's noise drops from −8 dB to −36 dB.
- Menos CPU: detección de tono ~2.7x más rápida (4.5 % → 1.7 % de un núcleo), mismo resultado. / Less CPU: ~2.7x faster pitch detection, same result.

## 1.0.0-alpha.2

- Nuevo nombre: **Vocoder-R** (antes Vox Synth). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Vocoder-R** (formerly Vox Synth): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Vox Synth deberían abrir con Vocoder-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Vox Synth should open with Vocoder-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Vox Synth" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Vox Synth" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Tres modos sobre el mismo motor de voz (YIN + PSOLA compartido con Tune Ready, formantes conservados):
  - **Armonías**: hasta 4 voces generadas desde tu voz, por grados de escala (tercera, quinta, octava… siempre dentro de la tonalidad), por semitonos fijos o cantando las notas MIDI que mantengas pulsadas (una voz por nota). Detune, pan, nivel y formante por voz; glide. Tests: E4 en Do mayor con +2 / −2 grados da G4 y C4 (±10 cents); +12 semitonos mantiene el centroide espectral (los formantes no se mueven); en modo MIDI cada voz canta su nota y respeta el pan.
  - **Vocoder**: 8 a 32 bandas (100 Hz – 8 kHz, dos biquads por banda), portadora sierra / pulso / ruido que sigue la nota que cantas (talkbox), toca una nota fija o las notas MIDI (acordes, hasta 4), o externa por la entrada sidechain. Ataque, release, sibilantes (consonantes) y detune. Tests: la salida tiene el tono de la portadora, los armónicos siguen la envolvente espectral de la voz (+6 dB donde la voz tiene energía), silencio → silencio, y la portadora externa sigue el tono del sidechain.
  - **Robot**: toda la voz a una sola nota, con formante independiente (monstruo / pequeño). Test: ±5 cents de la nota pedida.
- Sin voces activas y mix 0 la señal pasa idéntica salvo la latencia (test, bit a bit).
- Vista: piano-roll con la nota detectada y cada voz en su color, o barras de las bandas del vocoder.
- Presets: 3ª y 5ª, Coro octavas, Talkbox, Vocoder MIDI, Robot, Monstruo.
- Interfaz en español o inglés según el idioma del sistema, redimensionable. Entrada MIDI y bus sidechain opcional.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).

Pendiente para 1.0.x: humanize por voz (timing y afinación), portadora con más formas de onda y unísono, pruebas con voces reales.
