# Sesión — Migración HMI Axira (SquareLine → Claude Design)

Resumen de handoff para continuar en otra sesión. Producto: medidor de presión/flujo de gases medicinales,
ESP32-S3 **N16R2**, pantalla ST7796 480×320 táctil (FT5x06), ESP-IDF 6.0.1, LVGL 9.4.

---

## 1. Objetivo y estado

Reemplazar la HMI generada por **SquareLine Studio** por el diseño nuevo (16 mockups en `mockups/`),
borrando todo rastro anterior e integrándola con el firmware existente.

**Estado: las 16 pantallas están codificadas, navegables y con lógica de config real. Falta pulido y
algunos ítems de backend (abajo).** Validado visualmente en un simulador de PC.

---

## 2. Repositorios (GitHub, cuenta vbaena1972)

| Repo | Contenido | URL |
|---|---|---|
| **ClaudeHMI-FW** | Firmware ESP-IDF con la HMI nueva | https://github.com/vbaena1972/ClaudeHMI-FW |
| **ClaudeHMI-Sim** | Simulador PC (LVGL Windows/VS) | https://github.com/vbaena1972/ClaudeHMI-Sim |

> ⚠️ **Acoplados**: el sim compila las fuentes reales de `..\..\ClaudeHMI-FW\main\ui` por ruta relativa.
> Clonar **ambos como carpetas hermanas** en el mismo directorio padre.
> El firmware, tras clonar, necesita `idf.py reconfigure` (managed_components no se versiona).

Ambos ya están pusheados (rama `main`). Este archivo y cambios recientes de la última fase **aún no
commiteados** — hacer `git add . && git commit && git push` en cada repo cuando corresponda.

---

## 3. Arquitectura de la nueva HMI (`main/ui/`)

- **`ui_theme.h`** — tokens de diseño (paleta, radios, espaciados, fuentes). Alineado 1:1 al design system
  "MedGuard" del proyecto hermano `D:\SoftwareDevelop\Embedded\ClaudeHMI` (dark, Nest híbrido).
- **`ui_icons.h`** + `fonts/ui_font_tabler{16,20,28}.c` — subconjunto de iconos **Tabler** (generado con
  `lv_font_conv`, macros `UI_SYM_*`).
- **Fuentes de texto**: **Inter Medium** (`fonts/mg_font_{12,14,18,22,36}.c` + `mg_num_48.c`), copiadas de
  ClaudeHMI para paridad de diseño. (Se eliminaron las Montserrat de SquareLine.)
- **`ui_widgets.{c,h}`** — fábricas: `ui_card`, `ui_box`, `ui_label`, `ui_icon`, `ui_pill`, `ui_icon_badge`,
  `ui_nav_header`, `ui_menu_row`. (Ojo: `ui_box/ui_label/ui_icon_badge` limpian `LV_OBJ_FLAG_CLICKABLE`
  porque en LVGL9 todo `lv_obj_create` nace clickeable y robaría el toque de las tarjetas.)
- **`ui_nav.{c,h}`** — navegación con pila (`ui_nav_load/back/pop_to/show_root`).
- **`ui.{c,h}`** — orquestador: `ui_init()` crea main, muestra splash ~2.2 s → main. Dispatch de todos los
  `ui_open_*_cb` (getters perezosos + `fresh_*` para diálogos/formularios).
- **`ui_cfg.{c,h}`** — puente config: `peek → modificar → appcfg_save → refrescar main`. Setters de unidad/
  gas, validación de PIN (`general.admin.pass`), y estado de "edición de umbral" (keypad→confirm→PIN→apply).
- **`ui_form.{c,h}`** — formularios: textarea + **teclado en pantalla**, switch, dropdown, botón Guardar.
- **`ui_events.{c,h}`** — vacío (placeholder; la lógica real vive ahora en ui_cfg/pantallas).
- **`screens/`** — 16 pantallas (ver §5).

### Contrato de integración con el firmware (preservado)
- `main/main.c` `ui_refresh_task` llama a **`ui_main_update(&last,…, NULL, state, muted)`** (NULL = usa el
  **caché vivo** de AppConfig → los cambios de config se reflejan sin reiniciar).
- `components/ui_bind/` reescrito para **glifos** (no imágenes): `ui_statusbar_controller.c` (BT/WiFi/ETH/
  Nube por color) y `ui_wifi_main_icon.c` (nivel por color). API pública intacta. REQUIRES ampliado
  (storage, sensors_runtime, drivers).

---

## 4. Design system (alineado a ClaudeHMI/MedGuard, dark)

Paleta (0xRRGGBB): bg `15181e`, surface `191c22`, surface2 `12151a`, hairline `2a2f37`, text `f1f3f5`,
dim `9aa1ab`, mute `6f767f`, ok `34d399`, warn `fbbf24`, **alarm `f87171`**, info `38bdf8`, accent/teal
`2dd4bf`. Gas: O2 `1d9e75`, aire `e0a800`, N2O `2b6cb0`, vacío `dfe3e8`. Radios: card 14 / tile 11 / pill 6.
Pad 12 / gap 8 (en pantallas de 480×320 se aprieta a 8/4-5).

---

## 5. Pantallas (16) y navegación

