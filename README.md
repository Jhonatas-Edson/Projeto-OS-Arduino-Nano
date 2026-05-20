# Arduino OS — Mini Retro Dell XP

Simulação de um mini desktop estilo Windows XP rodando num Arduino Nano com display ILI9341.  
Simulado via extensão **Wokwi** no VS Code.

---

## Equipe

| Integrante | Contribuição |
|---|---|
| **Jhonatas Edson** | Criação do OS, soldagem e montagem do hardware |
| **Vinicius** | Sistema de sons completo |
| **Hiru Akiama** | Jogo Pong |

---

## Hardware

| Componente | Descrição |
|---|---|
| Arduino Nano | Microcontrolador principal |
| ILI9341 (240×320) | Display TFT colorido |
| 4× Pushbutton azul | Navegação (↑ ↓ ← →) |
| 1× Pushbutton verde | Liga/desliga (toggle) |
| 1× Pushbutton verde | Enter |
| 1× Pushbutton vermelho | ESC / voltar ao desktop |
| Buzzer passivo | Sons do sistema |
| LED verde | Power indicator |
| LED vermelho | HDD activity |

### Pinagem

| Pino | Função |
|---|---|
| 2 | BTN_TOGGLE (liga/desliga) |
| 3 | BTN_UP |
| 4 | BTN_DOWN |
| 5 | BUZZER |
| 6 | BTN_LEFT |
| 7 | BTN_RIGHT |
| 8 | TFT_RST |
| 9 | TFT_DC |
| 10 | TFT_CS |
| 11 | MOSI |
| 12 | MISO |
| 13 | SCK |
| A0 | BTN_ENTER |
| A1 | BTN_ESC |
| A2 | LED_POWER |
| A3 | LED_HDD |

---

## Funcionalidades

### Boot sequence
- Tela POST estilo BIOS Dell (CPU, RAM, HDD)
- Barra de progresso animada estilo Windows XP
- LED de HDD piscando durante o carregamento
- Som de inicialização ao completar o boot

### Desktop
- Wallpaper com gradiente de céu e colinas (estilo XP)
- 3 ícones na lateral esquerda: **My PC**, **Term**, **Pong**
- Taskbar com botão Start e relógio (atualiza a cada minuto)
- Ícone selecionado: fundo ciano + borda dupla amarela

### Ícones

| Ícone | Aparência | Função |
|---|---|---|
| **My PC** | Pasta amarela com aba e linhas de arquivos | Exibe créditos da equipe |
| **Term** | Fundo preto com `>_` verde | Abre o Terminal |
| **Pong** | Palitos e bola estilo arcade | Abre o jogo Pong |

### Navegação no desktop
- **BTN_DOWN** → seleciona próximo ícone (wrap circular)
- **BTN_UP** → seleciona ícone anterior (wrap circular)
- **BTN_ENTER** → abre o app selecionado
- Beep ao navegar

### My PC — Créditos
Exibe os integrantes do projeto com suas contribuições.  
Pressione **ESC** para fechar e voltar ao desktop.

### Terminal
Interface estilo terminal real: fundo preto, texto verde, inspirado no jogo *Sons of the Forest*.  
O usuário **monta um comando** escolhendo AÇÃO + ALVO nas duas colunas com o scroll-wheel.

**Controles dentro do Terminal:**
- `UP` / `DOWN` — rola os itens da coluna ativa
- `LEFT` / `RIGHT` — alterna entre coluna AÇÃO e ALVO
- `ENTER` — executa o comando montado
- `ESC` — sai e volta ao desktop

**Comandos disponíveis:**

| AÇÃO | ALVO | Hardware | O que faz |
|---|---|---|---|
| `EXEC` | `PONG` | — | Abre o jogo Pong |
| `STATUS` | `SISTEMA` | — | Exibe hora atual e status |
| `PISCAR` | `LEDS` | LED verde + vermelho | Pisca os dois LEDs alternados |
| `PISCAR` | `LED_PWR` | LED verde | Pisca só o LED de power |
| `PISCAR` | `LED_HDD` | LED vermelho | Pisca só o LED de HDD |
| `TOCAR` | `MUSICA` | Buzzer | Toca o Rick Astley (easter egg) |
| `TOCAR` | `BUZZER` | Buzzer | Toca escala de Dó maior |
| `LIGAR` | `LEDS` | LED verde + vermelho | Liga os dois LEDs |
| `LIGAR` | `LED_PWR` | LED verde | Liga o LED de power |
| `LIGAR` | `LED_HDD` | LED vermelho | Liga o LED de HDD |
| `LIGAR` | `SISTEMA` | Display | Liga o sistema (se desligado) |
| `APAGAR` | `LEDS` | LED verde + vermelho | Apaga os dois LEDs |
| `APAGAR` | `LED_PWR` | LED verde | Apaga o LED de power |
| `APAGAR` | `LED_HDD` | LED vermelho | Apaga o LED de HDD |
| `DESLIGAR` | `SISTEMA` | Display | Desliga o display |
| `REINICIAR` | `SISTEMA` | Display | Reinicia com boot completo |
| `VOLTAR` | `DESKTOP` | — | Volta para o desktop |

