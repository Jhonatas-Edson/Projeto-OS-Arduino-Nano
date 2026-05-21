#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <avr/pgmspace.h>

#define TFT_CS    10
#define TFT_DC     9
#define TFT_RST    8
#define BTN_PIN    2
#define BTN_UP     3
#define BTN_DOWN   4
#define BTN_LEFT   6
#define BTN_RIGHT  7
#define BTN_ENTER  A0
#define BTN_ESC    A1
#define BUZZER_PIN 5
#define LED_POWER  A2
#define LED_HDD    A3

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

bool ligado = false;
bool prevUp = false, prevDown = false, prevToggle = false, prevEsc = false;
bool prevLeft = false, prevRight = false, prevEnter = false;
unsigned long ultimaNavMs = 0;
#define NAV_COOLDOWN_MS 250

// ---- Apps do desktop ----
#define APP_COUNT 3
int8_t appSel = -1;

const int APP_X[APP_COUNT]        = {5,      5,      5     };
const int APP_Y[APP_COUNT]        = {8,      72,     136   };
const uint16_t APP_COR[APP_COUNT] = {0xFEA0, 0x0000, 0x000F};
const char APP_NOMES[APP_COUNT][6] = {"My PC", "Term", "Pong"};

// ---- Sons ----
const unsigned int MENU_BEEP_HZ = 1800;
const unsigned int MENU_BEEP_MS = 50;

const unsigned int SUCCESS_NOTES_HZ[] = { 784, 988, 1319 };
const unsigned int SUCCESS_DURATIONS_MS[] = { 55, 55, 110 };
const byte SUCCESS_SOUND_LEN = sizeof(SUCCESS_NOTES_HZ) / sizeof(SUCCESS_NOTES_HZ[0]);

const unsigned int ALERT_NOTES_HZ[] = { 220, 196, 165 };
const unsigned int ALERT_DURATIONS_MS[] = { 90, 90, 140 };
const byte ALERT_SOUND_LEN = sizeof(ALERT_NOTES_HZ) / sizeof(ALERT_NOTES_HZ[0]);

const unsigned int SHUTDOWN_NOTES_HZ[] = { 988, 784, 659, 523, 392, 330, 262 };
const unsigned int SHUTDOWN_DURATIONS_MS[] = { 75, 75, 80, 90, 100, 120, 170 };
const byte SHUTDOWN_SOUND_LEN = sizeof(SHUTDOWN_NOTES_HZ) / sizeof(SHUTDOWN_NOTES_HZ[0]);

#define RICK_A4F 415
#define RICK_B4F 466
#define RICK_C5  523
#define RICK_C5S 554
#define RICK_E5F 622
#define RICK_F5  698

const uint16_t EASTER_BEAT_MS = 100;
const byte EASTER_GAP_PERCENT = 30;

const uint16_t RICK_CHORUS_MELODY[] PROGMEM = {
  RICK_B4F, RICK_B4F, RICK_A4F, RICK_A4F,
  RICK_F5, RICK_F5, RICK_E5F, RICK_B4F, RICK_B4F, RICK_A4F, RICK_A4F,
  RICK_E5F, RICK_E5F, RICK_C5S, RICK_C5, RICK_B4F,
  RICK_C5S, RICK_C5S, RICK_C5S, RICK_C5S,
  RICK_C5S, RICK_E5F, RICK_C5, RICK_B4F, RICK_A4F, RICK_A4F, RICK_A4F,
  RICK_E5F, RICK_C5S,
  RICK_A4F, RICK_A4F, RICK_A4F, RICK_A4F
};
const byte RICK_CHORUS_RHYTHM[] PROGMEM = {
  1, 1, 1, 1,
  3, 3, 6, 1, 1, 1, 1, 3, 3, 3, 1, 2,
  1, 1, 1, 1,
  3, 3, 3, 1, 2, 2, 2, 4, 8,
  1, 1, 1, 1
};
const byte RICK_CHORUS_LEN = sizeof(RICK_CHORUS_MELODY) / sizeof(RICK_CHORUS_MELODY[0]);

// ---- Relogio ----
uint8_t clockH = 8, clockM = 0;
unsigned long clockBase = 0;

void log(const __FlashStringHelper* msg) {
  Serial.print(F("[ArduinoOS] "));
  Serial.println(msg);
}

void hddBlink(int ms = 60) {
  digitalWrite(LED_HDD, HIGH);
  delay(ms);
  digitalWrite(LED_HDD, LOW);
}

void playToneStep(const unsigned int notes[], const unsigned int durations[], byte index) {
  tone(BUZZER_PIN, notes[index], durations[index]);
  delay(durations[index] + 25);
  noTone(BUZZER_PIN);
}

void playMenuBeep() {
  tone(BUZZER_PIN, MENU_BEEP_HZ, MENU_BEEP_MS);
  delay(MENU_BEEP_MS + 10);
  noTone(BUZZER_PIN);
}

