# Sidechain-R (antes Duck Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Sidechain-R** (antes Duck Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Sidechain-R** (formerly Duck Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Duck Ready abren igual con Sidechain-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Duck Ready open with Sidechain-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Duck Ready" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Duck Ready" copy so the DAW does not list the plugin twice.

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Volume shaper sincronizado al tempo: la curva se calcula desde la posición del DAW, así que queda alineada al sample con la rejilla aunque se mueva el cursor, se haga loop o se inserte a mitad de la canción (test: posición exacta al sample).
- Curva paramétrica: Profundidad, Ataque, Hold, Release, Curva (forma) y Offset. Tres curvas listas: Clásico, Suave, Corto. Se puede arrastrar la curva (arriba/abajo = profundidad, izquierda/derecha = release).
- Velocidades: 1 compás, 1/2, 1/4, 1/4 T, 1/8 D, 1/8, 1/8 T, 1/16, 1/16 T, 1/32.
- Disparo por Tempo, por MIDI (cada nota reinicia la curva) o por Sidechain (detector de bombo con umbral).
- Modo multibanda: Full, Low (solo bajan los graves) o High, con crossover Linkwitz-Riley de 4.º orden que suma plano (test: < 0.05 dB).
- Sin clics: cualquier salto (reinicio, salto del transporte) se funde en 1 ms (test).
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).

Pendiente para 1.1: curva dibujada a mano con puntos libres (#17).
