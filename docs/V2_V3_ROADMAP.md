# V2 y V3 de ROMMUSER Audio

Estado al 24 de octubre de 2026: desarrollo, no release V2 estable. Los binarios publicados de music-plugins son alpha 1.0.0; las apps offline tienen versiones iniciales distintas. Este documento no certifica compatibilidad ni calidad equivalente a una suite comercial.

[Gate de releases V2](https://github.com/axelfj/music-plugins/issues/47). [RFC V3: referencias y modelos opcionales](https://github.com/axelfj/music-plugins/issues/74).

## Prioridad

1. Seguridad de datos y mediciones fiables en las apps offline; validación del audio thread y las sesiones de plugin.
2. ShapeEQ, PressReady, ClipReady y WarmDrive: funciones del producto, mediciones y escucha.
3. Resto de la suite: completar cada issue y compartir infraestructura/presets donde corresponda.
4. Integración de referencias; después evaluación de modelos locales opcionales.

## Productos

| Producto | Trabajo V2 |
| --- | --- |
| ShapeEQ | [48](https://github.com/axelfj/music-plugins/issues/48) |
| PressReady | [49](https://github.com/axelfj/music-plugins/issues/49) |
| ClipReady | [50](https://github.com/axelfj/music-plugins/issues/50) |
| WarmDrive | [51](https://github.com/axelfj/music-plugins/issues/51) |
| DuckReady | [52](https://github.com/axelfj/music-plugins/issues/52) |
| BandPush | [53](https://github.com/axelfj/music-plugins/issues/53) |
| GateReady | [54](https://github.com/axelfj/music-plugins/issues/54) |
| EchoReady | [55](https://github.com/axelfj/music-plugins/issues/55) |
| PhaseReady | [56](https://github.com/axelfj/music-plugins/issues/56) |
| WideReady | [57](https://github.com/axelfj/music-plugins/issues/57) |
| SpaceReady | [58](https://github.com/axelfj/music-plugins/issues/58) |
| ResTame | [59](https://github.com/axelfj/music-plugins/issues/59) |
| LowMono | [60](https://github.com/axelfj/music-plugins/issues/60) |
| PunchShape | [61](https://github.com/axelfj/music-plugins/issues/61) |
| EssClean | [62](https://github.com/axelfj/music-plugins/issues/62) |
| PeakReady | [63](https://github.com/axelfj/music-plugins/issues/63) |
| MasterCheckLive | [64](https://github.com/axelfj/music-plugins/issues/64) |
| FilterReady | [65](https://github.com/axelfj/music-plugins/issues/65) |
| KickReady | [66](https://github.com/axelfj/music-plugins/issues/66) |
| WaveReady | [67](https://github.com/axelfj/music-plugins/issues/67) |
| TuneReady | [68](https://github.com/axelfj/music-plugins/issues/68) |
| VoxSynth | [69](https://github.com/axelfj/music-plugins/issues/69) |
| StutterReady | [70](https://github.com/axelfj/music-plugins/issues/70) |
| ShimmerReady | [71](https://github.com/axelfj/music-plugins/issues/71) |
| ChordReady | [72](https://github.com/axelfj/music-plugins/issues/72) |
| ArpReady | [73](https://github.com/axelfj/music-plugins/issues/73) |

## Apps offline

- [stem-organizer](https://github.com/axelfj/stem-organizer/issues/5)
- [master-check](https://github.com/axelfj/master-check/issues/4)
- [mix-ready](https://github.com/axelfj/mix-ready/issues/5)
- [ref-match](https://github.com/axelfj/ref-match/issues/4)
- [stem-lift](https://github.com/axelfj/stem-lift/issues/13)

## Distribución y evidencia necesaria

- Windows x64: instalador .exe para VST3 y CLAP.
- macOS Intel/Apple silicon: .pkg universal para VST3, AU y CLAP.
- Linux: tarball; conservar el soporte existente.
- Las apps Python entregan ejecutables de escritorio, no plugins. El script de Stem Organizer es específico de FL Studio.
- Pro Tools/AAX no está implementado. No anunciar soporte para un DAW que no se haya probado, con versión, OS y formato registrados.
- CI debe pasar DSP y validación de plugin con salida cero, conservar logs y generar checksums de los assets exactos del commit del release.
- Instalación limpia, upgrade, desinstalación, session recall, automatización, bypass/PDC y pruebas GUI requieren evidencia nativa. Firma y notarización Apple siguen en #14; las credenciales no se inventan ni se incluyen en el código.
- Las pruebas locales de DSP no prueban el instalador ni todos los hosts. Los tests con BandSeparator no prueban inferencia Demucs.

## V3

Compartir perfiles de referencia versionados con Ref Match; mantener #7, #10, #11, #30 y #36 como trabajos canónicos existentes. Modelos: baseline DSP, datasets con procedencia/licencia, separación train/test por canción/artista, pesos versionados, benchmarks y controles editables. Inferencia fuera del audio thread. Funciones manuales offline disponibles sin cuenta.

