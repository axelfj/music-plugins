# EQ-R (antes Shape EQ)

## 1.0.0-alpha.2

- Nuevo nombre: **EQ-R** (antes Shape EQ). El nombre ahora dice qué hace el plugin; la R es de ROMMUSER. / Renamed to **EQ-R** (formerly Shape EQ): the name now says what the plugin does; the R stands for ROMMUSER.
- Los proyectos guardados con Shape EQ abren igual con EQ-R y conservan sus ajustes (mismo ID de plugin). / Projects saved with Shape EQ open with EQ-R and keep their settings (same plugin ID).
- El instalador borra la copia vieja llamada "Shape EQ" para que el DAW no muestre el plugin dos veces. / The installer removes the old "Shape EQ" copy so the DAW does not list the plugin twice.

## 1.0.0-alpha.1

Primera versión pública para probar. / First public test build.

- Hasta 12 bandas: Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch, Band Pass, Tilt, All Pass.
- Cortes de 6, 12, 18, 24, 36 y 48 dB/oct (Butterworth); con 12 dB el Q de la banda controla la resonancia.
- Cada banda puede trabajar en Estéreo, Izquierda, Derecha, Mid o Side.
- Bell con respuesta "matched" (M. Vicanek, 2016): la forma analógica se mantiene hasta el último octavo, sin aplastarse cerca de Nyquist.
- Cambios de parámetros con rampa (~5 ms) para que no haya clics al automatizar.
- Analizador de espectro pre/post con nodos arrastrables: arrastra para mover, rueda para el Q, doble clic para crear una banda, clic derecho para el menú.
- Auto Gain (compensa el volumen de la curva) y salida en dB.
- Latencia cero. Interfaz en español o inglés según el idioma del sistema, redimensionable.
- Formatos: VST3, CLAP (Windows, macOS, Linux) y AU (macOS).

Pendiente para 1.0.x: shelves con respuesta matched (hoy usan la receta cookbook, exacta por debajo de ~10 kHz), modo de fase lineal.
