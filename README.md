# 🏭 Sistema Industrial de Monitorização de Pintura de Blocos de Madeira (IoT)

Este projeto consiste em uma solução de IoT industrial para o acompanhamento e monitorização em tempo real do processo de pintura de blocos de madeira e das condições ambientais no chão de fábrica. 

O sistema realiza a leitura de sensores em um **ESP32 (Chão de Fábrica)**, transmite os pacotes via protocolo sem fio **ESP-NOW** para um **ESP32-S3 (Monitoramento)** equipado com matriz de LEDs MAX7219, encaminha as leituras via requisições HTTP POST para uma **API Flask**, armazena os dados no **InfluxDB** e exibe os dados em um dashboard em **Streamlit**.

---

## 🎯 Conteúdos Formativos & Objetivos
- **Comunicação sem fio com ESP-NOW:** Transmissão eficiente de pacotes de dados de 20 bytes entre microcontroladores.
- **Desenvolvimento Backend IoT em Python:** Construção de API REST em Flask para intermediação e validação de pacotes.
- **Banco de Dados Temporal:** Registro de dados em série temporal com timestamp no InfluxDB.
- **Visualização em Tempo Real:** Visualização de indicadores operacionais e alertas através de painel interativo no Streamlit.
- **Versionamento e Entrega:** Estruturação e controle de versão do projeto utilizando Git e GitHub na branch `feat/robo-lab`.

---

## 📋 Equipamentos e Materiais

| Item | Descrição / Quantidade |
| :--- | :--- |
| **Microcontrolador Emissor** | 1x ESP32 (Chão de Fábrica) |
| **Microcontrolador Receptor** | 1x ESP32-S3 (Monitoramento) |
| **Sensor de Nível** | 1x Sensor Ultrassônico HC-SR04 |
| **Sensor Ambiental** | 1x Sensor de Temperatura e Umidade DHT22 |
| **Sensor de Luz** | 1x Fotorresistor (LDR) |
| **Display Local** | 1x Matriz de LEDs MAX7219 |
| **Sinalização Visual Local** | 1x LED Verde (Operação Normal) e 1x LED Vermelho (Nível Crítico) |
| **Software / Serviços** | Arduino IDE, Python 3, Flask, InfluxDB 2.x, Streamlit, Git/GitHub |

---

## 📐 Arquitetura da Solução

```text
 [Sensores] -> [ESP32 Emissor] --(ESP-NOW)--> [ESP32-S3 Receptor] --(HTTP POST)--> [API Flask]
(DHT/HC-SR04/LDR) (Chão de Fábrica)           (Com MAX7219)                  (Porta 5000)
                                                                                  │
                                                                        (Grava no InfluxDB)
                                                                                  │
                                                                                  ▼
                                                                      [Dashboard Streamlit]
                                                                           (Porta 8501)
```

---

## 🔌 Conexão dos Componentes (Como Montar o Projeto)

### 1. ESP32 - Chão de Fábrica (Emissor)
- **Sensor Ultrassônico (HC-SR04):** Trig `GPIO 5` | Echo `GPIO 18` | VCC `5V` | GND `GND`
- **DHT22 (Temperatura e Umidade):** Data `GPIO 4` | VCC `3.3V` | GND `GND`
- **Fotorresistor (LDR):** Signal `GPIO 34` (com resistor pull-down) | VCC `3.3V` | GND `GND`
- **LED Verde (Faixa Segura >= 20%):** Anodo `GPIO 13` (com resistor 220Ω) | Catodo `GND`
- **LED Vermelho (Alerta Nível Baixo < 20%):** Anodo `GPIO 15` (com resistor 220Ω) | Catodo `GND`

### 2. ESP32-S3 - Monitoramento (Receptor)
- **Matriz de LEDs MAX7219:**
  - `DIN` -> `GPIO 11`
  - `CLK` -> `GPIO 12`
  - `CS`  -> `GPIO 10`
  - `VCC` -> `5V` | `GND` -> `GND`

---

## 💾 Banco de Dados & Visualização

* **Banco de Dados (InfluxDB 2.x):**
  * **Organização (`org`):** `Iot`
  * **Bucket:** `IOT`
  * **Measurement:** `dados_fabrica`
  * **Campos Registrados (`fields`):** `nivel` (float), `temperatura` (float), `umidade` (float), `luminosidade` (int).
* **Dashboard Visual (Streamlit):**
  * Apresenta indicadores em tempo real (KPIs), manômetro (*Gauge*) de nível do tanque com faixas de segurança, gráficos temporais de temperatura/umidade e tabela expansível com histórico dos dados.

---

## 🚀 Como Rodar o Backend e o Dashboard

### 1. Instalar as Dependências do Python
Abra o terminal na raiz do projeto e execute:
```bash
pip install flask influxdb-client streamlit plotly pandas
```

### 2. Iniciar o InfluxDB
No PowerShell, navegue até a pasta do InfluxDB e inicie o serviço:
```powershell
.\influxd.exe
```
> Mantendo o InfluxDB em execução na porta `8086`.

### 3. Iniciar a API Flask (Backend)
Navegue até a pasta `backend/` e execute:
```bash
python app.py
```
> A API escutará requisições na porta `5000` (`http://0.0.0.0:5000/api/telemetria`).

### 4. Iniciar o Dashboard Streamlit
Navegue até a pasta `dashboard/` e execute:
```bash
python -m streamlit run dashboard.py
```
> Acesse o painel pelo navegador no endereço: `http://localhost:8501`.

---

## 🔍 Como Consultar os Dados Salvos

### Opção 1: Via Dashboard Streamlit
Acesse `http://localhost:8501` no seu navegador. O painel atualiza a cada 2 segundos exibindo as métricas dos sensores, manômetro de nível e histórico temporal.

### Opção 2: Via Interface Web do InfluxDB
1. Abra a URL `http://127.0.0.1:8086` no navegador.
2. Acesse a aba **Data Explorer**.
3. Selecione a organização `Iot` e o bucket `IOT`.
4. Escolha a measurement `dados_fabrica` e clique em **Submit**.

### Opção 3: Consulta via Consulta Flux (CLI / Python)
```flux
from(bucket: "IOT")
  |> range(start: -1h)
  |> filter(fn: (r) => r["_measurement"] == "dados_fabrica")
  |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
```

---

## 📁 Versionamento e Entrega no GitHub

Este projeto foi versionado e disponibilizado no GitHub utilizando a branch de entrega exigida:

```bash
# Criar e alternar para a branch de desenvolvimento e entrega
git checkout -b feat/robo-lab

# Adicionar arquivos e realizar commit
git add .
git commit -m "feat: entrega completa do sistema IoT na branch feat/robo-lab"

# Envio para o repositório remoto
git push -u origin feat/robo-lab
```