void playSuccessSound() {
  for (byte i = 0; i < SUCCESS_SOUND_LEN; i++) {
    playToneStep(SUCCESS_NOTES_HZ, SUCCESS_DURATIONS_MS, i);
  }
}

void playAlertSound() {
  for (byte i = 0; i < ALERT_SOUND_LEN; i++) {
    playToneStep(ALERT_NOTES_HZ, ALERT_DURATIONS_MS, i);
  }
}

void playShutdownSound() {
  for (byte i = 0; i < SHUTDOWN_SOUND_LEN; i++) {
    playToneStep(SHUTDOWN_NOTES_HZ, SHUTDOWN_DURATIONS_MS, i);
  }
}

void playEasterEggSound() {
  for (byte i = 0; i < RICK_CHORUS_LEN; i++) {
    uint16_t frequency = pgm_read_word(&RICK_CHORUS_MELODY[i]);
    uint16_t duration = EASTER_BEAT_MS * (uint16_t)pgm_read_byte(&RICK_CHORUS_RHYTHM[i]);

    if (frequency > 0) {
      tone(BUZZER_PIN, frequency, duration);
    }
    delay(duration);
    noTone(BUZZER_PIN);
    delay((duration * EASTER_GAP_PERCENT) / 100);
  }
}

void desenharRelogio() {
  tft.fillRect(182, 224, 56, 13, 0x0018);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setCursor(184, 227);
  if (clockH < 10) tft.print('0');
  tft.print(clockH);
  tft.print(':');
  if (clockM < 10) tft.print('0');
  tft.print(clockM);
}

void atualizarRelogio() {
  if (!ligado) return;
  if (millis() - clockBase >= 60000UL) {
    clockBase += 60000UL;
    if (++clockM >= 60) { clockM = 0; if (++clockH >= 24) clockH = 0; }
    desenharRelogio();
  }
}

// ---- Icones ----
void icone(int x, int y, uint16_t cor, const char* nome, bool sel = false) {
  if (sel) {
    // fundo ciano brilhante + borda dupla amarela = muito visivel
    tft.fillRoundRect(x, y, 34, 30, 4, 0x07FF);
    tft.drawRoundRect(x,     y,     34, 30, 4, 0xFFE0);
    tft.drawRoundRect(x + 1, y + 1, 32, 28, 3, 0xFFE0);
  } else {
    tft.fillRoundRect(x, y, 34, 30, 4, cor);
    tft.drawRoundRect(x, y, 34, 30, 4, 0xFFFF);
  }
  tft.fillRect(x + 3, y + 2, 28, 3, 0xFFFF); // realce no topo
  int tw = strlen(nome) * 6;
  tft.setTextColor(0x0000);
  tft.setTextSize(1);
  tft.setCursor(x + (34 - tw) / 2 + 1, y + 35);
  tft.print(nome);
  tft.setTextColor(sel ? 0xFFE0 : 0xFFFF);
  tft.setCursor(x + (34 - tw) / 2, y + 34);
  tft.print(nome);
}

void decorarIconePong(int x, int y, bool sel) {
  uint16_t c  = sel ? 0xFFE0 : 0xFFFF;
  uint16_t bc = sel ? 0x07FF : 0x07FF;
  tft.fillRect(x + 4,  y + 7,  3, 15, c);   // palito esquerdo
  tft.fillRect(x + 27, y + 7,  3, 15, c);   // palito direito
  for (int dy = y + 5; dy < y + 25; dy += 5)
    tft.drawPixel(x + 17, dy, 0x4208);       // linha central tracejada
  tft.fillRect(x + 15, y + 13, 4, 4, bc);   // bola
}

void decorarIconeMyPC(int x, int y, bool sel) {
  uint16_t lc = sel ? 0x0000 : 0x8400;
  tft.fillRect(x+3, y+2, 11, 5, sel ? 0xFFE0 : 0xFCA0); // tab da pasta
  tft.drawFastHLine(x+3, y+8,  28, lc);  // abertura da pasta
  tft.drawFastHLine(x+6, y+13, 20, lc);  // arquivos dentro
  tft.drawFastHLine(x+6, y+18, 16, lc);
  tft.drawFastHLine(x+6, y+23, 18, lc);
}

void decorarIconeTerm(int x, int y, bool sel) {
  tft.setTextSize(1);
  tft.setTextColor(sel ? 0xFFE0 : 0x07E0);
  tft.setCursor(x + 5, y + 8);  tft.print(F(">_"));
  tft.setTextColor(sel ? 0xFFE0 : 0x4208);
  tft.setCursor(x + 5, y + 18); tft.print(F("$ "));
}

void desenharIconeCompleto(int8_t idx, bool sel) {
  icone(APP_X[idx], APP_Y[idx], APP_COR[idx], APP_NOMES[idx], sel);
  if (idx == 0) decorarIconeMyPC (APP_X[idx], APP_Y[idx], sel);
  if (idx == 1) decorarIconeTerm (APP_X[idx], APP_Y[idx], sel);
  if (idx == 2) decorarIconePong (APP_X[idx], APP_Y[idx], sel);
}

