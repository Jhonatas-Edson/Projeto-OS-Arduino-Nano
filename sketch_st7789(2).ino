#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

#define TFT_CS    10
#define TFT_DC     9
#define TFT_RST    8
#define BTN_PIN    2
#define BUZZER     5
#define BTN_UP     3
#define BTN_DOWN   4
#define BTN_LEFT   6
#define BTN_RIGHT  7
#define BTN_ENTER  A0
#define BTN_ESC    A1

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

bool ligado = false;

// ---- Notas ----
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_AS4 466
#define NOTE_F5  698
#define REST     0

void log(const __FlashStringHelper* msg) {
  Serial.print(F("[ArduinoOS] "));
  Serial.println(msg);
}

// ---- Musicas ----
void musicaXP() {
  const int notas[]    = {NOTE_E4,NOTE_G4,NOTE_D5,NOTE_C5,NOTE_E5,NOTE_D5,NOTE_G4,NOTE_B4,NOTE_C5,NOTE_A4};
  const int duracoes[] = {150,150,300,150,300,150,150,200,400,600};
  for (int i = 0; i < 10; i++) {
    tone(BUZZER, notas[i], duracoes[i]);
    delay(duracoes[i] * 1.2);
    noTone(BUZZER);
  }
}

void bipeDesligar() {
  tone(BUZZER, NOTE_C5, 100); delay(120);
  tone(BUZZER, NOTE_G4, 200); delay(250);
  noTone(BUZZER);
}

void bipeMorte() {
  tone(BUZZER, NOTE_A4, 200); delay(220);
  tone(BUZZER, NOTE_E4, 400); delay(450);
  noTone(BUZZER);
}

void superMario() {
  const int notas[]    = {NOTE_E5,NOTE_E5,REST,NOTE_E5,REST,NOTE_C5,NOTE_E5,REST,NOTE_G5,REST,REST,REST,NOTE_G4,REST,REST,REST,NOTE_C5,REST,NOTE_G4,REST,NOTE_E4,REST,NOTE_A4,NOTE_B4,NOTE_AS4,NOTE_A4,NOTE_G4,NOTE_E5,NOTE_G5,NOTE_A5,NOTE_F5,NOTE_G5,REST,NOTE_E5,NOTE_C5,NOTE_D5,NOTE_B4};
  const int duracoes[] = {125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,83,83,83,125,125,125,125,125,125,125,125};
  log(F("Tocando Super Mario Bros..."));
  for (int i = 0; i < 37; i++) {
    if (notas[i] == REST) noTone(BUZZER);
    else tone(BUZZER, notas[i], duracoes[i]);
    delay(duracoes[i] * 1.3);
    noTone(BUZZER);
  }
  log(F("Fim da musica!"));
}

// ---- Snake ----
#define CELL    10
#define COLS    23
#define ROWS    21
#define MAXLEN  60
#define OX      8
#define OY      10

int8_t sx[MAXLEN], sy[MAXLEN];
int8_t slen;
int8_t dir;   // 0=direita 1=baixo 2=esquerda 3=cima
int8_t fx, fy;
int score;
bool jogoAtivo;

void snakeDraw(int8_t x, int8_t y, uint16_t cor) {
  tft.fillRect(OX + x * CELL, OY + y * CELL, CELL - 1, CELL - 1, cor);
}

void snakeComida() {
  fx = random(0, COLS);
  fy = random(0, ROWS);
  snakeDraw(fx, fy, 0xF800);
}

void snakeInicio() {
  tft.fillScreen(0x0000);
  tft.drawRect(OX - 1, OY - 1, COLS * CELL + 2, ROWS * CELL + 2, 0x8410);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setCursor(OX, 1);
  tft.print(F("SNAKE  Pontos: "));
  tft.print(0);

  slen = 3;
  dir  = 0;
  score = 0;
  sx[0] = 5; sy[0] = 5;
  sx[1] = 4; sy[1] = 5;
  sx[2] = 3; sy[2] = 5;
  for (int i = 0; i < slen; i++) snakeDraw(sx[i], sy[i], 0x07E0);
  randomSeed(analogRead(0));
  snakeComida();
  jogoAtivo = true;
  log(F("Snake iniciado! Use: cima/baixo/esquerda/direita"));
  log(F("Digite 'sair' para voltar ao desktop."));
}

