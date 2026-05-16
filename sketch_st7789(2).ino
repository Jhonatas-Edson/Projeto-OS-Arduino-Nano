#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

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

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

bool ligado = false;

// ---- Relogio ----
uint8_t clockH = 8, clockM = 0;
unsigned long clockBase = 0;

void log(const __FlashStringHelper* msg) {
  Serial.print(F("[ArduinoOS] "));
  Serial.println(msg);
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
void icone(int x, int y, uint16_t cor, const char* nome) {
  tft.fillRoundRect(x, y, 34, 30, 4, cor);
  tft.drawRoundRect(x, y, 34, 30, 4, 0xFFFF);
  tft.fillRect(x + 3, y + 2, 28, 3, 0xFFFF); // realce no topo
  int tw = strlen(nome) * 6;
  tft.setTextColor(0x0000); // sombra
  tft.setTextSize(1);
  tft.setCursor(x + (34 - tw) / 2 + 1, y + 35);
  tft.print(nome);
  tft.setTextColor(0xFFFF); // texto
  tft.setCursor(x + (34 - tw) / 2, y + 34);
  tft.print(nome);
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

  wallpaper();

  // Icones (cor azul escuro, laranja pasta, cinza lixo)
  icone(5,   8, 0x32B4, "My PC");
  icone(5,  72, 0xFCC0, "Docs");
  icone(5, 136, 0x7BEF, "Lixo");

  desenharTaskbar();
  log(F("Desktop pronto."));
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
  log(F("Inicializando sistema..."));
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
  log(F("Display OK"));
  log(F("Digite 'ajuda' para ver os comandos."));
  boot();
}

// ---- Loop principal ----
void loop() {
  atualizarRelogio();

  if (digitalRead(BTN_PIN) == LOW) {
    delay(50);
    if (digitalRead(BTN_PIN) == LOW) {
      alternar();
      while (digitalRead(BTN_PIN) == LOW);
    }
  }

  if (digitalRead(BTN_ESC) == LOW) {
    delay(50);
    if (digitalRead(BTN_ESC) == LOW) {
      while (digitalRead(BTN_ESC) == LOW);
      desktop();
      return;
    }
  }

  char buf[16];
  if (!lerComando(buf, 16)) return;

  Serial.print(F("[ArduinoOS] Comando: "));
  Serial.println(buf);

  if (strcmp(buf, "ligar") == 0) {
    if (ligado) log(F("Tela ja ligada."));
    else boot();
  } else if (strcmp(buf, "desligar") == 0) {
    if (!ligado) log(F("Tela ja desligada."));
    else desligar();
  } else if (strcmp(buf, "status") == 0) {
    Serial.print(F("[ArduinoOS] Estado: "));
    Serial.println(ligado ? F("LIGADO") : F("DESLIGADO"));
  } else if (strcmp(buf, "reiniciar") == 0) {
    log(F("Reiniciando..."));
    desligar(); delay(500); boot();
  } else if (strcmp(buf, "ajuda") == 0) {
    Serial.println(F("=========================="));
    Serial.println(F("  ligar      - Liga o display"));
    Serial.println(F("  desligar   - Desliga o display"));
    Serial.println(F("  reiniciar  - Reinicia o boot"));
    Serial.println(F("  status     - Estado atual"));
    Serial.println(F("  ajuda      - Este menu"));
    Serial.println(F("=========================="));
  } else {
    log(F("Comando desconhecido. Digite 'ajuda'."));
  }
}

// ---- Boot sequence ----
void boot() {
  ligado = true;
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
      delay(18);
    }
  }
  tft.fillRect(27, 142, 186, 10, 0x0000);
  delay(300);
  desktop();
  log(F("Boot completo! Digite 'ajuda'."));
}

// ---- Desligar ----
void desligar() {
  log(F("Desligando..."));
  ligado = false;
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
