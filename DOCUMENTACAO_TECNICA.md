# 🔧 Documentação Técnica - Sistema WiFi Security Research

## 📋 Arquitetura Geral do Sistema

Este projeto implementa um **ambiente de pesquisa em segurança WiFi** utilizando múltiplos ESP32 para simular ataques e defesas em tempo real. A arquitetura modular permite estudos isolados de cada tipo de ataque e suas respectivas contramedidas.

## 🏗️ Componentes do Sistema

### 1. 🛡️ Access Point Inteligente (AP/)
**Função**: Ponto de acesso com sistema IDS integrado  
**Hardware**: ESP32-WROOM-32  
**Características**:
- IDS baseado em detecção de anomalias
- Sistema de blacklist automática
- Monitoramento de métricas em tempo real
- Logging detalhado de eventos de segurança

```c
// Configuração técnica do AP
#define AP_SSID "ESP32_AP"
#define AP_CHANNEL 1
#define AP_MAX_STA_CONN 20
#define AP_BEACON_INTERVAL 100
#define AP_AUTH_MODE WIFI_AUTH_WPA2_PSK
```

### 2. 👥 Clientes Legítimos (CLIENTS/)
**Função**: Simulação de tráfego normal  
**Comportamento**: 
- Conexão estável ao AP
- Tráfego TCP/UDP regular
- Padrões de comunicação previsíveis

### 3. 💥 Módulos de Ataque

#### 🔄 ConnectFlood
**Tipo**: DoS Layer 2 - Esgotamento de slots de conexão  
**Técnica**: Ciclos rápidos de connect/disconnect  
**Taxa**: 10 tentativas/segundo  
**Objetivo**: Saturar tabela de associação do AP

#### ⚡ DeauthFlood  
**Tipo**: DoS Layer 2 - Frames de management  
**Técnica**: Desautenticações forçadas  
**Taxa**: 8 deauths/segundo  
**Objetivo**: Interromper comunicação de clientes legítimos

#### 🔐 AuthFlood
**Tipo**: Resource exhaustion attack  
**Técnica**: Tentativas massivas de autenticação  
**Taxa**: 10 auths/segundo  
**Objetivo**: Sobrecarregar sistema de autenticação

#### 📡 PacketFlood
**Tipo**: Bandwidth exhaustion - Layer 3/4  
**Técnica**: Alto volume de tráfego UDP/TCP  
**Taxa**: 100 pacotes/segundo  
**Objetivo**: Saturar largura de banda

#### 🕸️ ArpSpoof
**Tipo**: Man-in-the-Middle - Layer 2  
**Técnica**: Envenenamento de tabela ARP  
**Taxa**: 1 spoof/2 segundos  
**Objetivo**: Interceptar tráfego de rede

#### 🎭 EvilTwin
**Tipo**: AP Impersonation + Social Engineering  
**Técnica**: AP falso com mesmo SSID  
**Canal**: Diferente do original (detectabilidade)  
**Objetivo**: Capturar credenciais e tráfego

## 🚀 Guia de Execução Técnica

### 📋 Pré-requisitos do Sistema

#### Hardware Necessário
```
┌─────────────────┬─────────────────┬─────────────────┐
│ Componente      │ Quantidade      │ Especificação   │
├─────────────────┼─────────────────┼─────────────────┤
│ ESP32           │ Mínimo 2        │ WROOM-32        │
│ Cabos USB       │ 1 por ESP32     │ Micro-USB       │
│ Computador      │ 1               │ Linux/Windows   │
│ Monitor Serial  │ Software        │ ESP-IDF Monitor │
└─────────────────┴─────────────────┴─────────────────┘
```

#### Software Requirements
```bash
# ESP-IDF Installation
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v4.4.6
./install.sh esp32

# Environment Setup  
. ./export.sh
```

### 🔧 Configuração do Ambiente