void redesenharApp(int8_t idx, bool sel) {
  if (idx < 0 || idx >= APP_COUNT) return;
  desenharIconeCompleto(idx, sel);
}

void navegarApp(int8_t delta) {
  int8_t prev = appSel;
  if (appSel < 0) {
    appSel = (delta > 0) ? 0 : APP_COUNT - 1;
  } else {
    appSel = (appSel + delta + APP_COUNT) % APP_COUNT;
  }
  redesenharApp(prev, false);
  redesenharApp(appSel, true);
}

// ---- Wallpaper estilo XP ----
void wallpaper() {
  // Gradiente de ceu em 4 faixas
  tft.fillRect(0,   0, 240, 65, 0x9EFF);
  tft.fillRect(0,  65, 240, 70, 0x659F);
  tft.fillRect(0, 135, 240, 45, 0x3C7F);
  tft.fillRect(0, 180, 240, 20, 0x2B5F);
  // Faixa verde (chao)
  tft.fillRect(0, 200, 240, 22, 0x0480);
  // Colinas simples
  tft.fillRect(0, 197, 90,   5, 0x0640);
  tft.fillRect(90, 194, 80,  8, 0x05A0);
  tft.fillRect(170, 196, 70, 6, 0x0580);
  // Linha de horizonte
  tft.drawFastHLine(0, 200, 240, 0x06E0);
}

// ---- Taskbar ----
void desenharTaskbar() {
  tft.fillRect(0, 222, 240, 18, 0x0018);
  tft.drawFastHLine(0, 222, 240, 0x4A8A); // borda topo

  // Botao Start
  tft.fillRoundRect(2, 224, 46, 14, 3, 0x0340);
  tft.fillRect(3, 224, 44, 7, 0x0580);    // highlight superior
  tft.drawRoundRect(2, 224, 46, 14, 3, 0x07C0);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setCursor(8, 228);
  tft.print(F("start"));

  // Separador
  tft.drawFastVLine(50, 223, 16, 0x4208);

  // Relogio
  desenharRelogio();
}

// ---- Desktop ----
void desktop() {
  ligado = true;
  clockH = 8; clockM = 0; clockBase = millis();

  appSel = -1;
  wallpaper();

  for (int8_t i = 0; i < APP_COUNT; i++) {
    desenharIconeCompleto(i, false);
  }

  desenharTaskbar();
}

void alternar() { if (ligado) desligar(); else boot(); }

// ---- Loop serial ----
bool lerComando(char* buf, int maxlen) {
  if (!Serial.available()) return false;
  int len = 0;
  unsigned long t = millis();
  while (millis() - t < 200) {
    while (Serial.available() && len < maxlen - 1) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') goto done;
      if (c >= 32) buf[len++] = tolower(c);
    }
  }
  done:
  buf[len] = '\0';
  return len > 0;
}

// ---- Setup ----
void setup() {
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(0x0000);
  pinMode(BTN_PIN,   INPUT_PULLUP);
  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);
  pinMode(BTN_ESC,   INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_HDD,   OUTPUT);
  digitalWrite(LED_POWER, LOW);
  digitalWrite(LED_HDD,   LOW);
  boot();
}

// ---- Loop principal ----
void loop() {
  atualizarRelogio();

  bool nowToggle = (digitalRead(BTN_PIN)   == LOW);
  bool nowEsc    = (digitalRead(BTN_ESC)   == LOW);
  bool nowDown   = (digitalRead(BTN_DOWN)  == LOW);
  bool nowUp     = (digitalRead(BTN_UP)    == LOW);
  bool nowLeft   = (digitalRead(BTN_LEFT)  == LOW);
  bool nowRight  = (digitalRead(BTN_RIGHT) == LOW);
  bool nowEnter  = (digitalRead(BTN_ENTER) == LOW);

  if (nowToggle && !prevToggle) { playMenuBeep(); alternar(); }
  if (nowEsc    && !prevEsc)    { playMenuBeep(); desktop(); prevEsc = nowEsc; return; }

  if (millis() - ultimaNavMs >= NAV_COOLDOWN_MS) {
    if (nowDown  && !prevDown  && ligado) { playMenuBeep(); navegarApp(+1); ultimaNavMs = millis(); }
    if (nowUp    && !prevUp    && ligado) { playMenuBeep(); navegarApp(-1); ultimaNavMs = millis(); }
  }

  if (nowEnter && !prevEnter && ligado && appSel >= 0) { abrirApp(appSel); return; }

  prevToggle = nowToggle;
  prevEsc    = nowEsc;
  prevDown   = nowDown;
  prevUp     = nowUp;
  prevLeft   = nowLeft;
  prevRight  = nowRight;
  prevEnter  = nowEnter;

  char buf[16];
  if (!lerComando(buf, 16)) return;

  Serial.print(F("[ArduinoOS] Comando: "));
  Serial.println(buf);

  if (strcmp(buf, "ligar") == 0) {
    if (ligado) {
      playAlertSound();
      log(F("Tela ja ligada."));
    }
    else { hddBlink(); boot(); }
  } else if (strcmp(buf, "desligar") == 0) {
    if (!ligado) {
      playAlertSound();
      log(F("Tela ja desligada."));
    }
    else { hddBlink(); desligar(); }
  } else if (strcmp(buf, "status") == 0) {
    hddBlink(40);
    playMenuBeep();
    Serial.print(F("[ArduinoOS] Estado: "));
    Serial.println(ligado ? F("LIGADO") : F("DESLIGADO"));
  } else if (strcmp(buf, "reiniciar") == 0) {
    hddBlink();
    log(F("Reiniciando..."));
    desligar(); delay(500); boot();
  } else if (strcmp(buf, "ajuda") == 0) {
    Serial.println(F("ligar|desligar|reiniciar|status|easter"));
    hddBlink(40);
    playMenuBeep();
  } else if (strcmp(buf, "easter") == 0) {
    hddBlink();
    playEasterEggSound();
  } else {
    playAlertSound();
    log(F("Comando desconhecido. Digite 'ajuda'."));
  }
}

