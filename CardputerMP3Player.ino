/*
 * CardputerMP3Player
 * Reproductor MP3 para M5Stack Cardputer-Adv
 *
 * Librerías necesarias (Library Manager):
 *   - M5Cardputer
 *   - libhelix-mp3  (by Phil Schatzmann — buscar "helix")
 *     o instalar manualmente: https://github.com/pschatzmann/arduino-libhelix
 *
 * Controles (lista):
 *   ;     → Arriba
 *   .     → Abajo
 *   SPACE → Play
 *
 * Controles (player):
 *   ,     → Canción anterior
 *   /     → Canción siguiente
 *   ;     → Subir volumen
 *   .     → Bajar volumen
 *   SPACE → Play / Pause
 *   G0    → Volver a lista
 *
 * Pines SD: SCK=40, MISO=39, MOSI=14, CS=12
 */

#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "mp3dec.h"   // libhelix-mp3

// ─── Pines SD ────────────────────────────────────────────────────────────────
#define SD_SCK   40
#define SD_MISO  39
#define SD_MOSI  14
#define SD_CS    12

// ─── Volumen ─────────────────────────────────────────────────────────────────
#define VOL_MIN     0
#define VOL_MAX     21
#define VOL_DEFAULT 14

// ─── Colores ─────────────────────────────────────────────────────────────────
#define DISPLAY_W  240
#define DISPLAY_H  135
#define COL_BG     0x0000
#define COL_FG     0xFFFF
#define COL_ACCENT 0x07FF
#define COL_DIM    0x4208
#define COL_BAR    0x03EF
#define COL_RED    0xF800
#define COL_SEL    0x1249

// ─── Audio buffer ────────────────────────────────────────────────────────────
#define MP3_INBUF_SIZE  2048
#define PCM_BUF_FRAMES  1152   // max frames por frame MP3

static uint8_t  mp3InBuf[MP3_INBUF_SIZE];
static int16_t  pcmBuf[PCM_BUF_FRAMES * 2];  // stereo
static HMP3Decoder hMP3 = nullptr;
static File     mp3File;
static int      inBufFill  = 0;
static int      inBufOffset = 0;

// ─── Track ───────────────────────────────────────────────────────────────────
struct Track {
  String filename;
  String title;
  String artist;
  String album;
};

// ─── Estado ──────────────────────────────────────────────────────────────────
std::vector<Track> playlist;
int  currentTrack = 0;
int  volume       = VOL_DEFAULT;
bool isPlaying    = false;
bool isPaused     = false;
bool inFileList   = true;
int  listSelected = 0;
int  listOffset   = 0;
uint32_t trackStartMs  = 0;
uint32_t pausedElapsed = 0;

// ─── Utilidades ──────────────────────────────────────────────────────────────
String truncate(const String& s, int n) {
  return ((int)s.length() <= n) ? s : s.substring(0, n-1) + ".";
}
String formatTime(uint32_t s) {
  char b[8]; snprintf(b, sizeof(b), "%02d:%02d", (int)(s/60), (int)(s%60));
  return String(b);
}
uint32_t getElapsed() {
  if (isPaused)   return pausedElapsed;
  if (!isPlaying) return 0;
  return (millis() - trackStartMs) / 1000;
}

// ─── Beeps (M5 Speaker) ──────────────────────────────────────────────────────
void beep(int freq, int ms) {
  if (!isPlaying && !isPaused) {
    M5Cardputer.Speaker.tone(freq, ms);
    delay(ms + 5);
  }
}