#### 1. Setup do Access Point
```bash
cd AP/
idf.py set-target esp32
idf.py menuconfig  # Opcional: ajustar configurações
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

**Logs Esperados do AP**:
```
🛡️ === ACCESS POINT SEGURO INICIADO ===
📡 SSID: ESP32_AP, Canal: 1, Max clients: 20
🔐 Sistema de detecção: ATIVO
⚡ Thresholds: 5 conn/s, 8 disc/s, 10 auth/s, 50 pkt/s
🌐 IP do AP: 192.168.4.1
📊 Blacklist: 0/20 entradas
✅ Pronto para receber conexões
```

#### 2. Verificação com Cliente Legítimo
```bash
cd CLIENTS/
idf.py build flash monitor
```

**Logs Esperados do Cliente**:
```
👤 === CLIENTE LEGÍTIMO INICIADO ===
📡 Tentando conectar ao AP: ESP32_AP
✅ Conectado! IP: 192.168.4.100
🔗 Iniciando tráfego TCP normal...
📊 Enviando heartbeat a cada 5 segundos
```

#### 3. Execução de Ataques

**ConnectFlood**:
```bash
cd ConnectFlood/
idf.py build flash monitor
```

**Logs de Ataque**:
```
🔥 === CONNECT FLOOD INICIADO ===
💥 Tentativa #1 - MAC: 02:aa:bb:cc:dd:01
✅ Conectado! Desconectando...
💥 Tentativa #2 - MAC: 02:aa:bb:cc:dd:02
❌ Rejeitado! Rate limit ativo
🚫 ATAQUE DETECTADO - Blacklist confirmada
```

### 📊 Sistema de Monitoramento

#### Métricas do IDS
```c
typedef struct {
    uint32_t connections_per_second;
    uint32_t disconnections_per_second;  
    uint32_t auth_attempts_per_second;
    uint32_t packets_per_second;
    uint32_t bytes_per_second;
    bool evil_twin_detected;
} ids_metrics_t;
```

#### Dashboard de Métricas (logs)
```
📊 === MÉTRICAS DO AP (último minuto) ===
├── Clientes conectados: 3/20
├── Conexões/seg: 2.3 (limite: 5)
├── Desconexões/seg: 1.1 (limite: 8)  
├── Auths/seg: 0.8 (limite: 10)
├── Pacotes/seg: 45.2 (limite: 50/cliente)
├── Blacklist ativa: 2/20
└── Evil Twin detectado: NÃO
```

## 🔍 Análise de Performance

### Benchmarks do Sistema

#### CPU e Memória
```c
// Métricas coletadas durante execução normal
performance_metrics_t normal_operation = {
    .cpu_usage_percent = 15,
    .free_heap_bytes = 180000,
    .largest_free_block = 90000,
    .minimum_free_heap = 150000
};

// Métricas durante ataque ativo
performance_metrics_t under_attack = {
    .cpu_usage_percent = 85,
    .free_heap_bytes = 120000,
    .largest_free_block = 40000,
    .minimum_free_heap = 80000
};
```

#### Network Performance
```
Normal Operation:
├── Throughput: 8-12 Mbps
├── Latency: 45-80ms
├── Packet Loss: <1%
└── Connections: 2-5 simultâneas

Under Attack:
├── Throughput: 0.5-2 Mbps  
├── Latency: 2000-8000ms
├── Packet Loss: 15-40%
└── Connections: 20 (máximo)

With Defense:
├── Throughput: 6-9 Mbps
├── Latency: 120-250ms
├── Packet Loss: 2-5%
└── Connections: Limitadas por blacklist
```

## 🛠️ Troubleshooting Técnico

### Problemas Comuns e Soluções

#### 1. Erro de Compilação
```bash
# Erro: "esp_wifi.h not found"
Solução:
idf.py clean
idf.py set-target esp32
idf.py build
```

#### 2. Falha na Conexão WiFi
```bash
# Verificar configuração
idf.py menuconfig
# Component config → ESP32-specific → WiFi
# Aumentar "Max number of WiFi static RX buffers"
```

#### 3. ESP32 não Detectado
```bash
# Verificar porta serial
ls /dev/ttyUSB*
# ou no Windows
# Verificar Device Manager → Ports

# Instalar drivers CP210x se necessário
```

#### 4. Memória Insuficiente
```c
// Ajustar configurações de memória
#define CONFIG_ESP32_WIFI_RX_BUFFER_NUM 25
#define CONFIG_ESP32_WIFI_TX_BUFFER_NUM 40
#define CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM 32
```

### Debug Avançado

#### Enable Verbose Logging
```c
// No arquivo principal
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

