# Gate-R (antes Gate Ready)

## 1.0.0-alpha.2

- Nuevo nombre: **Gate-R** (antes Gate Ready). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **Gate-R** (formerly Gate Ready): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Gate Ready abren igual con Gate-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Gate Ready open with Gate-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Gate Ready" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Gate Ready" copy so the DAW does not list the plugin twice.

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Gate y expansor: Umbral, Ratio (1:1 a Gate duro), Ataque 0.1–100 ms, Hold 0–1000 ms, Release 1–3000 ms, Rango (atenuación máxima, hasta −100 dB), Knee.
- **Histéresis**: abre en el umbral y solo cierra unos dB más abajo, para que no tartamudee (test: una señal que oscila ±1 dB alrededor del umbral no cierra con 3 dB de histéresis, y sí cierra con 0).
- Con la señal sobre el umbral la salida es idéntica a la entrada, bit a bit (test). Debajo, la atenuación en estado estable es exactamente el Rango (test: ±0.1 dB).
- Lookahead 0–10 ms (reporta latencia) para no comerse el transitorio (test).
- Filtros del sidechain (paso-alto y paso-bajo) con botón **Escuchar**, y sidechain externo (tests).
- Vista de nivel con el umbral arrastrable, la línea de cierre y la ganancia aplicada; medidor de reducción.
- Tres puntos de partida: Limpiar bombo, Limpiar caja, Ruido de voz.
- Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).