// ─── Lector ID3v2 ────────────────────────────────────────────────────────────
void readID3Tags(const char* path, Track& t) {
  String fname = String(path);
  int slash = fname.lastIndexOf('/');
  String base = (slash >= 0) ? fname.substring(slash+1) : fname;
  base.replace(".mp3",""); base.replace(".MP3","");
  int dash = base.indexOf(" - ");
  if (dash > 0) { t.artist = base.substring(0,dash); t.title = base.substring(dash+3); }
  else          { t.title = base; t.artist = ""; }
  t.album = "";

  File f = SD.open(path, FILE_READ);
  if (!f) return;
  uint8_t hdr[10];
  if (f.read(hdr,10) < 10 || hdr[0]!='I' || hdr[1]!='D' || hdr[2]!='3') { f.close(); return; }
  uint32_t tagSize = ((uint32_t)(hdr[6]&0x7F)<<21)|((uint32_t)(hdr[7]&0x7F)<<14)|
                     ((uint32_t)(hdr[8]&0x7F)<<7)|(uint32_t)(hdr[9]&0x7F);
  uint32_t pos = 10;
  while (pos+10 < tagSize) {
    uint8_t fh[10];
    if (f.read(fh,10) < 10) break;
    pos += 10;
    char fid[5]={0}; memcpy(fid,fh,4);
    uint32_t fsz = ((uint32_t)fh[4]<<24)|((uint32_t)fh[5]<<16)|((uint32_t)fh[6]<<8)|(uint32_t)fh[7];
    if (fsz==0||fsz>512) { f.seek(f.position()+fsz); pos+=fsz; continue; }
    bool isTit=!strcmp(fid,"TIT2"), isArt=!strcmp(fid,"TPE1"), isAlb=!strcmp(fid,"TALB");
    if (isTit||isArt||isAlb) {
      uint8_t buf[513]={0}; uint32_t rd=min(fsz,(uint32_t)512);
      f.read(buf,rd); pos+=fsz;
      String val="";
      if (buf[0]==0||buf[0]==3) { for(uint32_t i=1;i<rd;i++){if(buf[i]==0)break;if(buf[i]>=32)val+=(char)buf[i];} }
      else if(buf[0]==1) { for(uint32_t i=3;i+1<rd;i+=2){uint16_t c=buf[i]|((uint16_t)buf[i+1]<<8);if(c==0)break;if(c<128)val+=(char)c;} }
      if(val.length()>0){if(isTit)t.title=val;if(isArt)t.artist=val;if(isAlb)t.album=val;}
    } else { f.seek(f.position()+fsz); pos+=fsz; }
  }
  f.close();
}

// ─── Escanear SD ─────────────────────────────────────────────────────────────
void scanDirectory(File dir, String path) {
  while (true) {
    File e = dir.openNextFile();
    if (!e) break;
    String name = String(e.name());
    if (e.isDirectory()) {
      String sub = path+"/"+name;
      File sd2 = SD.open(sub.c_str());
      if (sd2) scanDirectory(sd2, sub);
      sd2.close();
    } else {
      String nl = name; nl.toLowerCase();
      if (nl.endsWith(".mp3")) {
        String full = path+"/"+e.name();
        Track t; t.filename=full;
        readID3Tags(full.c_str(), t);
        playlist.push_back(t);
      }
    }
    e.close();
  }
}

// ─── Audio: decode loop ──────────────────────────────────────────────────────
// Decodifica un frame MP3 y lo manda al Speaker de M5Unified
// Retorna false cuando termina el archivo
bool decodeFrame() {
  if (!mp3File || !hMP3) return false;

  // Rellenar buffer de entrada
  int bytesLeft = inBufFill - inBufOffset;
  if (bytesLeft < MP3_INBUF_SIZE / 2 && mp3File.available()) {
    memmove(mp3InBuf, mp3InBuf + inBufOffset, bytesLeft);
    int toRead = MP3_INBUF_SIZE - bytesLeft;
    int got = mp3File.read(mp3InBuf + bytesLeft, toRead);
    inBufFill  = bytesLeft + got;
    inBufOffset = 0;
  }

  if (inBufFill - inBufOffset == 0) return false;  // fin de archivo

  // Buscar sync word
  uint8_t* ptr = mp3InBuf + inBufOffset;
  int bytesAvail = inBufFill - inBufOffset;
  int offset = MP3FindSyncWord(ptr, bytesAvail);
  if (offset < 0) { inBufOffset = inBufFill; return true; }
  inBufOffset += offset;
  ptr += offset;
  bytesAvail -= offset;

  // Decodificar frame
  MP3FrameInfo info;
  int err = MP3Decode(hMP3, &ptr, &bytesAvail, pcmBuf, 0);
  inBufOffset = inBufFill - bytesAvail;

  if (err != ERR_MP3_NONE) return true;  // skip frame con error

  MP3GetLastFrameInfo(hMP3, &info);

  // Mandar muestras al Speaker M5Unified
  // playRaw espera: buffer, nSamples, sampleRate, stereo, repeat, channel
  int nSamples = info.outputSamps;
  bool stereo  = (info.nChans == 2);

  // Ajustar volumen via software
  float gain = (float)volume / (float)VOL_MAX;
  for (int i = 0; i < nSamples; i++) {
    pcmBuf[i] = (int16_t)(pcmBuf[i] * gain);
  }

  M5Cardputer.Speaker.playRaw(pcmBuf, nSamples, info.samprate, stereo, 1, 0);

  return true;
}