void snakePasso() {
  // Apaga cauda
  snakeDraw(sx[slen-1], sy[slen-1], 0x0000);
  // Move corpo
  for (int i = slen - 1; i > 0; i--) {
    sx[i] = sx[i-1];
    sy[i] = sy[i-1];
  }
  // Move cabeca
  if (dir == 0) sx[0]++;
  else if (dir == 1) sy[0]++;
  else if (dir == 2) sx[0]--;
  else sy[0]--;

  // Colisao parede
  if (sx[0] < 0 || sx[0] >= COLS || sy[0] < 0 || sy[0] >= ROWS) {
    snakeFim();
    return;
  }
  // Colisao corpo
  for (int i = 1; i < slen; i++) {
    if (sx[0] == sx[i] && sy[0] == sy[i]) {
      snakeFim();
      return;
    }
  }
  // Comeu comida
  if (sx[0] == fx && sy[0] == fy) {
    score++;
    if (slen < MAXLEN) slen++;
    // Atualiza placar
    tft.fillRect(OX + 56, 1, 40, 8, 0x0000);
    tft.setCursor(OX + 56, 1);
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.print(score);
    tone(BUZZER, NOTE_C5, 50); delay(60); noTone(BUZZER);
    snakeComida();
  }
  snakeDraw(sx[0], sy[0], 0x07E0);
}

void snakeFim() {
  jogoAtivo = false;
  bipeMorte();
  tft.fillScreen(0x0000);
  tft.setTextColor(0xF800);
  tft.setTextSize(2);
  tft.setCursor(40, 80);  tft.print(F("GAME"));
  tft.setCursor(40, 100); tft.print(F("OVER"));
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setCursor(50, 130); tft.print(F("Pontos: "));
  tft.print(score);
  tft.setCursor(20, 150); tft.print(F("'snake' p/ jogar dnv"));
  tft.setCursor(20, 162); tft.print(F("'sair' p/ desktop"));
  Serial.print(F("[ArduinoOS] Game Over! Pontos: "));
  Serial.println(score);
  log(F("Digite 'snake' para jogar de novo ou 'sair'."));
}

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
  pinMode(BUZZER, OUTPUT);
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
  log(F("Display ST7789 OK"));
  log(F("Buzzer OK"));
  log(F("Digite 'ajuda' para ver os comandos."));
  boot();
}

// ---- Loop principal ----
unsigned long ultimoPasso = 0;
#define VELOCIDADE 250

