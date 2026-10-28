# Primeros pasos / Getting started

[Español](#español) · [English](#english) · [Todos los manuales / All manuals](README.md)

---

## Español

Esta guía sirve para **todos** los plugins R. Léela una vez y después ve directo al manual de cada plugin.

### 1. Instalar

1. Descarga el instalador del plugin desde [Releases](https://github.com/axelfj/music-plugins/releases).
2. **Windows:** abre el `.exe`. Si sale "Windows protegió tu PC", haz clic en *Más información* → *Ejecutar de todas formas*.
3. **macOS:** abre el `.pkg`. Si dice "desarrollador no identificado", haz clic derecho → *Abrir*.
4. Abre tu DAW y **vuelve a escanear los plugins**.

### 2. Cargar un efecto (EQ, compresor, reverb...)

**FL Studio**
1. Abre el Mixer (F9) y elige la pista donde está tu sonido.
2. En un espacio libre de la columna de efectos (a la derecha), haz clic en la flechita → *Select* → busca el plugin, por ejemplo **Compressor-R**.
3. Si no aparece: *Options → Manage plugins → Find plugins*, y luego marca la estrella del plugin para que salga en la lista.

**Ableton Live**
1. En el navegador de la izquierda, ve a *Plug-ins* → *VST3*.
2. Arrastra el plugin a la pista (o haz doble clic con la pista seleccionada).
3. Si no aparece: *Preferencias → Plug-ins* → activa *Use VST3 Plug-In System Folders* y pulsa *Rescan*.

<a id="instrumento"></a>

### 3. Cargar un instrumento (Kick-R, Synth-R, Chord-R, Arpeggiator-R)

- **FL Studio:** en el Channel Rack, haz clic en **+** y elige el plugin. Toca notas en el Piano roll.
- **Ableton Live:** arrástralo a una **pista MIDI** (no a una de audio). Toca notas o dibuja un clip MIDI.

<a id="sidechain"></a>

### 4. Usar el sidechain (Compressor-R, Sidechain-R, Gate-R, Resonance-R, Vocoder-R)

El sidechain es cuando un sonido **controla** a otro. Ejemplo típico: el bombo hace que el bajo baje de volumen.

**FL Studio**
1. Pon el plugin en la pista del bajo (por ejemplo, Mixer 2). El bombo está en otra pista (por ejemplo, Mixer 1).
2. Selecciona la pista del **bombo**, haz clic derecho en la flechita de abajo de la pista del **bajo** → *Sidechain to this track*.
3. Abre el plugin, haz clic en el engranaje (arriba a la izquierda de la ventana) → pestaña *Processing*. En *Connections*, pon la entrada **Sidechain** del plugin en el número de la pista del bombo (1).
4. En el plugin activa el botón de sidechain externo (**SC EXTERNO**, **DISPARO: Sidechain**, etc.).

**Ableton Live**
1. Pon el plugin en la pista del bajo.
2. Abre el plugin con el triángulo del encabezado del dispositivo para ver la sección **Sidechain**.
3. Actívala y en *Audio From* elige la pista del bombo.
4. En el plugin activa el botón de sidechain externo.

<a id="midi"></a>

### 5. Mandar MIDI a otro instrumento (Chord-R, Arpeggiator-R)

- **FL Studio:** en el plugin, engranaje → *MIDI output* → pon un puerto (por ejemplo 1). En el instrumento que quieres tocar, pon su *MIDI input port* en el mismo número. Apaga **SONIDO** en Chord-R / Arpeggiator-R.
- **Ableton Live:** el plugin en una pista MIDI. En la pista del instrumento: *MIDI From* → la pista del plugin → el plugin, y *Monitor: In*.

### 6. Cosas que funcionan igual en todos

- **Idioma:** la interfaz sale en español si tu computadora está en español, si no en inglés.
- **Tamaño:** arrastra la esquina de abajo a la derecha para hacer la ventana más grande.
- **Ayuda:** deja el mouse quieto sobre cualquier perilla o botón y sale una explicación.
- **Valor por defecto:** doble clic en una perilla la regresa a su valor inicial.
- **Compara a igual volumen:** lo que suena más fuerte siempre parece "mejor". Muchos plugins traen **Auto Gain** o **Link** para que no te engañes.
- **Latencia:** algunos plugins (con *lookahead* o *calidad* alta) atrasan el audio unos milisegundos. Tu DAW lo compensa solo.

### 7. Palabras que vas a ver

| Palabra | Qué significa |
| --- | --- |
| **dB** (decibeles) | Cómo se mide el volumen. +6 dB suena bastante más fuerte; -6 dB más suave. |
| **dBFS** | Decibeles dentro de la computadora. **0 dBFS es el máximo**: más arriba se rompe (clip). |
| **Hz / kHz** | Qué tan grave o agudo es un sonido. 50 Hz = bombo profundo, 1 kHz = el centro, 10 kHz = brillo. |
| **Mix / Dry-Wet** | Mezcla entre el sonido original (dry) y el procesado (wet). |
| **Umbral / Threshold** | El nivel a partir del cual el plugin empieza a trabajar. |
| **Ataque / Release** | Qué tan rápido empieza y qué tan rápido deja de actuar. |
| **Sobremuestreo / Oversampling** | Procesar a más resolución para que los agudos queden limpios. Gasta más CPU. |
| **Mid / Side** | Mid es el centro del estéreo (voz, bombo, bajo). Side son los lados (ambiente, anchura). |
| **LUFS** | El volumen "percibido" de una canción. Lo usan Spotify, YouTube, etc. |
| **1/4, 1/8, 1/16** | Figuras del tempo: negra, corchea, semicorchea. **T** = tresillo, **D** = con puntillo. |

---

## English

This guide works for **every** R plugin. Read it once, then jump straight to each plugin's manual.

### 1. Install

1. Download the plugin's installer from [Releases](https://github.com/axelfj/music-plugins/releases).
2. **Windows:** open the `.exe`. If you see "Windows protected your PC", click *More info* → *Run anyway*.
3. **macOS:** open the `.pkg`. If it says "unidentified developer", right-click → *Open*.
4. Open your DAW and **rescan your plugins**.

### 2. Load an effect (EQ, compressor, reverb...)

**FL Studio**
1. Open the Mixer (F9) and select the track with your sound.
2. In an empty effect slot (right side), click the small arrow → *Select* → find the plugin, e.g. **Compressor-R**.
3. If it's missing: *Options → Manage plugins → Find plugins*, then star the plugin so it shows in the list.

**Ableton Live**
1. In the browser on the left, go to *Plug-ins* → *VST3*.
2. Drag the plugin onto the track (or double-click it with the track selected).
3. If it's missing: *Preferences → Plug-ins* → turn on *Use VST3 Plug-In System Folders* and press *Rescan*.

<a id="instrument-en"></a>

### 3. Load an instrument (Kick-R, Synth-R, Chord-R, Arpeggiator-R)

- **FL Studio:** in the Channel Rack, click **+** and pick the plugin. Play notes in the Piano roll.
- **Ableton Live:** drag it onto a **MIDI track** (not an audio track). Play notes or draw a MIDI clip.

<a id="sidechain-en"></a>

### 4. Use sidechain (Compressor-R, Sidechain-R, Gate-R, Resonance-R, Vocoder-R)

Sidechain means one sound **controls** another. Classic example: the kick makes the bass duck.

**FL Studio**
1. Put the plugin on the bass track (e.g. Mixer 2). The kick is on another track (e.g. Mixer 1).
2. Select the **kick** track, right-click the small arrow under the **bass** track → *Sidechain to this track*.
3. Open the plugin, click the cog (top-left of the window) → *Processing* tab. Under *Connections*, set the plugin's **Sidechain** input to the kick track's number (1).
4. In the plugin, turn on its external sidechain button (**EXT SIDECHAIN**, **TRIGGER: Sidechain**, etc.).

**Ableton Live**
1. Put the plugin on the bass track.
2. Expand the plugin with the triangle on the device header to see the **Sidechain** section.
3. Turn it on and choose the kick track under *Audio From*.
4. In the plugin, turn on its external sidechain button.

<a id="midi-en"></a>

### 5. Send MIDI to another instrument (Chord-R, Arpeggiator-R)

- **FL Studio:** in the plugin, cog → *MIDI output* → set a port (e.g. 1). On the instrument you want to play, set its *MIDI input port* to the same number. Turn **SOUND** off in Chord-R / Arpeggiator-R.
- **Ableton Live:** put the plugin on a MIDI track. On the instrument's track: *MIDI From* → the plugin's track → the plugin, and *Monitor: In*.

### 6. Things that work the same everywhere

- **Language:** the UI is in Spanish if your computer is in Spanish, otherwise English.
- **Size:** drag the bottom-right corner to make the window bigger.
- **Help:** hover the mouse over any knob or button to get an explanation.
- **Default value:** double-click a knob to send it back to its starting value.
- **Compare at equal loudness:** louder always sounds "better". Many plugins have **Auto Gain** or **Link** so you don't fool yourself.
- **Latency:** some plugins (with *lookahead* or high *quality*) delay the audio by a few milliseconds. Your DAW compensates for it automatically.

### 7. Words you'll see

| Word | What it means |
| --- | --- |
| **dB** (decibels) | How volume is measured. +6 dB is clearly louder; -6 dB is quieter. |
| **dBFS** | Decibels inside the computer. **0 dBFS is the maximum**: above it, sound breaks (clips). |
| **Hz / kHz** | How low or high a sound is. 50 Hz = deep kick, 1 kHz = the middle, 10 kHz = sparkle. |
| **Mix / Dry-Wet** | Blend between the original (dry) and processed (wet) sound. |
| **Threshold** | The level above which the plugin starts working. |
| **Attack / Release** | How fast it starts and how fast it stops acting. |
| **Oversampling** | Processing at a higher resolution so the highs stay clean. Uses more CPU. |
| **Mid / Side** | Mid is the centre of the stereo image (vocal, kick, bass). Side is the edges (ambience, width). |
| **LUFS** | A song's *perceived* loudness. Spotify, YouTube, etc. use it. |
| **1/4, 1/8, 1/16** | Note values: quarter, eighth, sixteenth. **T** = triplet, **D** = dotted. |