// ─── Audio: control ──────────────────────────────────────────────────────────
void stopAudio() {
  M5Cardputer.Speaker.stop();
  if (mp3File) mp3File.close();
  if (hMP3)    { MP3FreeDecoder(hMP3); hMP3 = nullptr; }
  inBufFill = 0; inBufOffset = 0;
  isPlaying = false; isPaused = false; pausedElapsed = 0;
}

void startTrack(int idx) {
  if (idx<0||idx>=(int)playlist.size()) return;
  stopAudio();
  currentTrack = idx;

  mp3File = SD.open(playlist[idx].filename.c_str(), FILE_READ);
  if (!mp3File) return;

  hMP3 = MP3InitDecoder();
  if (!hMP3) { mp3File.close(); return; }

  // Saltar tag ID3 si existe
  uint8_t hdr[10];
  if (mp3File.read(hdr,10) == 10 && hdr[0]=='I' && hdr[1]=='D' && hdr[2]=='3') {
    uint32_t tagSize = ((uint32_t)(hdr[6]&0x7F)<<21)|((uint32_t)(hdr[7]&0x7F)<<14)|
                       ((uint32_t)(hdr[8]&0x7F)<<7)|(uint32_t)(hdr[9]&0x7F);
    mp3File.seek(10 + tagSize);
  } else {
    mp3File.seek(0);
  }

  M5Cardputer.Speaker.begin();

  trackStartMs  = millis();
  pausedElapsed = 0;
  isPlaying     = true;
  isPaused      = false;
  inFileList    = false;
}

void togglePause() {
  static uint32_t lastToggle = 0;
  if (millis() - lastToggle < 400) return;
  lastToggle = millis();
  if (!isPlaying && !isPaused) return;
  if (isPaused) {
    M5Cardputer.Speaker.begin();
    trackStartMs = millis() - pausedElapsed*1000;
    isPaused=false; isPlaying=true;
  } else {
    pausedElapsed=(millis()-trackStartMs)/1000;
    M5Cardputer.Speaker.stop();
    isPaused=true; isPlaying=false;
  }
}

// ─── UI: Lista ───────────────────────────────────────────────────────────────
void drawFileList() {
  M5Cardputer.Display.fillScreen(COL_BG);
  M5Cardputer.Display.fillRect(0, 0, DISPLAY_W, 26, COL_ACCENT);
  M5Cardputer.Display.setTextColor(COL_BG, COL_ACCENT);
  M5Cardputer.Display.setTextSize(1.5f);
  M5Cardputer.Display.setCursor(6, 6);
  char hdr[40]; snprintf(hdr,sizeof(hdr),"MP3   %d canciones",(int)playlist.size());
  M5Cardputer.Display.print(hdr);

  const int ITEM_H  = 30;
  const int TOP     = 28;
  const int VISIBLE = 3;
  int total = playlist.size();

  if (listSelected < listOffset) listOffset = listSelected;
  if (listSelected >= listOffset+VISIBLE) listOffset = listSelected-VISIBLE+1;

  for (int i=0; i<VISIBLE; i++) {
    int idx = listOffset+i;
    if (idx>=total) break;
    int y = TOP + i*ITEM_H;
    bool sel = (idx==listSelected);
    if (sel) M5Cardputer.Display.fillRect(0, y, DISPLAY_W, ITEM_H-1, COL_SEL);

    M5Cardputer.Display.setTextSize(1.5f);
    M5Cardputer.Display.setTextColor(sel?COL_ACCENT:COL_DIM, sel?COL_SEL:COL_BG);
    M5Cardputer.Display.setCursor(4, y+3);
    char num[5]; snprintf(num,sizeof(num),"%2d",idx+1);
    M5Cardputer.Display.print(num);

    M5Cardputer.Display.setTextColor(COL_FG, sel?COL_SEL:COL_BG);
    M5Cardputer.Display.setCursor(30, y+3);
    M5Cardputer.Display.print(truncate(playlist[idx].title, 18));

    M5Cardputer.Display.setTextSize(1.1f);
    M5Cardputer.Display.setTextColor(sel?COL_ACCENT:COL_DIM, sel?COL_SEL:COL_BG);
    M5Cardputer.Display.setCursor(30, y+19);
    M5Cardputer.Display.print(truncate(playlist[idx].artist, 24));
  }

  M5Cardputer.Display.setTextSize(1.1f);
  M5Cardputer.Display.setTextColor(COL_DIM, COL_BG);
  M5Cardputer.Display.setCursor(4, DISPLAY_H-10);
  M5Cardputer.Display.print(";=arriba  .=abajo  SPC=play  P=volver");
}