// ============================================================
// ---- Pong ----
// ============================================================
#define PP_TOP       20
#define PP_H        280
#define PP_W        240
#define PP_PDL_W      6
#define PP_PDL_H     44
#define PP_PDL_SPD    5
#define PP_BALL_SZ    7
#define PP_MAX_SCR    5
#define PP_TICK_MS   28
#define PP1X          8
#define PP2X        226   // PP_W - PP_PDL_W - 8

enum PongEstado { PE_MENU, PE_JOGANDO, PE_PONTO, PE_FIM };
PongEstado pongEstado;

int   pP1Y, pP2Y, pOP1Y, pOP2Y;
float pBX, pBY, pBDX, pBDY;
int   pOBX, pOBY;
int   pPl1, pPl2;
unsigned long pTick;

void pongPlacar() {
  tft.fillRect(0, 0, PP_W, PP_TOP, 0x0000);
  tft.setTextColor(0xFFFF); tft.setTextSize(1);
  tft.setCursor(82, 6);
  tft.print(pPl1); tft.print(F("   x   ")); tft.print(pPl2);
}

void pongResetBola() {
  pBX  = PP_W / 2.0;
  pBY  = PP_TOP + PP_H / 2.0;
  pBDX = (random(2) == 0) ?  2.5 : -2.5;
  pBDY = (random(2) == 0) ?  1.5 : -1.5;
  pOBX = (int)pBX; pOBY = (int)pBY;
}

void pongReset() {
  pPl1 = pPl2 = 0;
  pP1Y = pP2Y = PP_TOP + (PP_H - PP_PDL_H) / 2;
  pOP1Y = pP1Y; pOP2Y = pP2Y;
  pongResetBola();
}

void pongTela() {
  tft.fillRect(0, PP_TOP, PP_W, PP_H, 0x0000);
  for (int y = PP_TOP; y < PP_TOP + PP_H; y += 8)
    tft.fillRect(119, y, 2, 4, 0x2104);
  tft.fillRect(PP1X, pP1Y, PP_PDL_W, PP_PDL_H, 0xFFFF);
  tft.fillRect(PP2X, pP2Y, PP_PDL_W, PP_PDL_H, 0xFFFF);
  tft.fillRect((int)pBX, (int)pBY, PP_BALL_SZ, PP_BALL_SZ, 0x07FF);
  pOP1Y = pP1Y; pOP2Y = pP2Y;
  pOBX = (int)pBX; pOBY = (int)pBY;
}

void pongMenuTela() {
  tft.fillScreen(0x0000);

  // Titulo
  tft.setTextSize(3); tft.setTextColor(0xFFFF);
  tft.setCursor(68, 38); tft.print(F("PONG"));

  // Controles
  tft.setTextSize(1); tft.setTextColor(0x8410);
  tft.setCursor(72, 98); tft.print(F("CONTROLES"));
  tft.drawFastHLine(30, 107, 180, 0x4208);
  tft.setTextColor(0x07FF);
  tft.setCursor(36, 115); tft.print(F("UP / DOWN  =  Jogador 1"));
  tft.setCursor(36, 127); tft.print(F("LT / RT    =  Jogador 2"));

  // Regra
  tft.setTextColor(0x8410);
  tft.setCursor(84, 153); tft.print(F("REGRA"));
  tft.drawFastHLine(30, 162, 180, 0x4208);
  tft.setTextColor(0xFFE0);
  tft.setCursor(22, 170); tft.print(F("Primeiro a "));
  tft.print(PP_MAX_SCR);
  tft.print(F(" pontos vence"));

  // Acoes
  tft.setTextColor(0x07E0);
  tft.setCursor(48, 205); tft.print(F("ENTER  para comecar"));
  tft.setTextColor(0xF800);
  tft.setCursor(60, 218); tft.print(F("ESC    para sair"));

  // Credito
  tft.setTextColor(0x4208);
  tft.setCursor(42, 298); tft.print(F("feito por HIRU AKIAMA"));
}

