# Calidad de sonido de los motores / Engine sound quality

`tools/dsp-lab` mide qué tan limpio suena cada motor: aliasing, distorsión armónica, transparencia
con ajustes neutros, precisión de los filtros cerca de Nyquist, pérdida del interpolador de los
delays, pureza del cambio de tono y caída, densidad y color de la reverb. Todo corre sobre el código
real de los plugins a 48 kHz y tarda unos segundos. En todas las métricas, menos es mejor, salvo
donde la nota dice otra cosa.

`tools/dsp-lab` measures how clean each engine sounds: aliasing, harmonic distortion, transparency at
neutral settings, filter accuracy near Nyquist, delay interpolation loss, pitch-shift purity and
reverb decay, density and colour. It runs the real engine code at 48 kHz in a few seconds. Lower is
better everywhere unless the note says otherwise.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMUSIC_PLUGINS_BUILD_PLUGINS=OFF
cmake --build build --parallel 4 --target dsp_lab
./build/tools/dsp-lab/dsp_lab            # todo / everything
./build/tools/dsp-lab/dsp_lab Reverb     # filtro por nombre / name filter
```

Cómo leerlo / How to read it: "alias" y "spurious" son toda la energía que no es el tono de entrada,
uno de sus armónicos o DC, relativa al tono (dBc). "THD" son los armónicos 2 en adelante. "time to
diffuse" es cuándo la densidad de ecos normalizada (Abel & Huang) llega a 0,95; "tail spectrum
spread" es la desviación del espectro de la cola respecto de su tendencia de 1/3 de octava (ruido
blanco = 5,6 dB). / "alias" and "spurious" are all energy that is not the input tone, one of its
harmonics or DC, relative to the tone (dBc); "THD" is harmonics 2 and up; "time to diffuse" is when
the normalised echo density reaches 0.95; "tail spectrum spread" is the tail spectrum's deviation from
its 1/3-octave trend (white noise = 5.6 dB).

## Resultados de octubre 2026 / October 2026 results

Linux x86-64, GCC 13, Release. Los puntos débiles que muestra esta corrida, cada uno con una mejora
ya probada en un prototipo / The weak spots in this run, each with a fix already tried in a prototype:

- Tune-R / Vocoder-R: el grano PSOLA se lee corrido por la fracción de muestra al revés y las marcas
  caen en muestras enteras: −8 dB de ruido en un diente de sierra de 220 Hz subido una quinta
  (prototipo: −52 dB). / PSOLA grains read the fractional offset with the wrong sign and marks sit on
  whole samples (prototype: −52 dB).
- Clipper-R: el techo estricto recorta a la frecuencia base y deshace el sobremuestreo (−30 dBc a
  cualquier factor; un limitador corto en su lugar: −75 dBc a 4x HQ, mismo pico). / The strict
  ceiling clamps at the base rate and undoes oversampling; a short limiter instead keeps −75 dBc.
- Compressor-R: hold de 2 ms en el detector de picos: −34 dB de THD a 50 Hz con release de 30 ms
  (10 ms: −113 dB). / 2 ms peak hold distorts bass with fast release (10 ms: −113 dB).
- Reverb-R: plate coloreado (28 dB de variación sin modulación) y hall que no se vuelve difuso en
  500 ms (FDN de 16 líneas con all-pass en el lazo: 6,9 dB y 78 ms). / Coloured plate, slow-to-diffuse
  hall (16-line FDN with in-loop all-passes: 6.9 dB, 78 ms).
- FractionalDelay: Hermite de 4 puntos da −23 dBc a 12 kHz en barrido (sinc Kaiser de 16 puntos:
  −88 dBc). / 4-point Hermite is −23 dBc at 12 kHz when swept (16-tap Kaiser sinc: −88 dBc).
- Synth-R y Filter-R: drives sin sobremuestreo o con 2x sin ADAA (−27 y −16 dBc). / Drives at 1x, or
  2x without ADAA.

```
group        setup                                metric                         value
saturation   Clipper-R hard 4x strict             alias, 7 kHz +12 dB            -30.5 dBc  
saturation   Clipper-R hard 2x HQ strict          alias, 7 kHz +12 dB            -27.1 dBc  
saturation   Clipper-R hard 8x strict             alias, 7 kHz +12 dB            -30.5 dBc  
saturation   Clipper-R soft 4x strict             alias, 7 kHz +12 dB            -32.2 dBc  
saturation   Clipper-R hard 2x                    alias, 7 kHz +12 dB            -36.5 dBc  
saturation   Clipper-R hard 4x                    alias, 7 kHz +12 dB            -50.1 dBc  
saturation   Clipper-R hard 2x HQ                 alias, 7 kHz +12 dB            -51.8 dBc  
saturation   Clipper-R hard 4x HQ                 alias, 7 kHz +12 dB            -75.2 dBc  
saturation   Clipper-R soft 4x                    alias, 7 kHz +12 dB            -64.9 dBc  
saturation   Saturator-R tape 2x                  alias, 7 kHz drive 18          -38.3 dBc  
saturation   Saturator-R tape 2x HQ               alias, 7 kHz drive 18          -51.7 dBc  
saturation   Saturator-R tube 2x                  alias, 7 kHz drive 18          -40.6 dBc  
saturation   Saturator-R rectify 4x               alias, 7 kHz drive 18          -47.9 dBc  
saturation   Saturator-R transistor 1x            alias, 7 kHz drive 18          -16.2 dBc  
dynamics     Compressor-R peak 1/30 ms            THD, 50 Hz -6 dBFS             -34.1 dB   release ripple distorts bass
dynamics     Compressor-R rms 1/30 ms             THD, 50 Hz -6 dBFS             -46.7 dB   release ripple distorts bass
dynamics     Compressor-R punch                   alias, 7 kHz -6 dBFS           -92.5 dBc  saturation at 1x
dynamics     Compressor-R glue                    alias, 7 kHz -6 dBFS           -96.0 dBc  saturation at 1x
dynamics     Multiband-R depth 100 %              THD, 50 Hz -12 dBFS            -66.0 dB   
dynamics     Multiband-R depth 100 %              alias, 7 kHz -12 dBFS          -92.6 dBc  
dynamics     Limiter-R transparent                THD, 100 Hz +6 dB push         -94.8 dB   bass under limiting
dynamics     Limiter-R transparent                true peak vs -1 dB ceiling       0.3 dB   > 0 = overshoot
dynamics     Limiter-R punchy                     THD, 100 Hz +6 dB push         -59.1 dB   bass under limiting
dynamics     Limiter-R punchy                     true peak vs -1 dB ceiling       0.3 dB   > 0 = overshoot
dynamics     Limiter-R aggressive                 THD, 100 Hz +6 dB push         -13.1 dB   bass under limiting
dynamics     Limiter-R aggressive                 true peak vs -1 dB ceiling       0.1 dB   > 0 = overshoot
neutral      EQ-R 8 bells at 0 dB                 null vs input                 -300.0 dB   
neutral      Multiband-R depth 0                  gain at 120 Hz (xover)          -0.0 dB   
neutral      Multiband-R depth 0                  gain at 2.5 kHz (xover)         -0.0 dB   
neutral      Transient-R at 0 / 0                 null vs input                 -300.0 dB   
neutral      Resonance-R depth 0                  null vs input                 -137.2 dB   STFT reconstruction
neutral      Tune-R fixed 0 st                    residual, 220 Hz sine          -93.4 dBc  
filter       EQ-R bell 15k +6 Q2                  error vs analog at 18000 Hz       0.1 dB   
filter       EQ-R high shelf 10k +6               error vs analog at 20000 Hz       0.4 dB   
filter       EQ-R high shelf 4k +6                error vs analog at 16000 Hz       0.0 dB   
filter       EQ-R low shelf 80 +6                 error vs analog at 40 Hz         0.0 dB   
filter       EQ-R high cut 12k Q0.7               error vs analog at 18000 Hz      -7.6 dB   
filter       Filter-R LP12 res 0                  gain at cutoff (1 kHz)          -6.0 dB   Butterworth = -3
filter       Filter-R LP24 res 0                  gain at cutoff (1 kHz)         -12.0 dB   Butterworth = -3
filter       Filter-R LP12 at 16 kHz              gain at 19 kHz vs analog        -0.5 dB   
filter       Filter-R drive 24 analog             alias, 7 kHz -6 dBFS           -16.0 dBc  2x oversampling
delay        Delay-R digital, frac 0.5            echo HF loss at 16 kHz          -3.3 dB   Hermite interpolation
delay        Delay-R digital, frac 0.5            echo HF loss at 19 kHz          -6.6 dB   
delay        FractionalDelay (Hermite)            error, 1 kHz swept tone        -89.4 dBc  
delay        FractionalDelay (Hermite)            error, 8 kHz swept tone        -35.5 dBc  
delay        FractionalDelay (Hermite)            error, 12 kHz swept tone       -23.5 dBc  
delay        FractionalDelay (Hermite)            error, 16 kHz swept tone       -14.5 dBc  
delay        Phaser-R 12 st fb 0.5                THD, 1 kHz -6 dBFS            -129.1 dB   feedback safety clip
delay        Phaser-R 12 st fb 0.8                THD, 1 kHz -6 dBFS             -33.8 dB   feedback safety clip
pitch        Tune-R fixed +7 st                   noise vs target, saw 220        -8.1 dB   mostly +-10 Hz sidebands
pitch        Tune-R fixed +7 st                   noise vs target, sine 220      -31.5 dB   
pitch        Vocoder-R harmony +7 st              noise vs target, saw 220        -7.9 dB   
reverb       Reverb-R hall 2 s mod 0              RT60 at 125 Hz (target 2)        2.0 s    
reverb       Reverb-R hall 2 s mod 0              RT60 at 500 Hz (target 2)        1.9 s    
reverb       Reverb-R hall 2 s mod 0              RT60 at 2000 Hz (target 2)       2.0 s    
reverb       Reverb-R hall 2 s mod 0              RT60 at 8000 Hz (target 2)       2.0 s    
reverb       Reverb-R hall 2 s mod 0              time to diffuse (NED 0.95)      -1.0 ms   -1 = never in 500 ms
reverb       Reverb-R hall 2 s mod 0              tail spectrum spread             8.8 dB   noise = 5.6
reverb       Reverb-R hall 2 s mod 0              L/R correlation of tail         -0.0      0 = widest
reverb       Reverb-R plate 2 s mod 0             RT60 at 125 Hz (target 2)        1.9 s    
reverb       Reverb-R plate 2 s mod 0             RT60 at 500 Hz (target 2)        2.0 s    
reverb       Reverb-R plate 2 s mod 0             RT60 at 2000 Hz (target 2)       2.0 s    
reverb       Reverb-R plate 2 s mod 0             RT60 at 8000 Hz (target 2)       2.0 s    
reverb       Reverb-R plate 2 s mod 0             time to diffuse (NED 0.95)     293.0 ms   -1 = never in 500 ms
reverb       Reverb-R plate 2 s mod 0             tail spectrum spread            28.4 dB   noise = 5.6
reverb       Reverb-R plate 2 s mod 0             L/R correlation of tail         -0.1      0 = widest
reverb       Reverb-R room 2 s mod 0              RT60 at 125 Hz (target 2)        2.0 s    
reverb       Reverb-R room 2 s mod 0              RT60 at 500 Hz (target 2)        2.0 s    
reverb       Reverb-R room 2 s mod 0              RT60 at 2000 Hz (target 2)       2.0 s    
reverb       Reverb-R room 2 s mod 0              RT60 at 8000 Hz (target 2)       2.0 s    
reverb       Reverb-R room 2 s mod 0              time to diffuse (NED 0.95)      -1.0 ms   -1 = never in 500 ms
reverb       Reverb-R room 2 s mod 0              tail spectrum spread            15.0 dB   noise = 5.6
reverb       Reverb-R room 2 s mod 0              L/R correlation of tail          0.0      0 = widest
reverb       Reverb-R hall 2 s mod 0.5            RT60 at 125 Hz (target 2)        2.0 s    
reverb       Reverb-R hall 2 s mod 0.5            RT60 at 500 Hz (target 2)        1.9 s    
reverb       Reverb-R hall 2 s mod 0.5            RT60 at 2000 Hz (target 2)       2.0 s    
reverb       Reverb-R hall 2 s mod 0.5            RT60 at 8000 Hz (target 2)       1.9 s    
reverb       Reverb-R hall 2 s mod 0.5            time to diffuse (NED 0.95)     253.0 ms   -1 = never in 500 ms
reverb       Reverb-R hall 2 s mod 0.5            tail spectrum spread             6.6 dB   noise = 5.6
reverb       Reverb-R hall 2 s mod 0.5            L/R correlation of tail          0.0      0 = widest
reverb       Reverb-R plate 2 s mod 0.5           RT60 at 125 Hz (target 2)        2.0 s    
reverb       Reverb-R plate 2 s mod 0.5           RT60 at 500 Hz (target 2)        2.0 s    
reverb       Reverb-R plate 2 s mod 0.5           RT60 at 2000 Hz (target 2)       2.0 s    
reverb       Reverb-R plate 2 s mod 0.5           RT60 at 8000 Hz (target 2)       1.7 s    
reverb       Reverb-R plate 2 s mod 0.5           time to diffuse (NED 0.95)     108.0 ms   -1 = never in 500 ms
reverb       Reverb-R plate 2 s mod 0.5           tail spectrum spread            11.4 dB   noise = 5.6
reverb       Reverb-R plate 2 s mod 0.5           L/R correlation of tail         -0.0      0 = widest
reverb       Reverb-R room 2 s mod 0.5            RT60 at 125 Hz (target 2)        2.0 s    
reverb       Reverb-R room 2 s mod 0.5            RT60 at 500 Hz (target 2)        2.1 s    
reverb       Reverb-R room 2 s mod 0.5            RT60 at 2000 Hz (target 2)       2.0 s    
reverb       Reverb-R room 2 s mod 0.5            RT60 at 8000 Hz (target 2)       1.9 s    
reverb       Reverb-R room 2 s mod 0.5            time to diffuse (NED 0.95)     196.0 ms   -1 = never in 500 ms
reverb       Reverb-R room 2 s mod 0.5            tail spectrum spread             9.2 dB   noise = 5.6
reverb       Reverb-R room 2 s mod 0.5            L/R correlation of tail          0.0      0 = widest
synth        Synth-R saw C7                       alias                          -94.2 dBc  
synth        Synth-R saw C7                       missing HF (harmonics < 20k)      -0.0 dB   0 = complete
synth        Synth-R saw G6                       alias                          -96.6 dBc  
synth        Synth-R saw G6                       missing HF (harmonics < 20k)      -0.1 dB   0 = complete
synth        Synth-R saw C5 drive 18              alias                          -26.9 dBc  
synth        Synth-R saw C5 filter drive 1        alias                          -29.2 dBc  
```