// ─── UI: Player ──────────────────────────────────────────────────────────────
void drawPlayer() {
  if (currentTrack<0||currentTrack>=(int)playlist.size()) return;
  Track& t = playlist[currentTrack];
  M5Cardputer.Display.fillScreen(COL_BG);

  M5Cardputer.Display.setTextColor(COL_ACCENT, COL_BG);
  M5Cardputer.Display.setTextSize(1.5f);
  M5Cardputer.Display.setCursor(4, 3);
  M5Cardputer.Display.print(truncate(t.artist.length()>0?t.artist:"Desconocido", 24));

  M5Cardputer.Display.setTextColor(COL_FG, COL_BG);
  M5Cardputer.Display.setTextSize(2.5f);
  M5Cardputer.Display.setCursor(4, 20);
  M5Cardputer.Display.print(truncate(t.title, 13));

  M5Cardputer.Display.drawFastHLine(0, 60, DISPLAY_W, COL_DIM);

  M5Cardputer.Display.setTextSize(1.5f);
  if (isPaused) {
    M5Cardputer.Display.setTextColor(COL_RED, COL_BG);
    M5Cardputer.Display.setCursor(4, 64); M5Cardputer.Display.print("|| PAUSE");
  } else {
    M5Cardputer.Display.setTextColor(COL_ACCENT, COL_BG);
    M5Cardputer.Display.setCursor(4, 64); M5Cardputer.Display.print("> PLAY");
  }
  char trkn[12]; snprintf(trkn,sizeof(trkn),"%d/%d",currentTrack+1,(int)playlist.size());
  M5Cardputer.Display.setTextColor(COL_DIM, COL_BG);
  M5Cardputer.Display.setCursor(DISPLAY_W-42, 64);
  M5Cardputer.Display.print(trkn);

  uint32_t el = getElapsed();
  M5Cardputer.Display.setTextColor(COL_FG, COL_BG);
  M5Cardputer.Display.setTextSize(1.5f);
  M5Cardputer.Display.setCursor(4, 78);
  M5Cardputer.Display.print(formatTime(el));

  int bx=4, by=92, bw=DISPLAY_W-8, bh=6;
  M5Cardputer.Display.drawRect(bx, by, bw, bh, COL_DIM);
  static int ap=0;
  if (isPlaying) ap=(ap+2)%(bw-2);
  M5Cardputer.Display.fillRect(bx+1, by+1, bw-2, bh-2, COL_BG);
  M5Cardputer.Display.fillRect(bx+1+ap, by+1, min(30,bw-2-ap), bh-2, isPlaying?COL_BAR:COL_DIM);

  M5Cardputer.Display.drawFastHLine(0, 101, DISPLAY_W, COL_DIM);

  M5Cardputer.Display.setTextSize(1.5f);
  M5Cardputer.Display.setTextColor(COL_DIM, COL_BG);
  M5Cardputer.Display.setCursor(4, 105);
  M5Cardputer.Display.print("VOL");
  char vb[5]; snprintf(vb,sizeof(vb)," %2d",volume);
  M5Cardputer.Display.setTextColor(COL_FG, COL_BG);
  M5Cardputer.Display.print(vb);

  int vx=58, vy=106, vw=80, vh=5;
  M5Cardputer.Display.drawRect(vx, vy, vw+2, vh+2, COL_DIM);
  int vf=(volume*vw)/VOL_MAX;
  M5Cardputer.Display.fillRect(vx+1, vy+1, vf, vh, COL_ACCENT);
  M5Cardputer.Display.fillRect(vx+1+vf, vy+1, vw-vf, vh, COL_BG);

  M5Cardputer.Display.setTextSize(1.1f);
  M5Cardputer.Display.setTextColor(COL_DIM, COL_BG);
  M5Cardputer.Display.setCursor(4, DISPLAY_H-10);
  M5Cardputer.Display.print(",/ prev-next  ;. vol  SPC=pause");
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  M5Cardputer.begin();
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.fillScreen(COL_BG);
  M5Cardputer.Display.setTextColor(COL_ACCENT, COL_BG);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(30, 45);
  M5Cardputer.Display.print("CardputerMP3");
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(COL_DIM, COL_BG);
  M5Cardputer.Display.setCursor(70, 70);
  M5Cardputer.Display.print("Iniciando...");

  // Speaker para beeps de navegación
  M5Cardputer.Speaker.begin();
  M5Cardputer.Speaker.setVolume(200);

  // SD
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  delay(100);
  if (!SD.begin(SD_CS, SPI, 25000000)) {
    M5Cardputer.Display.fillScreen(COL_BG);
    M5Cardputer.Display.setTextColor(COL_RED, COL_BG);
    M5Cardputer.Display.setCursor(30, 60);
    M5Cardputer.Display.print("SD ERROR!");
    while (true) delay(1000);
  }

  M5Cardputer.Display.setTextColor(COL_DIM, COL_BG);
  M5Cardputer.Display.setCursor(60, 85);
  M5Cardputer.Display.print("Leyendo SD...");

  File root = SD.open("/");
  scanDirectory(root, "");
  root.close();

  if (playlist.empty()) {
    M5Cardputer.Display.fillScreen(COL_BG);
    M5Cardputer.Display.setTextColor(COL_RED, COL_BG);
    M5Cardputer.Display.setCursor(30, 60);
    M5Cardputer.Display.print("Sin MP3 en la SD");
    while (true) delay(1000);
  }

  inFileList=true; listSelected=0; listOffset=0;
  drawFileList();
}