void loop() {
  // Botao fisico (toggle liga/desliga)
  if (digitalRead(BTN_PIN) == LOW) {
    delay(50);
    if (digitalRead(BTN_PIN) == LOW) {
      log(F("Botao pressionado!"));
      if (!jogoAtivo) alternar();
      while (digitalRead(BTN_PIN) == LOW);
    }
  }

  // Snake rodando
  if (jogoAtivo) {
    // Botoes fisicos de navegacao
    if (digitalRead(BTN_UP)    == LOW && dir != 1) { dir = 3; delay(150); }
    if (digitalRead(BTN_DOWN)  == LOW && dir != 3) { dir = 1; delay(150); }
    if (digitalRead(BTN_LEFT)  == LOW && dir != 0) { dir = 2; delay(150); }
    if (digitalRead(BTN_RIGHT) == LOW && dir != 2) { dir = 0; delay(150); }
    if (digitalRead(BTN_ESC)   == LOW) {
      delay(50);
      if (digitalRead(BTN_ESC) == LOW) {
        jogoAtivo = false;
        desktop();
        while (digitalRead(BTN_ESC) == LOW);
        return;
      }
    }

    // Comandos serial (ainda funciona)
    char buf[16];
    if (lerComando(buf, 16)) {
      if      (strcmp(buf, "cima")     == 0 && dir != 1) dir = 3;
      else if (strcmp(buf, "baixo")    == 0 && dir != 3) dir = 1;
      else if (strcmp(buf, "esquerda") == 0 && dir != 0) dir = 2;
      else if (strcmp(buf, "direita")  == 0 && dir != 2) dir = 0;
      else if (strcmp(buf, "sair")     == 0) { jogoAtivo = false; boot(); return; }
    }
    if (millis() - ultimoPasso > VELOCIDADE) {
      ultimoPasso = millis();
      snakePasso();
    }
    return;
  }

  // Desktop: ENTER inicia Snake, ESC desliga
  if (digitalRead(BTN_ENTER) == LOW) {
    delay(50);
    if (digitalRead(BTN_ENTER) == LOW) {
      while (digitalRead(BTN_ENTER) == LOW);
      snakeInicio();
      return;
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

  // Comandos normais
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
  } else if (strcmp(buf, "mario") == 0) {
    superMario();
  } else if (strcmp(buf, "snake") == 0) {
    snakeInicio();
  } else if (strcmp(buf, "ajuda") == 0) {
    Serial.println(F("=========================="));
    Serial.println(F("  ligar      - Liga o display"));
    Serial.println(F("  desligar   - Desliga o display"));
    Serial.println(F("  reiniciar  - Reinicia o boot"));
    Serial.println(F("  status     - Estado atual"));
    Serial.println(F("  mario      - Super Mario Bros"));
    Serial.println(F("  snake      - Jogo Snake"));
    Serial.println(F("  ajuda      - Este menu"));
    Serial.println(F("=========================="));
    Serial.println(F("  No Snake:"));
    Serial.println(F("  cima/baixo/esquerda/direita"));
    Serial.println(F("  sair       - Volta ao desktop"));
    Serial.println(F("=========================="));
  } else {
    log(F("Comando desconhecido. Digite 'ajuda'."));
  }
}

// ---- Desktop ----
void alternar() { if (ligado) desligar(); else boot(); }

void desktop() {
  ligado = true;
  jogoAtivo = false;
  tft.fillScreen(0x0451);
  tft.fillRect(0, 222, 240, 18, 0x0007);
  tft.fillRoundRect(2, 223, 44, 15, 3, 0x0320);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setCursor(8, 227);   tft.print(F("start"));
  tft.setCursor(195, 227); tft.print(F("6:21"));
  icone(14, 18,  "My PC");
  icone(14, 78,  "Recycle");
  icone(14, 138, "Snake!");
  log(F("Desktop restaurado."));
}

void boot() {
  ligado = true;
  jogoAtivo = false;
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

  log(F("Inicializando desktop..."));
  tft.fillScreen(0x0451);
  tft.fillRect(0, 222, 240, 18, 0x0007);
  tft.fillRoundRect(2, 223, 44, 15, 3, 0x0320);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setCursor(8, 227);   tft.print(F("start"));
  tft.setCursor(195, 227); tft.print(F("6:21"));
  icone(14, 18,  "My PC");
  icone(14, 78,  "Recycle");
  icone(14, 138, "Snake!");

  log(F("Tocando musica de boot..."));
  musicaXP();
  log(F("Boot completo! Digite 'ajuda'."));
}

void desligar() {
  log(F("Desligando..."));
  ligado = false;
  jogoAtivo = false;
  bipeDesligar();
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

void icone(int x, int y, const char* nome) {
  tft.fillRect(x, y, 28, 26, 0x2945);
  tft.drawRect(x, y, 28, 26, 0xFFFF);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(1);
  tft.setCursor(x, y + 30);
  tft.print(nome);
}