void pongFimTela() {
  tft.fillScreen(0x0000);
  tft.setTextSize(2); tft.setTextColor(0xFFFF);
  tft.setCursor(28, 80); tft.print(F("FIM DE JOGO"));
  tft.setTextSize(2);
  if (pPl1 >= PP_MAX_SCR) { tft.setTextColor(0x07FF); tft.setCursor(48, 130); tft.print(F("P1 VENCE!")); }
  else                     { tft.setTextColor(0xF800); tft.setCursor(48, 130); tft.print(F("P2 VENCE!")); }
  tft.setTextColor(0xFFFF);
  tft.setCursor(72, 175); tft.print(pPl1); tft.print(F(" x ")); tft.print(pPl2);
  tft.setTextSize(1); tft.setTextColor(0xFFE0);
  tft.setCursor(22, 240); tft.print(F("ENTER = revanche   ESC = sair"));
}

void pongFrame() {
  tft.fillRect(PP1X, pOP1Y, PP_PDL_W, PP_PDL_H, 0x0000);
  tft.fillRect(PP2X, pOP2Y, PP_PDL_W, PP_PDL_H, 0x0000);
  tft.fillRect(pOBX, pOBY,  PP_BALL_SZ, PP_BALL_SZ, 0x0000);
  // redraw dashes apagadas pelos palitos/bola
  for (int y = PP_TOP; y < PP_TOP + PP_H; y += 8)
    tft.fillRect(119, y, 2, 4, 0x2104);
  pOP1Y = pP1Y; pOP2Y = pP2Y;
  pOBX = (int)pBX; pOBY = (int)pBY;
  tft.fillRect(PP1X, pP1Y, PP_PDL_W, PP_PDL_H, 0xFFFF);
  tft.fillRect(PP2X, pP2Y, PP_PDL_W, PP_PDL_H, 0xFFFF);
  tft.fillRect((int)pBX, (int)pBY, PP_BALL_SZ, PP_BALL_SZ, 0x07FF);
}

void pongEntrada() {
  if (digitalRead(BTN_UP)    == LOW) pP1Y -= PP_PDL_SPD;
  if (digitalRead(BTN_DOWN)  == LOW) pP1Y += PP_PDL_SPD;
  if (digitalRead(BTN_LEFT)  == LOW) pP2Y -= PP_PDL_SPD;
  if (digitalRead(BTN_RIGHT) == LOW) pP2Y += PP_PDL_SPD;
  int bot = PP_TOP + PP_H - PP_PDL_H;
  pP1Y = constrain(pP1Y, PP_TOP, bot);
  pP2Y = constrain(pP2Y, PP_TOP, bot);
}

void pongAtualizarBola() {
  pBX += pBDX; pBY += pBDY;
  int bot = PP_TOP + PP_H - PP_BALL_SZ;

  if (pBY <= PP_TOP) { pBY = PP_TOP; pBDY = -pBDY; tone(BUZZER_PIN, 280, 20); }
  if (pBY >= bot)    { pBY = bot;    pBDY = -pBDY; tone(BUZZER_PIN, 280, 20); }

  // P1 paddle
  if (pBX <= PP1X + PP_PDL_W && pBX >= PP1X &&
      pBY + PP_BALL_SZ >= pP1Y && pBY <= pP1Y + PP_PDL_H) {
    pBX = PP1X + PP_PDL_W; pBDX = abs(pBDX);
    if (abs(pBDX) < 5.5f) pBDX *= 1.07f;
    if (abs(pBDY) < 5.5f) pBDY *= 1.07f;
    tone(BUZZER_PIN, 660, 40);
  }
  // P2 paddle
  if (pBX + PP_BALL_SZ >= PP2X && pBX + PP_BALL_SZ <= PP2X + PP_PDL_W &&
      pBY + PP_BALL_SZ >= pP2Y && pBY <= pP2Y + PP_PDL_H) {
    pBX = PP2X - PP_BALL_SZ; pBDX = -abs(pBDX);
    if (abs(pBDX) < 5.5f) pBDX *= 1.07f;
    if (abs(pBDY) < 5.5f) pBDY *= 1.07f;
    tone(BUZZER_PIN, 660, 40);
  }

  if (pBX + PP_BALL_SZ < 0) {
    pPl2++; tone(BUZZER_PIN, 880, 100); delay(120); tone(BUZZER_PIN, 1100, 180);
    pongPlacar(); pongEstado = PE_PONTO;
  }
  if (pBX > PP_W) {
    pPl1++; tone(BUZZER_PIN, 880, 100); delay(120); tone(BUZZER_PIN, 1100, 180);
    pongPlacar(); pongEstado = PE_PONTO;
  }
}

