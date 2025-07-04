# ESP32 Wi-Fi Cliente

Este projeto implementa um cliente Wi-Fi usando ESP32 com ESP-IDF que se conecta ao Access Point.

## Funcionalidades

- Conecta automaticamente ao AP "ESP32_AP"
- Senha: "12345678"
- Reconexão automática em caso de desconexão
- Monitoramento contínuo da conexão
- Múltiplas formas de verificar se está conectado

## Funções Principais Implementadas

### Funções Obrigatórias ESP-IDF:
- `esp_netif_create_default_wifi_sta()`: Cria interface de rede para modo Station
- `esp_wifi_init()`: Inicializa o Wi-Fi
- `wifi_config_t`: Configuração com SSID e senha do AP
- `esp_wifi_set_mode(WIFI_MODE_STA)`: Define modo Cliente
- `esp_wifi_set_config(WIFI_IF_STA, &wifi_config)`: Aplica configuração
- `esp_wifi_start()`: Inicia o Wi-Fi
- `esp_wifi_connect()`: Conecta ao AP

### Event Handlers:
- `WIFI_EVENT_STA_START`: Inicia tentativa de conexão
- `WIFI_EVENT_STA_DISCONNECTED`: Gerencia reconexão automática
- `IP_EVENT_STA_GOT_IP`: **PRINCIPAL** - Confirma que obteve IP do AP

##  Métodos de Verificação de Conexão

### 1. **Event `IP_EVENT_STA_GOT_IP` (OBRIGATÓRIO)**
```c
// Este é o evento mais importante para confirmar conexão
if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ESP_LOGI(TAG, "🎉 CONECTADO COM SUCESSO! 🎉");
    ESP_LOGI(TAG, "IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));
    is_connected = true;
}
```

### 2. **Variável de Status Global**
```c
static bool is_connected = false; // Atualizada pelos events
```

### 3. **Informações da Conexão**
```c
wifi_ap_record_t ap_info;
esp_wifi_sta_get_ap_info(&ap_info); // Obtém SSID, RSSI, canal, etc.
```

### 4. **Teste de Ping para o AP**
```c
ping_test(); // Ping para 192.168.4.1 (IP do AP)
```

### 5. **Logs Visuais com Emojis**
- 🎉 Conectado com sucesso
-  Status OK
-  Falha na conexão
-  Ping OK
-  Testando conectividade
-  Estatísticas de ping

### 6. **Monitoramento Contínuo**
- Task que verifica conexão a cada 30 segundos
- Reconexão automática (até 5 tentativas)
- Logs detalhados de status

## Como Verificar se o Cliente Está Conectado

### No Monitor Serial:
```
🎉 CONECTADO COM SUCESSO! 🎉
IP obtido: 192.168.4.2
Netmask: 255.255.255.0
Gateway: 192.168.4.1
 Conectado ao AP ESP32_AP com sucesso!
```

### No AP (Access Point):
```
Cliente conectado - MAC: xx:xx:xx:xx:xx:xx
```

### Informações Detalhadas:
```
=== INFORMAÇÕES DA CONEXÃO ===
SSID: ESP32_AP
RSSI: -45 dBm
Canal: 1
IP do Cliente: 192.168.4.2
Gateway (AP): 192.168.4.1
```

### Teste de Conectividade:
```
 Testando conectividade com ping para o AP...
 Ping OK - seq=1, ttl=64, time=5ms
 Ping OK - seq=2, ttl=64, time=3ms
 Ping OK - seq=3, ttl=64, time=4ms
 Ping finalizado: 3/3 pacotes, tempo total: 3000ms
```

## Como Compilar e Executar

```bash
cd CLIENTS
idf.py build
idf.py flash monitor
```

## Configurações

Para conectar em outro AP, modifique em `CLIENTS.c`:
```c
#define AP_SSID "ESP32_AP"    // Nome da rede
#define AP_PASS "12345678"    // Senha
#define AP_IP "192.168.4.1"   // IP do AP para ping
```

## Faixas de IP

- **AP (Access Point)**: `192.168.4.1`
- **Clientes**: `192.168.4.2` até `192.168.4.254`
- **Máscara de rede**: `255.255.255.0`

## Troubleshooting

1. **Não conecta**: Verifique SSID e senha
2. **Conecta mas perde conexão**: Verifique sinal Wi-Fi
3. **Não obtém IP**: Verifique se o AP está funcionando
4. **Ping falha**: Verifique conectividade de rede

## Estados de Conexão

-  **Inicializando**: Sistema iniciando
-  **Conectando**: Tentando conectar ao AP
- 🎉 **Conectado**: Obteve IP com sucesso
-  **Desconectado**: Perdeu conexão
-  **Reconectando**: Tentativa automática de reconexão
