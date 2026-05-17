# CardputerMP3Player

Reproductor de música MP3 minimalista para el **M5Stack Cardputer-Adv**, desarrollado en Arduino. Lee archivos MP3 desde una tarjeta microSD, muestra metadata ID3 (título, artista, álbum) y los reproduce a través del speaker interno usando el decoder Helix integrado en M5Unified.

![Platform](https://img.shields.io/badge/platform-M5Stack%20Cardputer--Adv-blue)
![Language](https://img.shields.io/badge/language-Arduino%20C%2B%2B-orange)

---

## Características

- Escaneo recursivo de archivos `.mp3` en la microSD
- Lectura de tags **ID3v2** (TIT2, TPE1, TALB) — con fallback al nombre del archivo
- UI minimalista con dos pantallas: **lista de canciones** y **reproductor**
- Control de volumen por software (21 niveles)
- Avance automático al terminar cada canción
- Sonidos de feedback en navegación
- Compatible con launchers (M5Launcher, etc.) via `.bin`

---

## Hardware requerido

- M5Stack **Cardputer-Adv** (ESP32-S3)
- Tarjeta **microSD** formateada en FAT32 con archivos `.mp3`

> ⚠️ Desarrollado y probado en el **Cardputer-Adv**. El Cardputer estándar no tiene IMU pero el reproductor de audio funciona igual en ambos.

---

## Dependencias

Instalar desde **Tools → Manage Libraries** en Arduino IDE:

| Librería | Autor | Notas |
|---|---|---|
| M5Cardputer | M5Stack | Incluye M5Unified y M5GFX |
| libhelix-mp3 | Phil Schatzmann | Buscar "helix" en Library Manager |

Si `libhelix-mp3` no aparece en el Library Manager, instalala manualmente:
1. Descargar ZIP desde https://github.com/pschatzmann/arduino-libhelix
2. Arduino IDE → **Sketch → Include Library → Add .ZIP Library**

---

## Configuración de compilación

| Parámetro | Valor |
|---|---|
| Board | M5Cardputer |
| Board Manager versión | >= 3.2.3 |
| **Partition Scheme** | **Huge APP (3MB No OTA/1MB SPIFFS)** |
| CPU Frequency | 240 MHz |

> El partition scheme es crítico — sin él el sketch no entra en flash.

---

## Preparar la microSD

- Formato: **FAT32**
- Los `.mp3` pueden estar en la raíz o en subcarpetas (se escanea recursivamente)
- Con tags ID3v2 se muestra título y artista correctamente
- Sin tags, se parsea el nombre del archivo con el formato `Artista - Titulo.mp3`
- Insertar la tarjeta con los **contactos mirando hacia el lado opuesto a la pantalla**

---

## Controles

### Pantalla: Lista de canciones

| Tecla | Acción |
|---|---|
| `;` | Navegar arriba |
| `.` | Navegar abajo |
| `SPACE` | Reproducir canción seleccionada |
| `P` | Volver al reproductor (si hay canción en curso) |

### Pantalla: Reproductor

| Tecla | Acción |
|---|---|
| `SPACE` | Play / Pause |
| `,` | Canción anterior |
| `/` | Canción siguiente |
| `;` | Subir volumen |
| `.` | Bajar volumen |
| `G0` (botón lateral) | Volver a la lista |

---

## Flashear

### Opción A — Arduino IDE (compilar y subir)

1. Abrir `CardputerMP3Player.ino` en Arduino IDE
2. Seleccionar board y partition scheme como se indica arriba
3. Conectar la Cardputer-Adv por USB-C
4. Seleccionar el puerto en **Tools → Port**
5. Click en **Upload**

**Si el upload falla**, entrar en modo download manualmente:
1. Apagar el switch lateral
2. Mantener presionado **G0**
3. Conectar USB-C
4. Soltar G0
5. Hacer Upload

### Opción B — Launcher (`.bin` precompilado)

El archivo `build/m5stack.esp32.m5stack_cardputer/CardputerMP3Player.ino.bin` puede copiarse directamente a la microSD y ejecutarse desde launchers como [M5Launcher](https://github.com/bmorcelli/M5Stick-Launcher).

---

## Pines de referencia

| Función | GPIO |
|---|---|
| SD SCK | 40 |
| SD MISO | 39 |
| SD MOSI | 14 |
| SD CS | 12 |
| Botón G0 | 0 |

---

## Estructura del proyecto

```
CardputerMP3Player/
├── CardputerMP3Player.ino   # Sketch principal
├── README.md
└── build/                   # Binarios compilados
    └── ...CardputerMP3Player.ino.bin
```

---

## Créditos

- Decoder MP3: [arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix) by Phil Schatzmann
- Framework: [M5Unified](https://github.com/m5stack/M5Unified) / [M5Cardputer](https://github.com/m5stack/M5Cardputer) by M5Stack
