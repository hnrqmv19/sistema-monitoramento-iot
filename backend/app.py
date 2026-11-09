"""
=============================================================================
PROJETO IOT INDUSTRIAL - BACKEND FLASK & INFLUXDB
Ficheiro: backend/app.py
Escuta na porta 5000 (0.0.0.0) e persiste dados no InfluxDB (porta 8086)
=============================================================================
"""

from flask import Flask, request, jsonify
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
import logging

app = Flask(__name__)
logging.basicConfig(level=logging.INFO)

# ===================== CONFIGURAÇÕES DO INFLUXDB =====================
INFLUX_URL = "http://127.0.0.1:8086"
INFLUX_TOKEN = "" # Insira o seu token InfluxDB
INFLUX_ORG = "" # Insira o nome da sua Organização
INFLUX_BUCKET = "" # Insira o nome do seu Bucket
 
# Conexão com o InfluxDB
client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
write_api = client.write_api(write_options=SYNCHRONOUS)

@app.route('/', methods=['GET'])
def status_api():
    """Rota raiz para teste visual de status no navegador."""
    return jsonify({
        "status": "online",
        "servico": "API de Telemetria Industrial IoT",
        "endpoint_esp32": "/api/telemetria (POST)"
    }), 200

@app.route('/api/telemetria', methods=['POST'])
def receber_telemetria():
    """Endpoint chamado pelo ESP32-S3 para persistir leituras no InfluxDB."""
    try:
        dados = request.get_json()
        if not dados:
            return jsonify({"status": "erro", "mensagem": "JSON ausente"}), 400

        # Criação do Ponto de Dados para o InfluxDB
        ponto = (
            Point("dados_fabrica")
            .field("nivel", float(dados.get("nivel", 0.0)))
            .field("temperatura", float(dados.get("temperatura", 0.0)))
            .field("umidade", float(dados.get("umidade", 0.0)))
            .field("luminosidade", int(dados.get("luminosidade", 0)))
        )

        write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=ponto)
        print(f"[SUCCESS] Dados gravados no InfluxDB: {dados}", flush=True)
        
        return jsonify({"status": "sucesso", "mensagem": "Dados persistidos"}), 201

    except Exception as e:
        print(f"[ERROR] Erro ao gravar no InfluxDB: {str(e)}", flush=True)
        return jsonify({"status": "erro", "mensagem": str(e)}), 500

if __name__ == '__main__':
    print("==================================================", flush=True)
    print(" Iniciando Servidor API Flask na porta 5000...", flush=True)
    print("==================================================", flush=True)
    app.run(host='0.0.0.0', port=5000, debug=True)