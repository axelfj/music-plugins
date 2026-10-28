# Pitch-R

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Cambia el tono de cualquier cosa, sin artefactos: mezclas, acordes, pads, bajos, samples, voces y drums. El rango es ±24 semitonos más un ajuste fino de ±100 cents. / Pitch shifting for anything, without artifacts (mixes, chords, pads, bass, samples, vocals, drums): ±24 semitones plus ±100 cents fine tuning.
- Usa un motor propio, el más limpio de los que se midieron contra Signalsmith Stretch y Rubber Band (R3, R2 y Live). Los números están en [docs/pitch-r/algorithm-study.md](../../docs/pitch-r/algorithm-study.md). / Its own engine, the cleanest of the ones measured against Signalsmith Stretch and Rubber Band (R3, R2, Live); numbers in the study linked above.
  - Un seno queda a menos de 1 cent de la nota exacta (test). / A sine lands within 1 cent of the exact note (test).
  - Lo que no es la nota queda por debajo de −65 dB, o de −55 dB en bajos por debajo de 30 Hz (test). / Everything that is not the note stays under −65 dB, or −55 dB for bass below 30 Hz (test).
  - Un acorde no deja tonos fantasma entre las notas: quedan por debajo de −45 dB (test). / A chord leaves no ghost tones between its notes: they stay under −45 dB (test).
  - Un golpe (click, bombo) no tiene pre-eco por encima de −25 dB (test), y su nivel queda dentro de ±2 dB. / A hit (click, kick) shows no pre-echo above −25 dB (test) and keeps its level within ±2 dB.
- Hay tres modos. / Three modes:
  - **Limpio**, unos 85 ms: para mezclas y acordes. / **Clean** (~85 ms): for mixes and chords.
  - **Voz**, unos 43 ms: para una sola voz o instrumento. / **Voice** (~43 ms): for one voice or instrument.
  - **Rápido**, unos 21 ms: para tocar en vivo. / **Fast** (~21 ms): for playing live.
  - La latencia se informa al DAW, y el test comprueba que coincide con el retardo medido a 44.1, 48 y 96 kHz. / Latency is reported to the DAW; the test checks it matches the measured delay at 44.1, 48 and 96 kHz.
- **Conservar formantes**: la voz cambia de nota pero sigue sonando a la misma persona, sin efecto ardilla ni monstruo. En el test, la forma de la vocal queda a menos de 2.5 dB de la original. / **Keep formants**: the voice changes note but still sounds like the same person (no chipmunk, no monster). In the test the vowel shape stays within 2.5 dB of the original.
  - **Formante** mueve el timbre ±12 semitonos sin cambiar la nota. / **Formant** moves the timbre ±12 semitones without changing the note.
- En 0 st la señal pasa idéntica, solo con la latencia (test). / At 0 st the signal passes bit-identical, only delayed (test).
  - **Original (A/B)** compara contra la señal sin cambiar, con la misma latencia. / **Original (A/B)** compares against the unchanged signal at the same latency.
  - **Mix** sirve para doblar a la octava. / **Mix** blends for octave doubling.
- Una señal mono sigue siendo mono, y los canales no se mezclan entre sí (test). / A mono signal stays mono and the channels stay apart (test).
- La automatización del tono no hace clicks (test). / Pitch automation is click-free (test).
- Consume entre 5 y 7 % de un núcleo en estéreo a 48 kHz. / Uses 5–7 % of one core in stereo at 48 kHz.
- La interfaz muestra el espectro de entrada y el de salida. / The interface shows the input and output spectrum.
  - Muestra el intervalo con su nombre (por ejemplo "Quinta justa arriba") y la nota más fuerte antes y después (A2 → E3). / It shows the interval by name ("Perfect fifth up") and the loudest note before and after (A2 → E3).
  - Tiene botones de intervalos rápidos. / It has quick interval buttons.
  - Está en español o inglés según el idioma del sistema, y es redimensionable. / It is in Spanish or English depending on the system language, and resizable.
- Formatos: VST3 y CLAP en Windows, macOS y Linux, y AU en macOS. / Formats: VST3 and CLAP on Windows, macOS and Linux, plus AU on macOS.

Pendiente para 1.0.x / Planned for 1.0.x:

- Modo de baja latencia con acordes más limpios. / A low-latency mode with cleaner chords.
- Pruebas con voces y mezclas reales, a oído. / Listening tests on real vocals and mixes.
