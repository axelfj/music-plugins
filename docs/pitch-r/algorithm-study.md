# Pitch-R: algorithm study / estudio del algoritmo

Why Pitch-R uses its own shifter instead of an existing library, and how it compares. Issue #82.

*Por qué Pitch-R usa su propio motor en vez de una librería existente, y cómo se compara.*

## Candidates / candidatos

| Engine | License | Notes |
| --- | --- | --- |
| Signalsmith Stretch 1.x (default, cheaper, 60 ms presets) | MIT | Phase vocoder with peak-locked phase, formant option |
| Rubber Band v4: R3 (finer), R2 (faster), LiveShifter short / medium | GPL-2+ / commercial | The reference most DAWs ship |
| **Own (shared/dsp/SpectralShifter.h)** | GPL-3 | Described below |

Signalsmith and Rubber Band were built and measured locally only. Neither is vendored in this repo.

*Signalsmith y Rubber Band solo se compilaron y midieron en local; ninguno se incluye en el repo.*

## Method / método

All engines ran through the same harness at 48 kHz, in stereo, with 256-sample blocks. Shifts were -12, -7, -1, +0.5, +5 and +12 semitones.

- **Pitch error:** the worst error in cents, on sines at 55 Hz to 3 kHz.
- **Spurs:** the power outside ±6 bins of the expected partials, relative to the power at them. This is measured on the sines and on a C major triad with 4 harmonics per note. A ghost tone between two notes shows up here.
- **Pre-echo:** the energy before the onset of a shifted click, relative to the click.
- **Smear:** the share of the energy found outside ±2 ms of the click.
- **Formants:** a vowel /a/ at f0 = 140 and 300 Hz, shifted ±7 st. The metric is the mean deviation of the output harmonics from the vowel's envelope at their new frequency. Lower means the voice keeps its character.
- **CPU:** the share of one core, measured on a 2.1 GHz Xeon VM.

## Results / resultados

| Engine | Latency | Sine error / worst spur | Chord worst spur | Click pre-echo (worst) / smear | Formants kept 140 / 300 Hz | CPU |
| --- | --- | --- | --- | --- | --- | --- |
| Signalsmith default | 120 ms | 43.7 c / -48.7 dB | -41.1 dB | -19.2 dB / 2 % | 1.65 / 1.38 dB | 2.4 % |
| Signalsmith cheaper | 140 ms | 86.3 c / -22.6 dB | -21.5 dB | -18.4 dB / 3 % | 1.98 / 1.40 dB | 1.3 % |
| Signalsmith 60 ms | 60 ms | 57.6 c / -44.3 dB | -8.2 dB | -20.8 dB / 1 % | 11.5 / 21.9 dB | 1.8 % |
| Rubber Band R3 | 81 ms | 0.12 c / -24.8 dB | -14.1 dB | -17.0 dB / 33 % | 3.65 / 2.05 dB | 19.2 % |
| Rubber Band R2 | 38 ms | 222 c / +9.8 dB | -2.2 dB | -6.9 dB / 66 % | 3.14 / 8.88 dB | 8.9 % |
| Rubber Band Live short | 55 ms | 0.10 c / -54.1 dB | -42.7 dB | -6.6 dB / 56 % | 2.76 / 4.01 dB | 9.7 % |
| **Own, 4096 (Clean)** | **85 ms** | **0.10 c / -83.2 dB** | **-50.2 dB** | **-30.3 dB / 0 %** | **1.42 / 1.34 dB** | 5.4 % |
| Own, 2048 (Voice) | 43 ms | 0.12 c / -44.8 dB | -13.6 dB | -28.2 dB / 0 % | 1.16 / 1.45 dB | 5.4 % |

With formants off, every engine scores about 6.2 to 8.2 dB on the formant test. That is the expected "chipmunk" result.

*Sin conservar formantes todos dan unos 6.2 a 8.2 dB, el efecto "ardilla" esperado.*

**Decision:** the own engine is the cleanest at every point measured: pitch, spurs, chords, clicks and formants. It costs about twice the CPU of Signalsmith and about half of Rubber Band's live shifter. Clean mode uses 4096 points.

A 2048-point FFT cannot keep the notes of a chord apart (−13.6 dB), so Voice mode is meant for one voice or instrument. An overlap of 4 instead of 8 was rejected, because a sine's worst spur rose from −83 to −52 dB.

*Decisión: el motor propio es el más limpio en todo lo medido, con el doble de CPU que Signalsmith y la mitad que Rubber Band Live. El modo Limpio usa 4096; el modo Voz (2048) es para una sola voz o instrumento.*

## How it works / cómo funciona

`shared/dsp/SpectralShifter.h` is an STFT with a Hann window and 8x overlap. The output is the input delayed by exactly N samples. It works in five stages:

1. **Analysis:** reassignment gives each bin its exact instantaneous frequency and its group delay. Both come from a single FFT through 3-tap combinations. The stereo pair is packed as one complex FFT.
2. **Peak regions:** the spectrum is split into regions that run from valley to valley around each peak. Each region moves as a rigid block, so a partial keeps its exact shape (Laroche-Dolson). The gaps between moved regions are filled from the valley level times its depth, so no copied neighbour lobes appear.
   - A prominence weight decides how each region is treated.
   - Clear partials keep the phase relationships inside their lobe (identity locking).
   - Noisy or transient regions are mapped bin by bin, and each bin takes its phase from step 4.
3. **Formants (optional):**
   - A cepstrum of the peak-held log spectrum estimates the pitch.
   - The envelope is an upper hull whose slope is limited by the harmonic spacing, smoothed by a lower hull.
   - The shifted spectrum is multiplied by `env(k / envRatio) / env(shifted position)`, capped at +40 dB.
4. **Phase:** phase gradient heap integration (Průša, PGHI) runs on a bucket queue. Phase spreads from the loudest bins outward, in time along the instantaneous frequency and in frequency along the group delay. This keeps clicks sharp, with no pre-echo or smear, and keeps sustained partials coherent.
5. **Synthesis:** the frames are mixed back with overlap-add in the frame-centre convention. Stereo keeps each channel's phase relative to the L+R reference, so a mono input stays mono.

`plugins/PitchR/Source/PitchEngine.h` adds the parts the plugin needs:

- Three modes: 4096, 2048 and 1024 points at 44.1 and 48 kHz, doubled at 88.2/96 kHz and doubled again at 176.4/192 kHz.
- A dry path delayed by the same latency, with a dry/wet mix.
- A/B against the original.
- A fade when switching modes.
- An exact pass-through at 0 st.

## Known limits / límites conocidos

- **Latency:** 85 ms in Clean mode. The latency is reported to the DAW, which compensates for it on playback but not while you are recording through the plugin.
- **Chords in Voice and Fast modes:** the shorter windows cannot separate close notes, so chords sound rough there.
- **Noise level:** noise comes out 1.3 to 3.3 dB quieter. At −12 st, a 3 dB drop is the physically correct result, since half the band leaves the audible range.
- **Stereo width:** the output's L/R correlation is 0.69, against 0.71 at the input. The image stays in place and gets very slightly wider.
- **Very low notes:** bass under about 30 Hz after the shift is less pure (−55 dB instead of −65 dB). Its window starts to overlap its own mirror image at DC.
