# Temas / Themes

## Español

Todos los plugins ROMMUSER tienen temas. Abre el botón redondo con tres puntos de colores (arriba a la derecha) y elige **Tema**.

**Crear tu propio tema:** botón de colores → *Crear tema nuevo...* → clic en cada color para elegirlo (el plugin cambia en vivo) → ponle nombre → *Guardar*.

- Con **Solo para este plugin** encendido, el tema aparece solo en ese plugin (por ejemplo un EQ-R verde y un Compressor-R morado).
- Apagado, aparece en todos los plugins ROMMUSER.
- *Editar este tema...* cambia un tema tuyo. *Abrir carpeta de temas* te lleva a los archivos.

Cada plugin recuerda su tema y su tamaño.

**Dónde se guardan:**

| Carpeta | Se ve en |
|---|---|
| `Documentos/ROMMUSER/Themes/*.json` | todos los plugins |
| `Documentos/ROMMUSER/Themes/<Plugin>/*.json` (ej. `Themes/EQ-R/`) | solo ese plugin |

Para compartir un tema, manda el `.json`. Para instalar uno, ponlo en una de esas carpetas y abre el menú otra vez.

## English

Every ROMMUSER plugin has themes. Open the round button with three coloured dots (top right) and pick **Theme**. *Create a new theme...* lets you click each colour, see the plugin change live and save it with a name. **Only for this plugin** keeps the theme in that plugin; off, it shows up in all of them. Themes are small JSON files in `Documents/ROMMUSER/Themes/` (all plugins) or `Documents/ROMMUSER/Themes/<Plugin>/` (one plugin), easy to share.

## Formato / Format

```json
{
  "name": "Mi tema",
  "background": "#15161A",
  "panel": "#1F2127",
  "outline": "#2E3139",
  "text": "#E8E9EE",
  "textDim": "#8A8F9C",
  "accent": "#F2A23A",
  "good": "#58C98B",
  "warn": "#E5534B"
}
```

| Clave / Key | Qué pinta / What it colours |
|---|---|
| `background` | fondo / window background |
| `panel` | paneles, perillas, menús / panels, knobs, menus |
| `outline` | bordes, rejillas, pista de las perillas / borders, grids, knob track |
| `text` | texto principal / main text |
| `textDim` | etiquetas y texto suave / labels and soft text |
| `accent` | perillas, curvas, botones encendidos / knobs, curves, buttons that are on |
| `good` | medidores en verde / green meters |
| `warn` | alertas, clip, rojo / warnings, clip, red |

Colores en `#RRGGBB` o `#AARRGGBB`. Si falta alguno, se usa el del tema ROMMUSER. Ejemplo listo para copiar: [themes/ejemplo-oceano.json](themes/ejemplo-oceano.json).