void rodarPong() {
  pongReset();
  pongEstado = PE_MENU;
  pongMenuTela();

  while (true) {
    if (digitalRead(BTN_ESC) == LOW) { delay(200); return; }

    switch (pongEstado) {
      case PE_MENU:
        if (digitalRead(BTN_ENTER) == LOW) {
          delay(200);
          pongReset(); pongPlacar(); pongTela();
          pongEstado = PE_JOGANDO;
        }
        break;

      case PE_JOGANDO:
        if (millis() - pTick >= PP_TICK_MS) {
          pTick = millis();
          pongEntrada();
          pongAtualizarBola();
          if (pongEstado == PE_JOGANDO) pongFrame();
        }
        break;

      case PE_PONTO:
        delay(1000);
        if (pPl1 >= PP_MAX_SCR || pPl2 >= PP_MAX_SCR) {
          pongEstado = PE_FIM; pongFimTela();
        } else {
          pongResetBola(); pongTela(); pongEstado = PE_JOGANDO;
        }
        break;

      case PE_FIM:
        if (digitalRead(BTN_ENTER) == LOW) {
          delay(200);
          pongReset(); pongPlacar(); pongTela();
          pongEstado = PE_JOGANDO;
        }
        break;
    }
  }
}

void rodarMyPC() {
  tft.fillScreen(0x0000);
  tft.fillRect(0, 0, 240, 22, 0xFEA0);
  tft.setTextColor(0x0000); tft.setTextSize(2);
  tft.setCursor(8, 4); tft.print(F("My PC"));
  tft.setTextColor(0xFFFF); tft.setTextSize(1);
  tft.setCursor(8,  32); tft.print(F("Jhonatas Edson"));
  tft.setCursor(8,  42); tft.print(F(" OS, soldagem e montagem"));
  tft.setCursor(8,  60); tft.print(F("Vinicius"));
  tft.setCursor(8,  70); tft.print(F(" Sons do sistema"));
  tft.setCursor(8,  88); tft.print(F("Hiru Akiama"));
  tft.setCursor(8,  98); tft.print(F(" Jogo PONG"));
  tft.setTextColor(0x4208);
  tft.setCursor(68, 150); tft.print(F("ESC sair"));
  while (digitalRead(BTN_ESC) != LOW);
  delay(150);
}

void abrirApp(int8_t idx) {
  if (idx == 0) rodarMyPC();
  else if (idx == 1) rodarTerminal();
  else if (idx == 2) rodarPong();
  if (ligado) desktop();
}

// ============================================================
// ---- Terminal v2 - Estilo Notepad (Sons of the Forest) ----
// Monta comando: [ACAO] + [ALVO], ENTER executa
// ============================================================
#define TRM_VN 9
#define TRM_ON 8

const char TRM_V[TRM_VN][10] = {
  "EXEC","STATUS","PISCAR","TOCAR","DESLIGAR","REINICIAR","VOLTAR","LIGAR","APAGAR"
};
const char TRM_O[TRM_ON][10] = {
  "PONG","SISTEMA","LEDS","MUSICA","DESKTOP","LED_PWR","LED_HDD","BUZZER"
};

// posicoes Y dos 5 itens do scroll-wheel (-2 a +2)
const uint8_t TRM_Y[5] = {68, 100, 140, 178, 208};

int8_t trmV = 0, trmO = 0, trmCol = 0;

void trmPalavra(const char* w, int8_t col, uint8_t sz, uint8_t y, uint16_t cor) {
  int tw = strlen(w) * 6 * sz;
  tft.setTextColor(cor); tft.setTextSize(sz);
  tft.setCursor(col * 120 + (120 - tw) / 2, y);
  tft.print(w);
}

void trmColuna(int8_t col) {
  int8_t idx = (col == 0) ? trmV   : trmO;
  int8_t n   = (col == 0) ? TRM_VN : TRM_ON;
  bool   at  = (col == trmCol);
  int    cx  = col * 120;

  tft.fillRect(cx, 26, 120, 210, 0x0000);
  for (uint8_t y = 36; y < 236; y += 20)
    tft.drawFastHLine(cx, y, 120, 0x0180);

  if (at) tft.fillRoundRect(cx + 3, 132, 114, 22, 3, 0x0060);

  // active: dim→mid→bright→mid→dim green  |  inactive: muito dim
  const uint16_t COR[5] = {0x0180, 0x0320, 0x07E0, 0x0320, 0x0180};
  const uint16_t INV[5] = {0x00C0, 0x0140, 0x0280, 0x0140, 0x00C0};
  const uint8_t  SZ [5] = {1, 1, 2, 1, 1};

  for (int8_t d = -2; d <= 2; d++) {
    int8_t i = (idx + d + n) % n;
    const char* w = (col == 0) ? TRM_V[i] : TRM_O[i];
    trmPalavra(w, col, SZ[d+2], TRM_Y[d+2], at ? COR[d+2] : INV[d+2]);
  }
}

