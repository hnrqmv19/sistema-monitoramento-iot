/*
  =============================================================================
  PROJETO IOT INDUSTRIAL - MONITORAMENTO (RECEPTOR)
  Ficheiro: firmware/receptor/receptor.ino
  Hardware: ESP32-S3
  Visualização: Matriz de LEDs MAX7219 (DIN: 11, CLK: 12, CS: 10)
  Comunicação: ESP-NOW (Recepção) + Wi-Fi HTTP POST (API Flask)
  =============================================================================
*/

#include <WiFi.h>
#include <esp_now.h>
#include <HTTPClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

// ===================== CONFIGURAÇÃO DE REDE E API =====================
const char* WIFI_SSID = "NOME_DA_SUA_REDE_WIFI";   // Insira o nome do seu Wi-Fi
const char* WIFI_PASS = "SENHA_DO_SEU_WIFI";       // Insira a senha do seu Wi-Fi

// Ajuste a IP_DA_SUA_MAQUINA para o IP do seu computador na rede Wi-Fi
const char* API_URL   = "http://10.0.2.15:5000/api/telemetria";

// ===================== HARDWARE MAX7219 =====================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW 
#define MAX_DEVICES   1                      
#define DATA_PIN      11                     // DIN
#define CLK_PIN       12                     // CLK
#define CS_PIN        10                     // CS

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// ===================== CONFIGURAÇÕES DO SISTEMA =====================
#define TIMEOUT_RX_MS   5000 
#define TEMPO_TELA_MS   2000 

// ===================== ESTRUTURA DE DADOS (20 BYTES) =====================
typedef struct struct_message {
    float nivel;
    float temperatura;
    float umidade;
    int luminosidade;
    unsigned long timestamp;
} struct_message;

struct_message dadosPacote;
uint8_t macOrigem[6];

volatile bool novoPacote = false;
volatile bool tamanhoInvalido = false;
volatile int  tamanhoRecebido = 0;
volatile unsigned long lastRxMillis = 0;

bool emTimeout = true;
int telaAtual = 0;
unsigned long lastTelaMillis = 0;

char textoExibicao[30] = "INICIANDO";

// ===================== ENVIO HTTP POST PARA API FLASK =====================
void enviarParaAPI(struct_message dados) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2000); 

    String jsonPayload;
    jsonPayload.reserve(150);
    jsonPayload = "{\"nivel\":" + String(dados.nivel, 1) +
                  ",\"temperatura\":" + String(dados.temperatura, 1) +
                  ",\"umidade\":" + String(dados.umidade, 1) +
                  ",\"luminosidade\":" + String(dados.luminosidade) +
                  ",\"timestamp\":" + String(dados.timestamp) + "}";

    int httpCode = http.POST(jsonPayload);

    if (httpCode > 0) {
      Serial.printf("[HTTP POST] Status: %d\n", httpCode);
    } else {
      Serial.printf("[HTTP ERRO] %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("[HTTP ERRO] Wi-Fi desconectado!");
  }
}

// ===================== TRATAMENTO ESP-NOW =====================
void tratarPacote(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(struct_message)) {
    tamanhoRecebido = len;
    tamanhoInvalido = true;
    return;
  }

  memcpy(&dadosPacote, data, sizeof(struct_message));
  memcpy(macOrigem, mac, 6);
  lastRxMillis = millis();
  novoPacote = true;
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void aoReceber(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  tratarPacote(info->src_addr, data, len);
}
#else
void aoReceber(const uint8_t *mac, const uint8_t *data, int len) {
  tratarPacote(mac, data, len);
}
#endif

// ===================== MATRIZ MAX7219 =====================
void prepararNovaTela() {
  if (emTimeout) {
    snprintf(textoExibicao, sizeof(textoExibicao), "SEM DADOS");
  } else {
    switch (telaAtual) {
      case 0:
        snprintf(textoExibicao, sizeof(textoExibicao), "NVL %.0f%%", dadosPacote.nivel);
        break;
      case 1:
        snprintf(textoExibicao, sizeof(textoExibicao), "TMP %.1fC", dadosPacote.temperatura);
        break;
      case 2:
        snprintf(textoExibicao, sizeof(textoExibicao), "UMD %.1f%%", dadosPacote.umidade);
        break;
      case 3:
        snprintf(textoExibicao, sizeof(textoExibicao), "LUX %d", dadosPacote.luminosidade);
        break;
    }
  }

  #if MAX_DEVICES == 1
    P.displayText(textoExibicao, PA_LEFT, 60, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  #else
    P.displayText(textoExibicao, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  #endif
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  P.begin();
  P.setIntensity(1);
  P.displayText("OK", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  while (!P.displayAnimate()) { delay(10); }
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[INFO] Conectando ao Wi-Fi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[INFO] Wi-Fi Conectado!");
  Serial.print("[INFO] MAC ESP32-S3 Receptor: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERRO] Erro ao iniciar ESP-NOW");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(aoReceber);
  Serial.println("[INFO] Aguardando pacotes do Emissor...\n");
}

// ===================== LOOP PRINCIPAL =====================
void loop() {
  if (P.displayAnimate()) {
    #if MAX_DEVICES == 1
      P.displayReset();
    #endif
  }

  if (tamanhoInvalido) {
    tamanhoInvalido = false;
    Serial.printf("[AVISO] Pacote de tamanho incorreto: %d bytes.\n", tamanhoRecebido);
  }

  if (novoPacote) {
    novoPacote = false;
    
    bool estavaEmTimeout = emTimeout;
    emTimeout = false;

    if (estavaEmTimeout) {
      telaAtual = 0;
      lastTelaMillis = millis();
      prepararNovaTela();
    }

    Serial.printf("[RX] Nível: %.0f%% | Temp: %.1f°C | Umid: %.1f%% | Lux: %d\n",
                  dadosPacote.nivel, dadosPacote.temperatura,
                  dadosPacote.umidade, dadosPacote.luminosidade);

    enviarParaAPI(dadosPacote);
  }

  // Verifica timeout de recepção (5s)
  if (!emTimeout && (millis() - lastRxMillis > TIMEOUT_RX_MS)) {
    emTimeout = true;
    Serial.println("[AVISO] Timeout de comunicação ativado!");
    prepararNovaTela();
  }

  // Alterna as telas a cada 2 segundos
  if (!emTimeout && (millis() - lastTelaMillis >= TEMPO_TELA_MS)) {
    lastTelaMillis = millis();
    telaAtual = (telaAtual + 1) % 4;
    prepararNovaTela();
  }
}