# Saturator-R (antes Warm Drive)

## 1.0.0-alpha.3

- Menos CPU: sobremuestreo ~2x más rápido con el mismo filtro (2.8 % → 1.3 % de un núcleo). / Less CPU: ~2x faster oversampling with the same filter.
- Nuevo botón **HQ**: antialiasing por antiderivada (ADAA) en las cinco curvas (Rectify con cuadratura de Gauss), plano hasta 20 kHz y sin peine al mezclar. Quita 11-18 dB de aliasing con drive alto (Tape a 2x: −68 → −85 dBFS) por ~25 % más de CPU. Suma 2 muestras de latencia a 2x y 1 a 4x, que se reportan al DAW. / New **HQ** button: ADAA on all five curves, flat to 20 kHz and comb-free in the mix; 11-18 dB less aliasing at high drive for ~25 % more CPU. Adds 2 samples of latency at 2x, 1 at 4x, reported to the host.

## 1.0.0-alpha.2

- Nuevo nombre: **Saturator-R** (antes Warm Drive). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Saturator-R** (formerly Warm Drive): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Warm Drive deberían abrir con Saturator-R y conservar sus ajustes, porque el ID del plugin no cambió; si el DAW no lo encuentra, vuelve a escanear los plugins. / Projects saved with Warm Drive should open with Saturator-R and keep their settings, since the plugin ID did not change; if your DAW cannot find it, rescan your plugins.
- El instalador de Windows y macOS borra la copia vieja llamada "Warm Drive" para que el DAW no muestre el plugin dos veces (en Linux bórrala a mano). / The Windows and macOS installers remove the old "Warm Drive" copy so the DAW does not list the plugin twice (on Linux, delete it by hand).

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Saturación de 1 a 4 bandas (crossovers Linkwitz-Riley de 4.º orden con compensación de fase: las bandas suman plano, test < 0.05 dB).
- Cinco curvas por banda: **Tape** (tanh, armónicos impares), **Tube** (asimétrica, armónicos pares), **Transistor** (codo más duro), **Soft Clip** (limpia hasta recortar) y **Rectify** (pliegue, octava arriba). Todas con pendiente 1 en cero: lo suave pasa casi intacto y el Drive dice cuánto golpea lo fuerte.
- Por banda: tipo, Drive 0–36 dB, Tono (paso-bajo de 1 kHz a Off) y Mix. Luz en la curva que muestra cuánto se está saturando.
- Auto Gain: compensa la ganancia de cada curva (medida con un seno a −12 dBFS) para comparar a igual volumen (test: ±1 dB).
- Sobremuestreo 1x / 2x / 4x con filtros half-band de −110 dB; los crossovers corren a la frecuencia alta. Aliasing a 4x con 18 dB de drive: < −75 dBFS en Tape/Tube/Transistor/Soft Clip, < −60 dBFS en Rectify (es el modo sucio por diseño).
- Bloqueo de DC tras las curvas asimétricas (test: sin DC a la salida).
- Entrada, Salida y Mix global (el lado limpio del mix es la suma de bandas, así que no hay cancelaciones de fase).
- Tres puntos de partida: Calentar bajo, Brillo en voces, Empuje de drums.
- Interfaz en español o inglés según el idioma del sistema, redimensionable. Latencia reportada al DAW.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