void trmPreview() {
  tft.fillRect(0, 250, 240, 44, 0x0000);
  tft.drawFastHLine(0, 250, 240, 0x0320);
  tft.drawFastHLine(0, 251, 240, 0x0180);

  char cmd[22];
  strcpy(cmd, TRM_V[trmV]);
  strcat(cmd, " ");
  strcat(cmd, TRM_O[trmO]);
  int tw = strlen(cmd) * 12;
  tft.setTextColor(0x07E0); tft.setTextSize(2);
  tft.setCursor((240 - tw) / 2, 262);
  tft.print(cmd);

  tft.setTextColor(0x07E0);
  tft.setCursor((240 - tw) / 2 + tw + 2, 262);
  tft.print(F("_"));
}

void trmTudo() {
  // header bar
  tft.fillRect(0, 0, 240, 22, 0x0060);
  tft.drawFastHLine(0, 22, 240, 0x07E0);
  tft.setTextColor(0x07E0); tft.setTextSize(1);
  tft.setCursor(6, 7); tft.print(F("> TERMINAL v1.0"));
  tft.setTextColor(0x0280);
  tft.setCursor(120, 7); tft.print(F("^v=scroll  <>  ENTER"));

  // area principal preta
  tft.fillRect(0, 22, 240, 228, 0x0000);
  for (uint8_t y = 32; y < 250; y += 20)
    tft.drawFastHLine(0, y, 240, 0x0180);
  tft.drawFastVLine(120, 22, 228, 0x0320);

  // cabecalhos de coluna
  tft.setTextColor(0x0320); tft.setTextSize(1);
  tft.setCursor(40, 26); tft.print(F("[ ACAO ]"));
  tft.setCursor(144, 26); tft.print(F("[ ALVO ]"));

  trmColuna(0);
  trmColuna(1);
  tft.drawFastVLine(120, 22, 228, 0x0320);
  trmPreview();

  tft.drawFastHLine(0, 294, 240, 0x0280);
  tft.setTextColor(0x0180); tft.setTextSize(1);
  tft.setCursor(18, 300); tft.print(F("ENTER=executar    ESC=sair"));
}

bool trmExecutar() {
  char buf[28];
  // EXEC + PONG
  if (trmV==0 && trmO==0) { rodarPong(); trmTudo(); return false; }
  // STATUS + SISTEMA
  if (trmV==1 && trmO==1) {
    tft.fillRect(0, 250, 240, 44, 0x0000);
    tft.drawFastHLine(0, 250, 240, 0x0320);
    tft.setTextColor(0x07FF); tft.setTextSize(1);
    tft.setCursor(8, 256); tft.print(F("> OK"));
    sprintf(buf, "Hora: %02d:%02d  STATUS: LIGADO", clockH, clockM);
    tft.setCursor(8, 268); tft.print(buf);
    delay(2500); trmPreview(); return false;
  }
  // PISCAR + LEDS
  if (trmV==2 && trmO==2) {
    for (int8_t j=0; j<5; j++) {
      digitalWrite(LED_HDD,HIGH); delay(80); digitalWrite(LED_POWER,LOW);  delay(80);
      digitalWrite(LED_HDD,LOW);  delay(80); digitalWrite(LED_POWER,HIGH); delay(80);
    }
    return false;
  }
  // TOCAR + MUSICA
  if (trmV==3 && trmO==3) { playEasterEggSound(); return false; }
  // DESLIGAR + SISTEMA
  if (trmV==4 && trmO==1) { desligar(); return true; }
  // REINICIAR + SISTEMA
  if (trmV==5 && trmO==1) { desligar(); delay(400); boot(); return true; }
  // VOLTAR + DESKTOP
  if (trmV==6 && trmO==4) { desktop(); return true; }

  // LIGAR + LEDS
  if (trmV==7 && trmO==2) { digitalWrite(LED_POWER,HIGH); digitalWrite(LED_HDD,HIGH); return false; }
  // APAGAR + LEDS
  if (trmV==8 && trmO==2) { digitalWrite(LED_POWER,LOW);  digitalWrite(LED_HDD,LOW);  return false; }
  // LIGAR + LED_PWR
  if (trmV==7 && trmO==5) { digitalWrite(LED_POWER,HIGH); return false; }
  // APAGAR + LED_PWR
  if (trmV==8 && trmO==5) { digitalWrite(LED_POWER,LOW);  return false; }
  // LIGAR + LED_HDD
  if (trmV==7 && trmO==6) { digitalWrite(LED_HDD,HIGH);   return false; }
  // APAGAR + LED_HDD
  if (trmV==8 && trmO==6) { digitalWrite(LED_HDD,LOW);    return false; }
  // PISCAR + LED_PWR
  if (trmV==2 && trmO==5) {
    for (int8_t j=0;j<6;j++){digitalWrite(LED_POWER,j%2);delay(120);}
    return false;
  }
  // PISCAR + LED_HDD
  if (trmV==2 && trmO==6) {
    for (int8_t j=0;j<6;j++){digitalWrite(LED_HDD,j%2);delay(120);}
    return false;
  }
  // TOCAR + BUZZER (escala do)
  if (trmV==3 && trmO==7) {
    const uint16_t SC[]={262,294,330,349,392,440,494,523};
    for(uint8_t i=0;i<8;i++){tone(BUZZER_PIN,SC[i],120);delay(150);}
    noTone(BUZZER_PIN); return false;
  }
  // LIGAR + SISTEMA
  if (trmV==7 && trmO==1) { if(!ligado){boot();return true;} return false; }
  // invalido
  tft.fillRect(0, 250, 240, 44, 0x0000);
  tft.drawFastHLine(0, 250, 240, 0xF800);
  tft.setTextColor(0xF800); tft.setTextSize(1);
  tft.setCursor(8, 256); tft.print(F("> ERRO: combinacao invalida"));
  tft.setTextSize(2);
  tft.setCursor(50, 268); tft.print(F("INVALIDO!"));
  delay(900); trmPreview();
  return false;
}