Qualquer outra combinação exibe `ERRO: combinacao invalida` em vermelho.

### Pong
Jogo de Pong para dois jogadores na tela TFT.

**Controles:**
- Jogador 1: `BTN_UP` / `BTN_DOWN`
- Jogador 2: `BTN_LEFT` / `BTN_RIGHT`
- `BTN_ENTER` — iniciar / revanche
- `BTN_ESC` — sair e voltar ao desktop

Primeiro a 5 pontos vence. A bola acelera a cada rebatida (+8% por toque, máximo de 5.5).

### Sons

| Evento | Som |
|---|---|
| Navegação / beep | Beep curto 1800 Hz |
| Boot completo | Melodia crescente 3 notas |
| Erro / alerta | Melodia descendente 3 notas |
| Desligamento | Melodia descendente 7 notas |
| Easter egg (`TOCAR MUSICA`) | Chorus do Rick Astley |
| Escala (`TOCAR BUZZER`) | Dó maior — 8 notas |
| Bola na parede (Pong) | Beep grave 280 Hz |
| Rebatida no palito (Pong) | Beep médio 660 Hz |
| Ponto marcado (Pong) | Sequência 880 Hz → 1100 Hz |

### Outros controles
- **BTN_TOGGLE** → liga/desliga o display
- **BTN_ESC** → volta ao desktop de qualquer estado
- Comandos via Serial Monitor (9600 baud): `ligar`, `desligar`, `reiniciar`, `status`, `easter`

---

## Como compilar

O Wokwi usa um binário pré-compilado — é necessário recompilar manualmente após cada alteração no `.ino`.

```bash
# Copiar para pasta temporária com nome correto
cp "sketch_st7789(2).ino" /tmp/sketch_st7789/sketch_st7789.ino

# Compilar
arduino-cli compile \
  --fqbn arduino:avr:nano \
  --output-dir build/arduino.avr.nano \
  /tmp/sketch_st7789
```

O Wokwi lê automaticamente `build/arduino.avr.nano/sketch_st7789.ino.hex` (configurado em `wokwi.toml`).

> **Atenção:** o sketch ocupa ~99% da flash do Nano (30.4 KB / 30.7 KB). Qualquer nova funcionalidade exige remoção equivalente de código.

---

## Estrutura do código

```
setup()
  └── boot()
        └── desktop()

loop()
  ├── atualizarRelogio()
  ├── leitura de botões (detecção de borda + cooldown 250ms)
  │     ├── BTN_TOGGLE → alternar()
  │     ├── BTN_ESC    → desktop()
  │     ├── BTN_DOWN   → navegarApp(+1)
  │     ├── BTN_UP     → navegarApp(-1)
  │     └── BTN_ENTER  → abrirApp(appSel)
  └── lerComando()  ← comandos Serial

abrirApp(idx)
  ├── idx 0 → rodarMyPC()
  ├── idx 1 → rodarTerminal()
  └── idx 2 → rodarPong()
```

### Arrays de apps
```cpp
#define APP_COUNT 3
const int      APP_X[]   = {5,      5,      5     };
const int      APP_Y[]   = {8,      72,     136   };
const uint16_t APP_COR[] = {0xFEA0, 0x0000, 0x000F};
const char     APP_NOMES[][6] = {"My PC", "Term", "Pong"};
```

---

## Notas de desenvolvimento

- **Debounce no Wokwi**: o simulador registra cliques como pulsos duplos — solução usada foi detecção de borda (HIGH→LOW) com cooldown de 250 ms entre navegações, em vez do padrão `delay(50) + segunda leitura` (que falha em simulação).
- **Conflito de buffer no VS Code**: o Wokwi VS Code extension não recompila automaticamente. Sempre compilar via `arduino-cli` e reiniciar a simulação após mudanças.
- **Cor de seleção**: `cor | 0x1860` em RGB565 é imperceptível — troca completa para ciano (0x07FF) com borda dupla amarela (0xFFE0).
- **Flash quase cheio**: o sketch usa 99% da flash. Strings longas desnecessárias (ex: help serial detalhado) foram encurtadas para abrir espaço. Usar `F()` em todas as strings literais é obrigatório para não estourar a SRAM.
- **Resolução alvo**: o produto final usará display 240×240. O código atual foi desenvolvido para 240×320 (ILI9341 no Wokwi). Ao migrar para o hardware físico, todas as coordenadas Y do terminal, pong e desktop precisarão ser ajustadas.