| Pantalla | Archivo | Origen |
|---|---|---|
| Splash | `ui_splashScreen` | boot → main |
| Principal (4 estados) | `ui_mainScreen` | raíz; `ui_main_update()` desde main.c |
| Ajustes generales (menú) | `ui_generalScreen` | ⚙ de main |
| Ajustes simple | `ui_generalSimpleScreen` | general → Pantalla / Fecha |
| Información | `ui_infoScreen` | general → Información |
| Conectividad (estado) | `ui_connectivityScreen` | general → Red y nube |
| Sensores (edición) | `ui_sensorEditScreen` | general → Unidades |
| Sensores (diagnóstico) | `ui_sensorDiagScreen` | general → Alarmas |
| Keypad umbral | `ui_keypadScreen` | sensorEdit → tocar umbral |
| Confirmar cambio | `ui_confirmScreen` | keypad → Aceptar |
| PIN | `ui_pinScreen` | confirm → Confirmar con PIN |
| Config por app (QR) | `ui_bleAppScreen` | ble form → botón |
| Form WiFi/Eth/Nube/BLE | `ui_net{Wifi,Eth,Cloud,Ble}Screen` | Conectividad → tocar tarjeta |

**Flujo de umbral (completo, con persistencia):** sensorEdit → keypad (precargado) → confirm (antes→después
reales) → PIN (valida `admin.pass`, default `1234`) → `ui_edit_apply()` guarda en NVS → vuelve a sensorEdit.

---

## 6. Lógica / persistencia (hecho)

- **Sensores**: unidades P/F guardan al tocar el segmento; gas cicla y guarda; umbrales por el flujo de PIN.
- **Red**: formularios wifi/eth/cloud/ble cargan de AppConfig, guardan a NVS y **aplican a los managers**
  (`wifi_mgr`, `eth_mgr`, `transport_ble`) bajo `#ifdef ESP_PLATFORM` (el sim omite los managers).
- **JSON**: `wifi/eth/bt/cloud` ya tenían todos los campos → **no se tocó**. `AppConfig.json` intacto.

---

## 7. Simulador de PC (`ClaudeHMI-Sim`)

- Copia del sim LVGL Windows/VS del usuario, cableada a las fuentes reales del firmware.
- Stubs de dependencias ESP en `sim_app/` (`storage.h` con AppConfig reducido, `sensors_runtime.h`,
  `alarm_mgr.h`, `ui_statusbar_controller.h`, `sim_backend.c` con datos de ejemplo + `appcfg_save` no-op).
- `LvglWindowsSimulator.cpp`: ventana 480×320, `ui_init()`, cicla los 4 estados cada 2.5 s, **reanuda el
  refr timer** del driver Windows (si no, la UI dinámica se congela).
- Abrir `LVGL.slnx`, proyecto de inicio **LvglWindowsSimulator**, **Debug | x64**, F5.
- **Al agregar/quitar archivos de UI hay que actualizar `LvglWindowsSimulator.vcxproj`** (lista `ClCompile`).

---

## 8. Cómo compilar

**Firmware** (terminal ESP-IDF):
```
idf.py fullclean   # primer build tras muchos cambios de archivos
idf.py build
idf.py -p COMx flash monitor
```
**Sim**: Visual Studio → `ClaudeHMI-Sim\LVGL.slnx` → LvglWindowsSimulator → Debug|x64 → F5.

### Arreglos preexistentes de build (IDF 6.0.1, no-HMI) ya aplicados
- `components/drivers/CMakeLists.txt`: quitado `ms5837.c` (no existía).
- `CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID=y` (para `TaskStatus_t.xCoreID`).
- Buffers `core_str/core_s` a `[12]` en `diag/mem_diag.c` y `task_tracer/task_tracer.c` (format-truncation).

---

## 9. Pendiente (para próximas sesiones)

1. **`brightness`**: agregar campo por todo el pipeline (`storage.h` struct, `appcfg_defaults`,
   `cfg_from_json`, `json_from_cfg`, `AppConfig.json`, stub del sim) + `bsp_display_brightness_set()` en
   generalSimple. **Es el único hueco real del JSON.**
2. **Consumo (m³)** en main: no hay backend (hoy placeholder `-- m³ hoy`). Definir acumulador de flujo.
3. **infoScreen**: fechas de calibración/mantenimiento y FW versions son placeholders (no hay campos).
4. **Import de certificados AWS** desde SD (`cert_store`) y **BLE mesh/provisioning** avanzados: portar del
   `ui_events.c` viejo (respaldo).
5. **i18n** (idioma es/en) y **zona horaria**: hoy visuales.
6. **Escalas main**: `FLOW_AXIS_FULLSCALE_LPM` y eje de presión en `ui_mainScreen.c` a calibrar; conversión
   "bar" replica `kPa/10` del FW original (¿debería ser `/100`?).
7. **Pulido**: el label de umbral en sensorEdit muestra el valor viejo hasta re-entrar (el guardado sí ocurre).
8. **Verificación firmware**: recompilar `idf.py build` tras las fases 3-5 (aún no reprobado en hardware con
   las últimas pantallas/lógica).

---

## 10. Backup

Respaldo del HMI SquareLine anterior: `hmi_squareline_backup.tar.gz` (raíz de `ClaudeHMI-FW`, gitignored).
Contiene el `ui_events.c` (68KB) con la lógica original de wifi/eth/cloud/ble/certs para portar.
