# CardputerMP3Player

Reproductor MP3 minimalista para M5Stack **Cardputer-Adv**.

---

## Librerías necesarias

Instalar desde **Tools → Manage Libraries** en Arduino IDE:

| Librería | Versión mínima | Dónde instalar |
|---|---|---|
| M5Cardputer | >= 1.1.1 | Library Manager |
| M5Unified | >= 0.2.10 | Library Manager |
| ESP8266Audio | >= 1.9.7 | Library Manager (buscar "ESP8266Audio by earlephilhower") |

> Si no aparece ESP8266Audio en el Library Manager, instalá manualmente desde:
> https://github.com/earlephilhower/ESP8266Audio
> (Code → Download ZIP → Sketch → Include Library → Add .ZIP Library)

---

## Configuración de placa

- **Board:** M5Cardputer
- **Board Manager versión:** >= 3.2.3
- **Flash Size:** recomendado 4MB (Default)
- **CPU Frequency:** 240 MHz

---

## Cómo compilar y flashear

1. Abrir `CardputerMP3Player.ino` con Arduino IDE
2. Verificar que estén instaladas todas las librerías
3. Seleccionar la placa: **Tools → Board → M5Cardputer**
4. Conectar la Cardputer-Adv por USB-C
5. Seleccionar el puerto correcto en **Tools → Port**
6. Click en **Upload** (→)

### Si el upload falla

Entrar en modo download manualmente:
1. Apagar el switch lateral
2. Mantener G0 (botón superior)
3. Conectar USB-C
4. Soltar G0
5. Hacer Upload desde el IDE

---

## Preparar la SD

- Formato: **FAT32**
- Colocar archivos `.mp3` en cualquier carpeta (el app escanea recursivamente)
- Los archivos con tags **ID3v2** (Title, Artist, Album) se muestran correctamente
- Sin tags ID3, se parsea el nombre del archivo: `Artista - Titulo.mp3`

---

## Controles

| Tecla | Acción |
|---|---|
| `ENTER` | Play / Pause |
| `,` (coma) | Canción anterior |
| `.` (punto) | Canción siguiente |
| Flecha arriba / `;` | Subir volumen |
| Flecha abajo / `/` | Bajar volumen |
| `G0` (botón lateral) | Volver a la lista de canciones |

---

## Notas

- El reproductor **sigue sonando** cuando volvés a la lista con G0
- Al terminar una canción, pasa automáticamente a la siguiente
- Si la Cardputer-Adv tiene auriculares conectados al jack 3.5mm, el audio sale por ahí
- La barra de progreso es animada si el MP3 no tiene tag de duración
