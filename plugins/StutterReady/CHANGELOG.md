# Stutter-R (antes Stutter Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Stutter-R** (antes Stutter Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Stutter-R** (formerly Stutter Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Stutter Ready abren igual con Stutter-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Stutter Ready open with Stutter-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Stutter Ready" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Stutter Ready" copy so the DAW does not list the plugin twice.

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Captura el audio en vivo y lo repite en divisiones del tempo de 1/4 a 1/128. La primera división suena en directo mientras se graba y las siguientes la repiten exactamente: repetición k empieza k × L muestras después del gesto (test: idéntico muestra a muestra durante 6 repeticiones, y el disparo cuantizado arranca exactamente en la siguiente línea de la rejilla 1/8).
- Cada repetición termina con un crossfade de 5 ms hacia el audio anterior al corte: sin clicks (test: en un seno que no cabe entero en la división el salto máximo entre muestras se mantiene a 1,6× la pendiente natural, mientras el corte duro saltaría más de 3×).
- 8 gestos editables, disparados manteniendo su botón o con MIDI (nota 1-8 de cada octava: C, C#, D…), cuantizados a 1/4, 1/8 o 1/16 con el DAW en marcha: Stutter, Buzz (rampa 1/8 → 1/128, test: intervalos decrecientes de 12000 a 3000 muestras), Pitch up / down (test: 300 → 600 Hz), Reversa (test: las repeticiones suenan al revés), Tape stop (test: baja de velocidad y acaba en silencio), Gate y Filtro. Rampas de 1/16 a 16 tiempos.
- Efectos encadenados por gesto: gate, filtro pasa bajos / altos con barrido, pan alterno, bitcrush y eco sincronizado con realimentación.
- Secuenciador de pasos de 1/16 en 1, 2 o 4 compases: clic para pintar el gesto seleccionado, arrastre para rellenar, clic derecho para borrar. Test: con el DAW en marcha el gesto sólo suena en los pasos programados y el patrón se repite en el siguiente compás; parado, no hace nada.
- Sin gesto la señal pasa bit a bit (test). Sin latencia.
- Presets de builds: 1 compás, 2 compases, Relleno, Glitch, Frenazo, Limpio.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).

Pendiente para 1.0.x: delay por gesto con tiempo libre, gestos encadenados en una misma pulsación, grabar la automatización de los botones, modo "freeze" sin tempo.
