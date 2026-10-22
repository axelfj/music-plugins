# Vox Synth

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
