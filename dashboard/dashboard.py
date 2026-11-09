"""
=============================================================================
PROJETO IOT INDUSTRIAL - DASHBOARD STREAMLIT
Ficheiro: dashboard/dashboard.py
Visualização de dados em tempo real a partir do InfluxDB
=============================================================================
"""

import streamlit as st
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from influxdb_client import InfluxDBClient
import time

# ===================== CONFIGURAÇÃO DA PÁGINA =====================
st.set_page_config(
    page_title="Monitorização Industrial - ESP32",
    page_icon="🏭",
    layout="wide"
)

# ===================== CONFIGURAÇÕES DO INFLUXDB =====================
INFLUX_URL = "http://127.0.0.1:8086"
INFLUX_TOKEN = "" # Insira o seu token InfluxDB
INFLUX_ORG = "" # Insira o nome da sua Organização
INFLUX_BUCKET = "" # Insira o nome do seu Bucket

@st.cache_resource
def get_influx_client():
    return InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)

client = get_influx_client()
query_api = client.query_api()

def buscar_dados():
    # Consulta Flux com pivot para estruturar colunas
    query = f'''
    from(bucket: "{INFLUX_BUCKET}")
      |> range(start: -1h)
      |> filter(fn: (r) => r["_measurement"] == "dados_fabrica")
      |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
    '''
    try:
        df = query_api.query_data_frame(query)
        if isinstance(df, list):
            df = pd.concat(df)
        if not df.empty:
            df['_time'] = pd.to_datetime(df['_time'])
            df = df.sort_values('_time')
        return df
    except Exception as e:
        st.error(f"Erro ao ligar ao InfluxDB: {e}")
        return pd.DataFrame()

# ===================== INTERFACE GRÁFICA =====================
st.title("🏭 Painel de Monitorização Industrial")
st.caption("Dados transmitidos via ESP-NOW -> ESP32-S3 -> API Flask -> InfluxDB")

# Barra lateral
st.sidebar.header("Configurações")
auto_refresh = st.sidebar.checkbox("Atualização Automática (2s)", value=True)

df_dados = buscar_dados()

if df_dados.empty:
    st.warning("⚠️ Nenhum dado encontrado no InfluxDB nos últimos 60 minutos. Certifique-se de que a API Python e os ESP32 estão a transmitir.")
else:
    # Última leitura recebida
    ultimo = df_dados.iloc[-1]

    # --- LINHA 1: CARTÕES MÉTRICOS (KPIs) ---
    col1, col2, col3, col4 = st.columns(4)
    
    col1.metric(
        label="Nível de Tinta",
        value=f"{ultimo.get('nivel', 0.0):.1f} %"
    )
    col2.metric(
        label="Temperatura",
        value=f"{ultimo.get('temperatura', 0.0):.1f} °C"
    )
    col3.metric(
        label="Umidade",
        value=f"{ultimo.get('umidade', 0.0):.1f} %"
    )
    col4.metric(
        label="Luminosidade",
        value=f"{int(ultimo.get('luminosidade', 0))} lux"
    )

    st.markdown("---")

    # --- LINHA 2: GRÁFICOS INTERATIVOS ---
    g_col1, g_col2 = st.columns([1, 2])

    # Manómetro (Gauge) de Nível
    with g_col1:
        st.subheader("Nível Atual do Tanque")
        fig_gauge = go.Figure(go.Indicator(
            mode="gauge+number",
            value=float(ultimo.get('nivel', 0.0)),
            domain={'x': [0, 1], 'y': [0, 1]},
            number={'suffix': "%"},
            gauge={
                'axis': {'range': [0, 100]},
                'bar': {'color': "#1f77b4"},
                'steps': [
                    {'range': [0, 20], 'color': "#ff4b4b"},
                    {'range': [20, 80], 'color': "#faca2b"},
                    {'range': [80, 100], 'color': "#2cfec1"}
                ]
            }
        ))
        fig_gauge.update_layout(height=320, margin=dict(l=20, r=20, t=30, b=20))
        st.plotly_chart(fig_gauge, use_container_width=True)

    # Histórico de Temperatura e Umidade
    with g_col2:
        st.subheader("Histórico de Temperatura e Umidade")
        fig_temp = px.line(
            df_dados, 
            x="_time", 
            y=["temperatura", "umidade"],
            labels={"_time": "Horário", "value": "Valor", "variable": "Variável"},
            color_discrete_map={"temperatura": "#EF553B", "umidade": "#636EFA"}
        )
        fig_temp.update_layout(height=320, margin=dict(l=20, r=20, t=30, b=20))
        st.plotly_chart(fig_temp, use_container_width=True)

    # --- LINHA 3: TABELA DE REGISTOS RECENTES ---
    with st.expander("📄 Ver Registos Brutos no InfluxDB"):
        st.dataframe(
            df_dados[['_time', 'nivel', 'temperatura', 'umidade', 'luminosidade']]
            .tail(15)
            .sort_values('_time', ascending=False),
            use_container_width=True
        )

# Loop de auto-refresh de 2 segundos
if auto_refresh:
    time.sleep(2)
    st.rerun()