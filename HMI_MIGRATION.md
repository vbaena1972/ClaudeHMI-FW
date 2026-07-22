# Migración HMI: SquareLine → Claude Design

Estado de la migración de la interfaz (LVGL 9, ST7796 480×320, ESP32-S3 **N16R2**).

## ✅ Completado (Fases 0–2)

### Fase 0 — Tooling y sistema de diseño
- **Fuente de iconos Tabler propia** generada con `lv_font_conv` (subconjunto de 39 glifos usados en
  los mockups) en 3 tamaños: `main/ui/fonts/ui_font_tabler16.c`, `...20.c`, `...28.c`.
  - Los codepoints se mapearon desde el webfont Tabler v3.45.0.
  - Las variantes *filled* (`alert-triangle-filled`, `circle-check-filled`) no existen en el webfont libre
    → usan la versión *outline* (visualmente casi idéntica).
- `main/ui/ui_icons.h` — macros `UI_SYM_*` (glifos UTF-8) + `LV_FONT_DECLARE` de las fuentes de iconos.
- `main/ui/ui_theme.h` — tokens de color/tipografía/geometría extraídos de los mockups.
- `main/ui/ui_widgets.{h,c}` — fábricas compartidas (card, pill, icon-badge, header-back, menu-row, etc.).
- `main/ui/ui_nav.{h,c}` — navegación con pila de retroceso.

### Fase 1 — Esqueleto + limpieza
- **Eliminado todo el HMI SquareLine**: pantallas viejas, `images/*` (todos los bitmaps), `ui_helpers`,
  `ui_theme_manager`, `ui_themes`, `ui_comp_hook`, `ui_font_Font1`, `filelist.txt`, `project.info`,
  `main/ui/CMakeLists.txt` huérfano.
- Se conservan las fuentes **Montserrat** (12–48, rango `0x20–0xff`, incluyen acentos ES).
- `main/ui/ui.{h,c}` reescritos como orquestador (`ui_init` crea main, aplica config, arranca navegación).
- `main/ui/ui_events.{h,c}` vaciados (la lógica de config se portará en Fases 4–5 desde el backup).

### Fase 2 — Pantalla principal + integración
- `main/ui/screens/ui_mainScreen.{h,c}` — pantalla principal con **4 estados** (Normal / Advertencia /
  Alarma / Alarma-Silenciada): header con statusbar, banner de estado, 2 tarjetas (presión/flujo con
  barra de rango + marcador + min/max 24H), fila de consumo y banner de gas.
- **`main/main.c`** — `ui_refresh_task` refactorizada: ya no manipula widgets; llama a
  `ui_main_update(...)` y actualiza el reloj con `ui_main_set_clock(...)`. Añadido `#include <time.h>`.
- **`components/ui_bind/`** reescrito para usar **labels de icono (glifos)** en vez de imágenes:
  - `ui_statusbar_controller.c` (BT/WiFi/ETH/Nube por color).
  - `ui_wifi_main_icon.c` (nivel de señal por color).
  - `CMakeLists.txt`: añadidas dependencias `storage sensors_runtime drivers`.

## 🔨 Cómo compilar (en tu terminal ESP-IDF)

> Se cambió el conjunto de archivos y `main/CMakeLists.txt` usa `GLOB_RECURSE`, que **no** re-detecta
> archivos nuevos sin reconfigurar. Por eso el primer build debe ser limpio:

```
idf.py fullclean
idf.py build
idf.py -p COMx flash monitor
```

Si aparecen errores de compilación, pásamelos y los resolvemos.

## 🩹 Arreglos preexistentes (NO relacionados con la HMI)

El `idf.py fullclean` forzó una reconfiguración limpia que destapó 2 incompatibilidades latentes con
ESP-IDF 6.0.1 (el build cacheado anterior las ocultaba):

1. **`components/drivers/CMakeLists.txt`** listaba `ms5837.c`, que no existe (ni se usa en el código).
   → Eliminé esa línea. Si querías añadir ese driver, hay que crear el archivo y volver a listarlo.
2. **`components/diag/mem_diag.c`** usa `TaskStatus_t.xCoreID`, que en el FreeRTOS de IDF 6.0 solo existe
   con `CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID=y`. → Activé esa opción en `sdkconfig` y
   `sdkconfig.defaults` (preserva la funcionalidad de mostrar el core por tarea, sin tocar el código).

## 🔎 Qué validar en la placa (Fase 2)
- Arranque directo a la pantalla principal (Splash llega en Fase 3).
- Valores de presión/flujo en vivo, unidad según config, marcador moviéndose en la barra.
- Transiciones de estado (fuerza umbrales): banner y colores de tarjeta cambian
  Normal→Advertencia→Alarma; tocar el banner silencia (mute).
- Iconos de statusbar reaccionan a WiFi/ETH/BLE/Nube; reloj de cabecera.
- El engranaje (⚙) aún no navega (Fase 3).

## ⏳ Pendiente
- **Fase 3** — Splash, infoScreen, generalScreen (menú de ajustes) + navegación del engranaje.
- **Fase 4** — sensorScreen (vista/edición) + Keypad de umbral + diálogo Confirmar (reusa lógica de sensores).
- **Fase 5** — Conectividad + formularios WiFi/ETH/Nube/BLE (portar `ui_events` del backup) + PinDialog +
  Config-App-BLE (provisioning + QR).

## 📌 Ítems abiertos / decisiones
- **Consumo (m³)**: no existe en el backend → hoy se muestra placeholder `-- m³ hoy`. Definir si se
  integra el flujo en el tiempo (acumulador en `sensors_runtime`) o queda informativo.
- **Escalas de barra**: `FLOW_AXIS_FULLSCALE_LPM` (100 L/min) y el eje de presión (límite×1.1) son
  constantes ajustables en `ui_mainScreen.c` — calibrar contra el sensor real.
- **Conversión "bar"**: se replicó el factor del firmware original (`kPa/10`); si el valor esperado es
  bar físico real usar `kPa/100`. A confirmar.
- **Fechas calibración/mantenimiento** (infoScreen, Fase 3): no hay campos en `AppConfig`.

## 💾 Backup
Copia completa del HMI anterior (`main/ui`, `components/ui_bind`, `main.c`) guardada en el scratchpad de la
sesión antes de borrar (este proyecto no es repo git). Pídeme restaurar cualquier archivo si hace falta.
