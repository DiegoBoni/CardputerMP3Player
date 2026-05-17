# M5Stack Cardputer / Cardputer-Adv — Referencia Arduino

> Documentación compilada desde https://docs.m5stack.com/en/arduino/m5cardputer/
> Última actualización: Mayo 2026

---

## Índice

1. [Quick Start / Configuración](#1-quick-start--configuración)
2. [Batería](#2-batería)
3. [Botón (G0)](#3-botón-g0)
4. [Display](#4-display)
5. [IMU ⚠️ Solo Cardputer-Adv](#5-imu--solo-cardputer-adv)
6. [IR (Infrarrojo)](#6-ir-infrarrojo)
7. [Teclado (Keyboard)](#7-teclado-keyboard)
8. [Micrófono](#8-micrófono)
9. [MicroSD](#9-microsd)
10. [Speaker](#10-speaker)
11. [Notas importantes de hardware](#11-notas-importantes-de-hardware)

---

## 1. Quick Start / Configuración

### Instalación Arduino IDE

1. Instalar Arduino IDE desde https://www.arduino.cc/en/software
2. En **Board Manager**, buscar e instalar: **M5Cardputer**
3. En **Library Manager**, instalar: **M5Cardputer**
4. GitHub del repo oficial: https://github.com/m5stack/M5Cardputer

### Seleccionar la placa

Board option → **M5Cardputer**

### Entrar en modo download (flash)

1. Apagar el switch lateral (OFF)
2. Mantener presionado el botón **G0** (botón superior)
3. Conectar el cable USB-C
4. Soltar G0

### Estructura básica del sketch

```cpp
#include <M5Cardputer.h>

void setup() {
  M5Cardputer.begin();
  // tu init acá
}

void loop() {
  M5Cardputer.update();  // SIEMPRE llamar en loop
  // tu lógica acá
}
```

---

## 2. Batería

### Requisitos mínimos

- Board Manager >= 3.2.3
- M5Cardputer >= 1.1.1
- M5Unified >= 0.2.10

### ⚠️ Limitación de hardware

**No se puede leer el estado de carga ni la corriente de batería** — limitación del hardware.
El switch lateral debe estar en ON para habilitar la carga.

### APIs

```cpp
#include <M5Cardputer.h>

void setup() {
  M5Cardputer.begin();
}

void loop() {
  M5Cardputer.update();

  bool isCharging    = M5Cardputer.Power.isCharging();
  int batteryLevel   = M5Cardputer.Power.getBatteryLevel();    // 0-100 %
  int batteryVoltage = M5Cardputer.Power.getBatteryVoltage();  // en mV

  delay(1000);
}
```

---

## 3. Botón (G0)

El **único botón físico** que usa la Button API es el **G0** (costado superior del dispositivo).
El teclado frontal usa la Keyboard API (ver sección 7).

### Requisito

Llamar `M5Cardputer.update()` en cada iteración del loop.

### APIs

```cpp
#include "M5Cardputer.h"

void setup() {
  M5Cardputer.begin();
}

void loop() {
  M5Cardputer.update();

  if (M5Cardputer.BtnA.wasPressed())  { Serial.println("Button A Pressed"); }
  if (M5Cardputer.BtnA.wasReleased()) { Serial.println("Button A Released"); }
  if (M5Cardputer.BtnA.isPressed())   { Serial.println("Button A Held"); }
  if (M5Cardputer.BtnA.pressedFor(1000)) { Serial.println("Long Press 1s"); }
}
```

---

## 4. Display

Usa la librería **M5GFX** (basada en LovyanGFX).

### Requisitos mínimos

- M5Cardputer >= 1.1.0
- M5GFX >= 0.2.10

### APIs básicas

```cpp
#include "M5Cardputer.h"

void setup() {
  M5Cardputer.begin();
  M5Cardputer.Display.setRotation(1);  // landscape
}

void loop() {
  int w = M5Cardputer.Display.width();
  int h = M5Cardputer.Display.height();

  // Dibujar formas
  int x = rand() % w;
  int y = rand() % h;
  int r = rand() % 30 + 5;
  uint32_t c = rand();

  M5Cardputer.Display.fillCircle(x, y, r, c);
  M5Cardputer.Display.fillRect(x, y, 50, 30, TFT_RED);

  // Texto
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.println("Hola Cardputer!");

  // Limpiar
  M5Cardputer.Display.fillScreen(TFT_BLACK);
}
```

### Canvas (sprites off-screen)

```cpp
M5Canvas canvas(&M5Cardputer.Display);

void setup() {
  M5Cardputer.begin();
  canvas.setColorDepth(1);  // mono
  canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
  canvas.setPaletteColor(1, GREEN);
  canvas.setTextSize(2);
  canvas.setTextScroll(true);
}

void draw() {
  canvas.println("Texto scrollable");
  canvas.pushSprite(0, 0);  // render al display
}
```

### Pasar el display como puntero

```cpp
void drawSomething(LovyanGFX* display) {
  display->fillCircle(60, 40, 20, TFT_BLUE);
}

// uso:
drawSomething(&M5Cardputer.Display);
```

---

## 5. IMU — ⚠️ Solo Cardputer-Adv

> **CRÍTICO:** El IMU **NO está disponible** en el Cardputer regular.
> Solo funciona en el **Cardputer-Adv**.

Usar `M5.Imu` (no `M5Cardputer.Imu`).

### Requisitos mínimos

- Board Manager >= 3.2.3
- M5Cardputer >= 1.1.1
- M5Unified >= 0.2.10

### APIs

```cpp
#include <M5Cardputer.h>

m5::imu_data_t imuData;

void setup() {
  M5Cardputer.begin();
}

void loop() {
  M5.Imu.update();  // ← M5, no M5Cardputer
  imuData = M5.Imu.getImuData();

  // Acelerómetro (g)
  float ax = imuData.accel.x;
  float ay = imuData.accel.y;
  float az = imuData.accel.z;

  // Giróscopo (dps - degrees per second)
  float gx = imuData.gyro.x;
  float gy = imuData.gyro.y;
  float gz = imuData.gyro.z;

  Serial.printf("Accel: %.2f, %.2f, %.2f\n", ax, ay, az);
  Serial.printf("Gyro:  %.2f, %.2f, %.2f\n", gx, gy, gz);

  delay(100);
}
```

---

## 6. IR (Infrarrojo)

Usa la librería de terceros **Arduino-IRremote**:
https://github.com/Arduino-IRremote/Arduino-IRremote

### Pin TX

```cpp
#define IR_TX_PIN 44
```

### Requisitos mínimos

- M5Cardputer >= 1.1.0
- Librería: Arduino-IRremote (instalar via Library Manager)

### Ejemplo — Envío NEC

```cpp
#define DISABLE_CODE_FOR_RECEIVER
#define SEND_PWM_BY_TIMER
#define IR_TX_PIN 44

#include "M5Cardputer.h"
#include <IRremote.hpp>

uint16_t sAddress  = 0x1111;
uint8_t  sCommand  = 0x11;
uint8_t  sRepeats  = 0;

void setup() {
  M5Cardputer.begin();
  IrSender.begin(DISABLE_LED_FEEDBACK);
  IrSender.setSendPin(IR_TX_PIN);
}

void loop() {
  IrSender.sendNEC(sAddress, sCommand, sRepeats);
  sCommand += 1;
  delay(500);
}
```

### Protocolos disponibles (IRremote)

`sendNEC`, `sendSony`, `sendRC5`, `sendRC6`, `sendSAMSUNG`, `sendLG`, `sendPanasonic`, entre otros.

---

## 7. Teclado (Keyboard)

La clase `Keyboard_Class` es exclusiva de la Cardputer — es el API más completo y único del dispositivo.

### Activar el teclado

```cpp
auto cfg = M5Cardputer.config();
M5Cardputer.begin(cfg, true);  // segundo param = enableKeyboard
```

O simplemente:

```cpp
M5Cardputer.begin();  // el teclado se habilita por defecto
```

### API completa

```cpp
#include "M5Cardputer.h"

void loop() {
  M5Cardputer.update();

  // ¿Hubo algún cambio en el estado del teclado?
  if (M5Cardputer.Keyboard.isChange()) {

    // Cantidad de teclas presionadas actualmente
    uint8_t numPressed = M5Cardputer.Keyboard.isPressed();

    // Obtener todas las teclas presionadas
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    // Iterar caracteres escritos
    String data = "";
    for (auto ch : status.word) {
      data += ch;
    }
    if (data.length() > 0) {
      Serial.print("Chars: ");
      Serial.println(data);
    }

    // Teclas especiales
    if (status.del)   { Serial.println("Backspace"); }
    if (status.enter) { Serial.println("Enter"); }
    if (status.fn)    { Serial.println("Fn"); }
  }
}
```

### Verificar tecla específica

```cpp
// Por carácter ASCII
bool aPressed = M5Cardputer.Keyboard.isKeyPressed('a');
bool APressed = M5Cardputer.Keyboard.isKeyPressed('A');
bool onePressed = M5Cardputer.Keyboard.isKeyPressed('1');

// Por constante de tecla especial
bool enterPressed = M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER);
bool delPressed   = M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE);
```

### Obtener tecla por coordenada de grilla

El teclado es una grilla de 14 columnas (x: 0-13) × 4 filas (y: 0-3).

```cpp
// Obtener ASCII de una tecla en posición (x, y)
Point2D keyCoor;
keyCoor.x = 3;  // columna 0-13
keyCoor.y = 1;  // fila 0-3
uint8_t ascii = M5Cardputer.Keyboard.getKey(keyCoor);
```

### Ejemplo — Input de texto con display

```cpp
#include "M5Cardputer.h"

String inputText = "";

void setup() {
  M5Cardputer.begin();
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(2);
}

void loop() {
  M5Cardputer.update();

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    for (auto ch : status.word) {
      inputText += ch;
    }
    if (status.del && inputText.length() > 0) {
      inputText.remove(inputText.length() - 1);
    }
    if (status.enter) {
      Serial.println("Enviado: " + inputText);
      inputText = "";
    }

    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("> " + inputText);
  }
}
```

---

## 8. Micrófono

### ⚠️ Limitación crítica: Mic y Speaker son MUTUAMENTE EXCLUYENTES

No se pueden usar simultáneamente. Hay que hacer `.end()` de uno antes de `.begin()` del otro.

### Requisitos mínimos

- M5Cardputer >= 1.1.0
- M5Unified >= 0.2.8

### Grabar audio

```cpp
#include <M5Cardputer.h>

#define RECORD_LENGTH 1024
#define SAMPLE_RATE   17000

int16_t audioBuffer[RECORD_LENGTH];

void setup() {
  M5Cardputer.begin();

  // Speaker OFF → Mic ON
  M5Cardputer.Speaker.end();
  M5Cardputer.Mic.begin();
}

void loop() {
  M5Cardputer.update();

  // Grabar
  M5Cardputer.Mic.record(audioBuffer, RECORD_LENGTH, SAMPLE_RATE);

  // ... procesar audioBuffer ...
}
```

### Reproducir lo grabado (Mic → Speaker)

```cpp
// Terminar mic y pasar a speaker
M5Cardputer.Mic.end();
M5Cardputer.Speaker.begin();
M5Cardputer.Speaker.playRaw(audioBuffer, RECORD_LENGTH, SAMPLE_RATE);

// Cuando termine, volver al mic si hace falta:
// M5Cardputer.Speaker.end();
// M5Cardputer.Mic.begin();
```

---

## 9. MicroSD

### Pines SPI

```cpp
#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12   // ← CS pin crítico
```

> **Nota de inserción:** La tarjeta se inserta con los contactos mirando al lado OPUESTO a la pantalla.

### Requisitos mínimos

- Board Manager >= 3.2.2
- M5Cardputer >= 1.1.0
- M5Unified >= 0.2.8
- M5GFX >= 0.2.10

### Inicialización

```cpp
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

void setup() {
  M5Cardputer.begin();
  Serial.begin(115200);

  // Init SPI y SD
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

  if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
    Serial.println("SD init FALLÓ o no hay tarjeta");
    while (1);
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) { Serial.println("Sin tarjeta SD"); return; }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Size: %lluMB\n", cardSize);
  Serial.printf("Total: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Usado: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}
```

### Operaciones de archivo

```cpp
// Listar directorio
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  File root = fs.open(dirname);
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("  DIR : %s\n", file.name());
      if (levels) listDir(fs, file.path(), levels - 1);
    } else {
      Serial.printf("  FILE: %s  SIZE: %d\n", file.name(), file.size());
    }
    file = root.openNextFile();
  }
}

// Leer archivo
void readFile(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  if (!file) { Serial.println("No se pudo abrir para leer"); return; }
  while (file.available()) Serial.write(file.read());
  file.close();
}

// Escribir archivo (sobreescribe)
void writeFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) { Serial.println("No se pudo abrir para escribir"); return; }
  file.print(message);
  file.close();
}

// Agregar al final
void appendFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) { Serial.println("No se pudo abrir para append"); return; }
  file.print(message);
  file.close();
}

// Renombrar
void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  fs.rename(path1, path2);
}

// Borrar
void deleteFile(fs::FS &fs, const char *path) {
  fs.remove(path);
}

// Crear directorio
void createDir(fs::FS &fs, const char *path) {
  fs.mkdir(path);
}

// Borrar directorio
void removeDir(fs::FS &fs, const char *path) {
  fs.rmdir(path);
}
```

### Uso típico

```cpp
listDir(SD, "/", 0);
writeFile(SD, "/hola.txt", "Hola mundo!");
appendFile(SD, "/hola.txt", " Y algo más.\n");
readFile(SD, "/hola.txt");
renameFile(SD, "/hola.txt", "/renamed.txt");
deleteFile(SD, "/renamed.txt");
```

### Reproducir WAV desde SD

Para reproducir archivos WAV desde la tarjeta, usar el ejemplo de M5Unified:
`File → Examples → M5Unified → Advanced → Speaker_SD_wav_file`

Cambiar el pin CS en el ejemplo:
```cpp
// Reemplazar SDCARD_CSPIN con:
#define SDCARD_CSPIN GPIO_NUM_12
```

---

## 10. Speaker

### APIs básicas

```cpp
#include <M5Cardputer.h>

void setup() {
  M5Cardputer.begin();
  M5Cardputer.Speaker.begin();
  M5Cardputer.Speaker.setVolume(128);  // 0-255
}

void loop() {
  // Beep simple: tone(frecuencia_Hz, duración_ms)
  M5Cardputer.Speaker.tone(7000, 100);   // 7 kHz por 100ms
  delay(200);
  M5Cardputer.Speaker.tone(4000, 20);    // 4 kHz por 20ms
  delay(500);
}
```

### Reproducir audio raw

```cpp
// Reproducir buffer PCM crudo
// M5Cardputer.Speaker.playRaw(buffer, length, sampleRate, stereo, repeat, channel)
M5Cardputer.Speaker.playRaw(audioBuffer, RECORD_LENGTH, 17000);
```

### Control de volumen y estado

```cpp
M5Cardputer.Speaker.setVolume(200);   // 0-255
M5Cardputer.Speaker.end();            // apagar speaker (liberar para Mic)
M5Cardputer.Speaker.begin();          // encender speaker
bool playing = M5Cardputer.Speaker.isPlaying();
```

### ⚠️ Solo Cardputer-Adv: Jack de audio 3.5mm

En el **Cardputer-Adv**, cuando se conecta un cable al jack de 3.5mm AUX, el audio se redirige automáticamente del speaker interno al dispositivo externo (auriculares, etc.).

---

## 11. Notas importantes de hardware

### Tabla de pines relevantes

| Función         | GPIO/Pin        |
|----------------|-----------------|
| IR TX           | GPIO 44         |
| SD CS           | GPIO 12         |
| SD SCK          | GPIO 40         |
| SD MISO         | GPIO 39         |
| SD MOSI         | GPIO 14         |
| Botón G0        | GPIO 0          |

### Diferencias Cardputer vs Cardputer-Adv

| Característica    | Cardputer | Cardputer-Adv |
|------------------|-----------|----------------|
| IMU (accel+gyro)  | ❌        | ✅             |
| Jack 3.5mm AUX    | ❌        | ✅             |
| Teclado           | ✅        | ✅             |
| Display           | ✅        | ✅             |
| IR TX             | ✅        | ✅             |
| MicroSD           | ✅        | ✅             |
| Mic + Speaker     | ✅        | ✅             |
| Batería LiPo      | ✅        | ✅             |

### Limitaciones conocidas

- **Mic y Speaker son mutuamente excluyentes** — llamar `.end()` de uno antes de `.begin()` del otro.
- **No se puede leer estado de carga ni corriente de batería** — limitación de hardware.
- **IMU solo en Cardputer-Adv** — en el Cardputer regular no hay sensor IMU.
- **El switch lateral debe estar en ON** para que la batería cargue.

### Versiones mínimas recomendadas (Cardputer-Adv features)

```
Board Manager (M5Stack):  >= 3.2.3
M5Cardputer library:      >= 1.1.1
M5Unified library:        >= 0.2.10
M5GFX library:            >= 0.2.10
```

### Loop siempre necesita update()

```cpp
void loop() {
  M5Cardputer.update();  // ← OBLIGATORIO para Button, Keyboard, Power, etc.
  // ...
}
```

---

## Links de referencia

- Documentación oficial: https://docs.m5stack.com/en/arduino/m5cardputer/program
- GitHub M5Cardputer: https://github.com/m5stack/M5Cardputer
- Arduino-IRremote: https://github.com/Arduino-IRremote/Arduino-IRremote
- M5Unified ejemplos: File → Examples → M5Unified → Advanced