void rodarTerminal() {
  trmV = trmO = trmCol = 0;
  trmTudo();

  bool lpU=false,lpD=false,lpL=false,lpR=false,lpEn=false,lpEs=false;
  unsigned long lpNav = 0;

  while (true) {
    bool nU  = (digitalRead(BTN_UP)    == LOW);
    bool nD  = (digitalRead(BTN_DOWN)  == LOW);
    bool nL  = (digitalRead(BTN_LEFT)  == LOW);
    bool nR  = (digitalRead(BTN_RIGHT) == LOW);
    bool nEn = (digitalRead(BTN_ENTER) == LOW);
    bool nEs = (digitalRead(BTN_ESC)   == LOW);

    if (nEs && !lpEs) { delay(150); return; }

    if (millis() - lpNav >= 220) {
      bool mudou = false;
      if (nR && !lpR) {
        trmCol = 1; trmColuna(0); trmColuna(1); mudou = true;
      } else if (nL && !lpL) {
        trmCol = 0; trmColuna(0); trmColuna(1); mudou = true;
      } else if (nD && !lpD) {
        if (trmCol==0) trmV=(trmV+1)%TRM_VN; else trmO=(trmO+1)%TRM_ON;
        trmColuna(trmCol); trmPreview(); mudou = true;
      } else if (nU && !lpU) {
        if (trmCol==0) trmV=(trmV-1+TRM_VN)%TRM_VN; else trmO=(trmO-1+TRM_ON)%TRM_ON;
        trmColuna(trmCol); trmPreview(); mudou = true;
      }
      if (mudou) { tft.drawFastVLine(120,22,228,0xAD55); playMenuBeep(); lpNav=millis(); }
    }

    if (nEn && !lpEn) { playMenuBeep(); if (trmExecutar()) return; }

    lpU=nU; lpD=nD; lpL=nL; lpR=nR; lpEn=nEn; lpEs=nEs;
  }
}

// ---- Boot sequence ----
void boot() {
  ligado = true;
  digitalWrite(LED_POWER, HIGH);
  log(F("Iniciando boot sequence..."));
  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setCursor(8, 8);  tft.println(F("Dell Computer Corporation"));
  tft.setCursor(8, 20); tft.println(F("BIOS Version A08"));
  tft.setCursor(8, 32); tft.println(F("CPU : Intel Pentium III 500MHz"));
  tft.setCursor(8, 44); tft.println(F("RAM : 524288K OK"));
  tft.setCursor(8, 56); tft.println(F("HDD : ST340810A"));
  log(F("POST: CPU OK | RAM OK | HDD OK"));
  delay(1500);

  log(F("Carregando Windows XP..."));
  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.setCursor(30, 60); tft.print(F("Windows"));
  tft.setTextColor(0x07FF);
  tft.setTextSize(1);
  tft.setCursor(30, 80); tft.print(F("XP  Professional"));
  tft.drawRoundRect(25, 140, 190, 14, 3, 0x8410);
  log(F("Carregando drivers..."));
  for (int v = 0; v < 3; v++) {
    for (int x = 0; x <= 180; x += 5) {
      tft.fillRect(27, 142, 186, 10, 0x0000);
      int s = max(0, x - 50);
      tft.fillRect(27 + s, 142, min(50, x - s), 10, 0x001F);
      digitalWrite(LED_HDD, (x / 5) % 2 == 0 ? HIGH : LOW);
      delay(18);
    }
    digitalWrite(LED_HDD, LOW);
    delay(30);
  }
  tft.fillRect(27, 142, 186, 10, 0x0000);
  digitalWrite(LED_HDD, LOW);
  delay(300);
  desktop();
  playSuccessSound();
  log(F("Boot completo! Digite 'ajuda'."));
}

// ---- Desligar ----
void desligar() {
  log(F("Desligando..."));
  playShutdownSound();
  ligado = false;
  digitalWrite(LED_POWER, LOW);
  digitalWrite(LED_HDD,   LOW);
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(0xFFFF); delay(80);
    tft.fillScreen(0x0000); delay(80);
  }
  tft.fillScreen(0x0000);
  tft.setTextColor(0x8410);
  tft.setTextSize(1);
  tft.setCursor(60, 115); tft.print(F("Desligado"));
  log(F("Display desligado."));
}
