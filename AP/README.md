# Access Point Inteligente com Sistema de Defesa

## Visão Técnica

Este módulo implementa um **Access Point WiFi com sistema de detecção e mitigação de ataques** baseado em ESP32. O AP possui capacidades de IDS (Intrusion Detection System) em tempo real e mecanismos automatizados de resposta a ameaças.

## 🏗️ Arquitetura do Sistema

### 📡 Configuração de Rede
```c
// Configurações do AP
#define AP_SSID "ESP32_AP"
#define AP_PASS "12345678"
#define AP_CHANNEL 1
#define AP_MAX_STA_CONN 20
#define AP_IP "192.168.4.1"
#define AP_GATEWAY "192.168.4.1"
#define AP_NETMASK "255.255.255.0"
```

### 🔍 Sistema de Detecção (IDS)
O AP implementa um IDS baseado em **análise comportamental** e **detecção de anomalias**:

#### **Métricas Monitoradas**
| Métrica | Threshold | Janela | Ação |
|---------|-----------|--------|------|
| Conexões/seg | 5 | 1s | Blacklist 60s |
| Desconexões/seg | 8 | 1s | Blacklist 60s |
| Auth attempts/seg | 10 | 1s | Rate limiting |
| Pacotes/seg/cliente | 50 | 1s | Traffic shaping |
| Duplicate SSIDs | 1 | - | Evil Twin alert |

#### **Algoritmo de Detecção**
```c
typedef struct {
    uint8_t mac[6];
    uint32_t connections_per_sec;
    uint32_t disconnections_per_sec;
    uint32_t auth_attempts_per_sec;
    uint32_t packets_per_sec;
    uint32_t last_seen;
    attack_type_t detected_attack;
} client_metrics_t;

bool analyze_client_behavior(client_metrics_t *client) {
    if (client->connections_per_sec > CONNECT_FLOOD_THRESHOLD) {
        return trigger_mitigation(client, CONNECT_FLOOD);
    }
    // ... outras análises
}
```
    
    // Servidor de testes
    start_test_server();
}
```

#### 3.2 Configuração do SoftAP
```c
wifi_config_t wifi_config = {
    .ap = {
        .ssid = EXAMPLE_ESP_WIFI_SSID,
        .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
        .channel = EXAMPLE_ESP_WIFI_CHANNEL,
        .password = EXAMPLE_ESP_WIFI_PASS,
        .max_connection = EXAMPLE_MAX_STA_CONN,
        .authmode = WIFI_AUTH_WPA2_PSK,
        .pmf_cfg = {
            .required = false,  // PMF desabilitado intencionalmente
        },
    },
};
```

### 4. Monitoramento e Logging

#### 4.1 Eventos Monitorados
- Conexões de clientes (`WIFI_EVENT_AP_STACONNECTED`)
- Desconexões de clientes (`WIFI_EVENT_AP_STADISCONNECTED`)
- Tentativas de autenticação (`WIFI_EVENT_AP_START`)
- Tráfego HTTP no servidor de teste

#### 4.2 Métricas Coletadas
```c
typedef struct {
    int clients_connected;
    int total_connections;
    int disconnections;
    unsigned long uptime;
    float throughput;
} ap_stats_t;
```

## Como Executar

### 1. Preparação do Ambiente
```bash
# Configurar ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Navegar para o diretório
cd AP/
```

### 2. Compilação e Flash
```bash
# Limpar build anterior
idf.py clean

# Configurar projeto (opcional)
idf.py menuconfig

# Compilar
idf.py build

# Flash no ESP32
idf.py -p /dev/ttyUSB0 flash

# Monitorar saída
idf.py -p /dev/ttyUSB0 monitor
```

### 3. Verificação de Funcionamento
```bash
# Buscar o SSID ESP32_AP em dispositivos próximos
# Conectar com senha: 12345678
# Acessar http://192.168.4.1 no navegador
```

## Análise de Logs

### 1. Padrões Normais de Operação
```
I (1234) AP: AP started on channel 6
I (2345) AP: Cliente conectado: 24:0a:c4:xx:xx:xx
I (3456) AP: HTTP server started on port 80
```

### 2. Detecção de Ataques
```
W (5678) AP: Múltiplas tentativas de autenticação detectadas
W (6789) AP: Cliente desconectado inesperadamente: 24:0a:c4:xx:xx:xx
E (7890) AP: Alto número de frames de deautenticação recebidos
```

### 3. Métricas de Performance
```
I (10000) AP: Stats - Clientes: 3, Throughput: 1.2 Mbps, Uptime: 10s
W (15000) AP: Performance degradada - Latência alta detectada
```

## Uso Responsável

⚠️ **AVISO IMPORTANTE**: Este AP vulnerável deve ser usado APENAS em ambientes controlados e isolados. Nunca ativar em redes de produção ou espaços públicos.
- Logs de conexão e desconexão de clientes

## Configuração

As principais configurações podem ser alteradas no arquivo `AP.c`:

```c
#define AP_SSID "ESP32_AP"        // Nome da rede Wi-Fi
#define AP_PASS "12345678"        // Senha (mínimo 8 caracteres)
#define AP_CHANNEL 1              // Canal Wi-Fi
#define AP_MAX_STA_CONN 4         // Máximo de clientes conectados
```

## Como Compilar e Executar

1. Configure o ambiente ESP-IDF
2. No diretório do projeto, execute:

```bash
idf.py build
idf.py flash monitor
```

## Funções Principais Implementadas

- `esp_netif_create_default_wifi_ap()`: Cria a interface de rede para o modo AP
- `wifi_init_config_t`: Configuração inicial do Wi-Fi com valores padrão
- `esp_wifi_init()`: Inicializa o Wi-Fi
- `wifi_config_t`: Configuração detalhada do AP (SSID, senha, canal, autenticação)
- `esp_wifi_set_mode(WIFI_MODE_AP)`: Define o modo de operação como AP
- `esp_wifi_set_config()`: Aplica a configuração ao AP
- `esp_wifi_start()`: Inicia o Wi-Fi

## Event Handler

O código inclui um event handler que monitora:
- Conexão de novos clientes (`WIFI_EVENT_AP_STACONNECTED`)
- Desconexão de clientes (`WIFI_EVENT_AP_STADISCONNECTED`)

## Logs

O sistema produz logs informativos sobre:
- Inicialização do AP
- Configurações ativas
- Conexões e desconexões de clientes
- Status geral do sistema

## IP do Access Point

Por padrão, o ESP32 em modo AP usa o IP: `192.168.4.1`
Os clientes conectados recebem IPs na faixa: `192.168.4.2` a `192.168.4.254`
