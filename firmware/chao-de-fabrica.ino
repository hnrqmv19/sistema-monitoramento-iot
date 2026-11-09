/*
  =============================================================================
  PROJETO IOT INDUSTRIAL - CHÃO DE FÁBRICA (EMISSOR)
  Ficheiro: firmware/emissor/emissor.ino
  Hardware: ESP32 WROOM / ESP32-DevKit
  Sensores: DHT22, HC-SR04 (Ultrassónico), LDR (Luminosidade)
  Atuadores: LED Verde (Nível OK >= 20%), LED Vermelho (Alerta < 20%)
  Comunicação: ESP-NOW (Pacote de 20 Bytes)
  =============================================================================
*/

#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

// ===================== CONFIGURAÇÃO DE PINOS =====================
#define DHTPIN          4
#define DHTTYPE         DHT22

#define TRIG_PIN        5
#define ECHO_PIN        18

#define LDR_PIN         34

// LEDs de Status do Tanque no Chão de Fábrica
#define PIN_LED_VERDE   13   // Aceso quando nível >= 20% (Faixa Segura)
#define PIN_LED_VERMELHO 15  // Aceso quando nível < 20% (Alerta de Nível Baixo)

// Altura total do reservatório de tinta em cm
const float ALTURA_MAXIMA_TANQUE = 100.0; 

// Endereço MAC do ESP32-S3 Receptor (Substitua pelo MAC do esp de monitoramento)
uint8_t macReceptor[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

// ===================== ESTRUTURA DE DADOS (20 BYTES) =====================
typedef struct struct_message {
    float nivel;             // 4 bytes
    float temperatura;       // 4 bytes
    float umidade;           // 4 bytes
    int luminosidade;        // 4 bytes
    unsigned long timestamp; // 4 bytes
} struct_message;

struct_message dadosEnvio;
esp_now_peer_info_t peerInfo;
DHT dht(DHTPIN, DHTTYPE);

// ===================== CALLBACK DE ENVIO (COMPATÍVEL V2.X E V3.X) =====================
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void aoEnviar(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("[ESP-NOW] Status do Envio: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sucesso (ACK Recebido)" : "Falha na Entrega");
}
#else
void aoEnviar(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("[ESP-NOW] Status do Envio: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sucesso (ACK Recebido)" : "Falha na Entrega");
}
#endif

// ===================== LEITURA DO ULTRASSÔNICO =====================
float lerNivelUltrassonico() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout 30ms
  if (duracao == 0) return 0.0;

  float distancia = (duracao * 0.0343) / 2.0;

  if (distancia >= ALTURA_MAXIMA_TANQUE) return 0.0;
  float nivelPercentual = ((ALTURA_MAXIMA_TANQUE - distancia) / ALTURA_MAXIMA_TANQUE) * 100.0;
  
  if (nivelPercentual > 100.0) nivelPercentual = 100.0;
  if (nivelPercentual < 0.0) nivelPercentual = 0.0;

  return nivelPercentual;
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configuração dos Pinos dos Sensores
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);

  // Configuração dos LEDs
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  digitalWrite(PIN_LED_VERDE, LOW);
  digitalWrite(PIN_LED_VERMELHO, LOW);

  dht.begin();

  WiFi.mode(WIFI_STA);
  Serial.print("[INFO] MAC do ESP32 Emissor: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERRO] Falha ao inicializar ESP-NOW");
    while (true) delay(1000);
  }

  esp_now_register_send_cb(aoEnviar);

  // Cadastrar o ESP32 Receptor
  memcpy(peerInfo.peer_addr, macReceptor, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ERRO] Falha ao registrar Peer Receptor");
    while (true) delay(1000);
  }

  Serial.println("[INFO] Emissor pronto. Iniciando leituras...\n");
}

// ===================== LOOP PRINCIPAL =====================
void loop() {
  // 1. Ler Nível do Tanque
  dadosEnvio.nivel = lerNivelUltrassonico();

  // 2. Lógica dos LEDs de Status no Chão de Fábrica
  if (dadosEnvio.nivel < 20.0) {
    // Nível Baixo (< 20%) -> Acende Vermelho
    digitalWrite(PIN_LED_VERDE, LOW);
    digitalWrite(PIN_LED_VERMELHO, HIGH);
  } else {
    // Operação Normal (>= 20%) -> Acende Verde
    digitalWrite(PIN_LED_VERDE, HIGH);
    digitalWrite(PIN_LED_VERMELHO, LOW);
  }

  // 3. Ler DHT22 (Temperatura e Umidade)
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();
  dadosEnvio.temperatura = isnan(temp) ? 0.0 : temp;
  dadosEnvio.umidade = isnan(umid) ? 0.0 : umid;

  // 4. Ler LDR (Luminosidade)
  dadosEnvio.luminosidade = analogRead(LDR_PIN);

  // 5. Timestamp local
  dadosEnvio.timestamp = millis();

  Serial.printf("[TX 20 Bytes] Nível: %.1f%% | Temp: %.1f°C | Umid: %.1f%% | Lux: %d\n",
                dadosEnvio.nivel, dadosEnvio.temperatura, 
                dadosEnvio.umidade, dadosEnvio.luminosidade);

  // Dispara pacote via ESP-NOW
  esp_err_t result = esp_now_send(macReceptor, (uint8_t *) &dadosEnvio, sizeof(dadosEnvio));

  if (result != ESP_OK) {
    Serial.println("[ERRO] Falha ao executar esp_now_send()");
  }

  delay(2000); // Ciclo de envio a cada 2 segundos
}