// ─── Loop ────────────────────────────────────────────────────────────────────
static uint32_t lastDraw = 0;

void loop() {
  M5Cardputer.update();

  // Pump audio — decodifica varios frames por iteración para audio fluido
  if (isPlaying && !isPaused) {
    // Decodificar hasta llenar el buffer del Speaker (4 frames por loop)
    for (int f = 0; f < 4; f++) {
      if (M5Cardputer.Speaker.isPlaying()) break; // buffer lleno, esperamos
      bool ok = decodeFrame();
      if (!ok) {
        // Fin de archivo → siguiente canción
        int next = (currentTrack+1) % playlist.size();
        listSelected = next;
        startTrack(next);
        drawPlayer();
        lastDraw = millis();
        break;
      }
    }
  }

  // Teclado
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    bool redraw = false;

    if (inFileList) {
      if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        if (listSelected>0) { listSelected--; beep(1000,15); redraw=true; }
      }
      if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        if (listSelected<(int)playlist.size()-1) { listSelected++; beep(1000,15); redraw=true; }
      }
      if (M5Cardputer.Keyboard.isKeyPressed(' ')) {
        beep(1800,20);
        startTrack(listSelected);
        redraw=true;
      }
      // P → volver al player de la canción en curso sin reiniciar
      if (M5Cardputer.Keyboard.isKeyPressed('p') || M5Cardputer.Keyboard.isKeyPressed('P')) {
        if (isPlaying || isPaused) {
          beep(1400,15);
          inFileList = false;
          redraw=true;
        }
      }
    } else {
      if (M5Cardputer.Keyboard.isKeyPressed(',')) {
        int prev=(currentTrack-1+playlist.size())%playlist.size();
        beep(900,15); listSelected=prev; startTrack(prev); redraw=true;
      }
      if (M5Cardputer.Keyboard.isKeyPressed('/')) {
        int next=(currentTrack+1)%playlist.size();
        beep(1300,15); listSelected=next; startTrack(next); redraw=true;
      }
      if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        if (volume<VOL_MAX) { volume++; beep(1500,12); redraw=true; }
      }
      if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        if (volume>VOL_MIN) { volume--; beep(800,12); redraw=true; }
      }
      if (M5Cardputer.Keyboard.isKeyPressed(' ')) {
        bool wasPaused = isPaused;
        togglePause();
        beep(wasPaused?1400:700, 20);
        redraw=true;
      }
    }

    if (redraw) {
      if (inFileList) drawFileList();
      else            drawPlayer();
      lastDraw=millis();
    }
  }

  // G0 → lista
  if (M5Cardputer.BtnA.wasPressed()) {
    if (!inFileList) {
      inFileList=true; listSelected=currentTrack;
      drawFileList(); lastDraw=millis();
    }
  }

  // Redibujado periódico
  if (!inFileList && millis()-lastDraw > 800) {
    drawPlayer(); lastDraw=millis();
  }
}
