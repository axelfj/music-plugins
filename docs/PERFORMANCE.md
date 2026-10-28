# CPU de los motores / Engine CPU

`tools/dsp-bench` mide cuánto de un núcleo necesita cada motor para correr en tiempo real: 20 s de
ruido estéreo a 48 kHz en bloques de 512, con los parámetros por defecto (o los que dice la
etiqueta), el mejor de tres pasadas. Se compila junto con los tests; ejecútalo a mano en Release:

`tools/dsp-bench` measures how much of one core each engine needs to run in real time (20 s of
stereo noise at 48 kHz, 512-sample blocks, best of three passes). It builds with the tests; run it
by hand in a Release build:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMUSIC_PLUGINS_BUILD_PLUGINS=OFF
cmake --build build --parallel 4 --target dsp_bench
./build/tools/dsp-bench/dsp_bench            # todos / all
./build/tools/dsp-bench/dsp_bench Limiter 5  # filtro por nombre, 5 s / name filter, 5 s
```

## Optimización de octubre 2026 / October 2026 optimisation

Linux x86-64, GCC 13 `-O2` (Release), un núcleo. Los números varían ±15 % entre corridas; lo
importante es la proporción. / Numbers vary ±15 % between runs; the ratio is what matters.

| Motor / Engine | Antes / Before | Después / After | Qué cambió / What changed |
| --- | ---: | ---: | --- |
| Clipper-R (+12 dB, 4x) | 4.44 % | 1.57 % | Oversampler: filtros simétricos, sumas en paralelo; un log10 por bloque |
| Clipper-R (+12 dB, 8x) | 7.48 % | 2.56 % | Oversampler |
| Saturator-R | 2.75 % | 1.35 % | Oversampler |
| Filter-R | 2.65 % | 1.26 % | Oversampler |
| Limiter-R (+6 dB) | 1.94 % | 0.69 % | Oversampler (detección true-peak) |
| Meter-R | 1.62 % | 0.47 % | Oversampler (true peak) |
| Phaser-R (12 etapas) | 3.13 % | 1.02 % | tan por aproximación de Padé, una exp2 por muestra |
| Reverb-R | 1.96 % | 0.83 % | Ganancias en caché, LFO por fasor, delays sin división |
| Resonance-R | 1.56 % | 0.74 % | FFT nueva, dos canales en una FFT |
| Tune-R | 4.76 % | 1.84 % | FFT nueva, correlación de mitad de tamaño, dos señales en una FFT |
| Vocoder-R | 4.55 % | 1.76 % | Igual que Tune-R |
| Pitch Delay-R | 3.18 % | 1.56 % | Detección de tono + reverb |
| Chorus-R | 1.01 % | 0.83 % | Delays sin división |
| Delay-R | 0.44 % | 0.38 % | Delays sin división |

Todos los cambios dan el mismo resultado que antes salvo el redondeo de float (los 225 tests siguen
pasando, también con ASan/UBSan y con FMA), excepto Limiter-R, cuyo ataque se corrigió a propósito
(ver su CHANGELOG). / Every change gives the same result up to float rounding, except Limiter-R,
whose attack was fixed on purpose (see its CHANGELOG).