// No menuconfig
// Component config → Log output → Default log verbosity → Debug
```

#### Memory Monitoring
```c
void monitor_memory(void) {
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Largest free block: %d bytes", 
             heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
    ESP_LOGI(TAG, "Minimum free heap: %d bytes", 
             esp_get_minimum_free_heap_size());
}
```

#### WiFi Debugging
```c
// Habilitar debug do WiFi
esp_log_level_set("wifi", ESP_LOG_DEBUG);
esp_log_level_set("esp_netif_lwip", ESP_LOG_DEBUG);
```

## 📈 Configurações Avançadas

### Tuning de Performance

#### Configurações do AP
```c
// Otimização para testes de ataque
wifi_config_t ap_config = {
    .ap = {
        .ssid = AP_SSID,
        .ssid_len = strlen(AP_SSID),
        .channel = AP_CHANNEL,
        .password = AP_PASS,
        .max_connection = AP_MAX_STA_CONN,
        .authmode = WIFI_AUTH_WPA2_PSK,
        .beacon_interval = 100,      // ms - mais frequente para detecção
        .dtim_period = 2,            // Delivery traffic indication
        .pairwise_cipher = WIFI_CIPHER_TYPE_CCMP,
        .group_cipher = WIFI_CIPHER_TYPE_CCMP,
        .pmf_cfg = {
            .required = false        // PMF desabilitado para vulnerabilidade
        }
    }
};
```

#### Thresholds do IDS
```c
// Ajustar sensibilidade do sistema de detecção
#define CONNECT_FLOOD_THRESHOLD 5     // conexões/segundo
#define DEAUTH_FLOOD_THRESHOLD 8      // desconexões/segundo  
#define AUTH_FLOOD_THRESHOLD 10       // tentativas auth/segundo
#define PACKET_FLOOD_THRESHOLD 50     // pacotes/segundo/cliente
#define BLACKLIST_DURATION_MS 60000   // 60 segundos
#define MAX_BLACKLIST_ENTRIES 20      // entradas simultâneas
```

### Customização de Ataques

#### Ajustar Intensidade
```c
// ConnectFlood - menos agressivo
#define FLOOD_INTERVAL_MS 500   // de 100ms para 500ms

// AuthFlood - mais agressivo  
#define AUTH_FLOOD_INTERVAL_MS 100  // de 200ms para 100ms

// PacketFlood - pacotes maiores
#define LARGE_PACKET_SIZE 1500  // MTU completo
```

#### MAC Address Randomization
```c
void randomize_mac_address(void) {
    uint8_t random_mac[6];
    esp_fill_random(random_mac, 6);
    
    // Manter OUI específico para identificação
    random_mac[0] = 0x02;  // Locally administered
    random_mac[1] = 0xAA;  // Identificador do projeto
    random_mac[2] = 0xBB;  // Identificador do projeto
    
    esp_wifi_set_mac(WIFI_IF_STA, random_mac);
}
```

## 📚 Estrutura de Código

### Organização dos Arquivos
```
Cada módulo segue a estrutura padrão ESP-IDF:
├── CMakeLists.txt          # Build configuration
├── sdkconfig              # Configurações do projeto
├── main/
│   ├── CMakeLists.txt     # Build config do main
│   └── [Module].c         # Código principal
├── README_TECNICO.md      # Documentação técnica
└── build/                 # Arquivos de build (gerados)
```

### Padrões de Codificação
```c
// Header padrão dos módulos
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

// Configurações sempre #define no topo
#define TARGET_SSID "ESP32_AP"
#define TARGET_PASS "12345678"

// TAG para logging sempre igual ao nome do módulo
static const char *TAG = "MODULE_NAME";
```

## 🔬 Metodologia de Teste

### Protocolo Experimental
```python
# Sequência padrão de testes
def run_security_test():
    # 1. Baseline (30 segundos)
    baseline = collect_metrics(30)
    
    # 2. Ataque (60 segundos)  
    start_attack()
    attack_metrics = collect_metrics(60)
    
    # 3. Detecção e mitigação
    detection_time = measure_detection()
    mitigation_time = measure_mitigation()
    
    # 4. Recuperação (30 segundos)
    stop_attack()
    recovery_metrics = collect_metrics(30)
    
    return compile_report(baseline, attack_metrics, 
                         detection_time, mitigation_time, 
                         recovery_metrics)
```

### Métricas Coletadas
```c
typedef struct {
    // Performance do sistema
    float cpu_usage_percent;
    uint32_t free_heap_bytes;
    uint32_t network_throughput_bps;
    uint32_t average_latency_ms;
    
    // Métricas de segurança  
    uint32_t attack_attempts;
    uint32_t successful_attacks;
    uint32_t blocked_attempts;
    uint32_t false_positives;
    
    // Tempos de resposta
    uint32_t detection_time_ms;
    uint32_t mitigation_time_ms;
    uint32_t recovery_time_ms;
} test_metrics_t;
```

---

📋 **Esta documentação técnica fornece as informações necessárias para configuração, execução e customização completa do sistema de pesquisa em segurança WiFi.**
