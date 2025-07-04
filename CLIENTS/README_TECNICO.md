# CLIENTS - Simuladores de Clientes Legítimos

## Descrição Técnica

Este módulo implementa simuladores de clientes WiFi legítimos que se conectam ao Access Point para servir como alvos dos ataques implementados nos outros módulos. Os clientes simulam comportamento típico de dispositivos reais, gerando tráfego de rede e mantendo conexões ativas.

## Funcionalidades Implementadas

### 1. Simulação de Dispositivos Diversos
```c
typedef enum {
    CLIENT_TYPE_SMARTPHONE,    // Smartphone típico
    CLIENT_TYPE_LAPTOP,        // Notebook/laptop
    CLIENT_TYPE_IOT_DEVICE,    // Dispositivo IoT
    CLIENT_TYPE_TABLET,        // Tablet
    CLIENT_TYPE_SMART_TV       // Smart TV
} client_device_type_t;

typedef struct {
    client_device_type_t device_type;
    char device_name[32];
    uint8_t mac_address[6];
    char user_agent[128];
    uint32_t typical_bandwidth_kbps;
    uint32_t connection_interval_ms;
} client_profile_t;
```

### 2. Comportamentos de Tráfego Realistas
```c
typedef struct {
    uint32_t http_requests_per_hour;
    uint32_t dns_queries_per_hour;
    uint32_t background_sync_interval_sec;
    uint32_t idle_periods_duration_sec;
    bool streaming_active;
    bool download_active;
} traffic_behavior_t;
```

### 3. Implementação de Cliente Base
```c
static void client_simulation_task(void *pvParameters) {
    client_config_t *config = (client_config_t*)pvParameters;
    
    ESP_LOGI(TAG, " Iniciando cliente %s (%s)", 
             config->profile.device_name, 
             get_device_type_name(config->profile.device_type));
    
    // Configurar MAC único para este cliente
    esp_wifi_set_mac(WIFI_IF_STA, config->profile.mac_address);
    
    // Conectar ao AP
    if (connect_to_ap(config) == ESP_OK) {
        ESP_LOGI(TAG, " Cliente %s conectado com sucesso", config->profile.device_name);
        
        // Iniciar simulação de atividades
        simulate_device_behavior(config);
    } else {
        ESP_LOGE(TAG, " Falha na conexão do cliente %s", config->profile.device_name);
    }
}
```

## Tipos de Clientes Simulados

### 1. Smartphone
```c
client_profile_t smartphone_profile = {
    .device_type = CLIENT_TYPE_SMARTPHONE,
    .device_name = "iPhone-12",
    .mac_address = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC},
    .user_agent = "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0)",
    .typical_bandwidth_kbps = 500,
    .connection_interval_ms = 30000
};

void simulate_smartphone_behavior(client_config_t *config) {
    while (keep_running) {
        // Atividades típicas de smartphone
        simulate_social_media_usage();
        vTaskDelay(pdMS_TO_TICKS(15000));
        
        simulate_instant_messaging();
        vTaskDelay(pdMS_TO_TICKS(8000));
        
        simulate_web_browsing();
        vTaskDelay(pdMS_TO_TICKS(20000));
        
        simulate_app_background_sync();
        vTaskDelay(pdMS_TO_TICKS(45000));
        
        // Período de inatividade
        simulate_idle_period(60000);
    }
}
```

### 2. Laptop/Notebook
```c
client_profile_t laptop_profile = {
    .device_type = CLIENT_TYPE_LAPTOP,
    .device_name = "MacBook-Pro",
    .mac_address = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
    .user_agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)",
    .typical_bandwidth_kbps = 2000,
    .connection_interval_ms = 10000
};

void simulate_laptop_behavior(client_config_t *config) {
    while (keep_running) {
        // Atividades típicas de laptop
        simulate_web_development();
        vTaskDelay(pdMS_TO_TICKS(120000));  // 2 minutos
        
        simulate_video_conference();
        vTaskDelay(pdMS_TO_TICKS(300000));  // 5 minutos
        
        simulate_file_download();
        vTaskDelay(pdMS_TO_TICKS(60000));
        
        simulate_email_sync();
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
```

### 3. Dispositivo IoT
```c
client_profile_t iot_profile = {
    .device_type = CLIENT_TYPE_IOT_DEVICE,
    .device_name = "SmartSensor-01",
    .mac_address = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55},
    .user_agent = "IoTDevice/1.0",
    .typical_bandwidth_kbps = 10,
    .connection_interval_ms = 300000  // 5 minutos
};

void simulate_iot_behavior(client_config_t *config) {
    while (keep_running) {
        // Comportamento típico de IoT
        send_sensor_data();
        vTaskDelay(pdMS_TO_TICKS(300000));  // Dados a cada 5 minutos
        
        check_firmware_updates();
        vTaskDelay(pdMS_TO_TICKS(3600000)); // Check a cada hora
        
        send_heartbeat();
        vTaskDelay(pdMS_TO_TICKS(60000));   // Heartbeat a cada minuto
    }
}
```

## Simulação de Atividades de Rede

### 1. Navegação Web
```c
void simulate_web_browsing(void) {
    const char* popular_sites[] = {
        "www.google.com",
        "www.facebook.com", 
        "www.youtube.com",
        "www.amazon.com",
        "www.wikipedia.org"
    };
    
    int site_index = esp_random() % (sizeof(popular_sites) / sizeof(char*));
    
    ESP_LOGD(TAG, " Simulando acesso a %s", popular_sites[site_index]);
    
    // Simular requisição HTTP
    simulate_http_request(popular_sites[site_index]);
    
    // Simular download de recursos (CSS, JS, imagens)
    int num_resources = 3 + (esp_random() % 8);  // 3-10 recursos
    for (int i = 0; i < num_resources; i++) {
        simulate_resource_download(100 + esp_random() % 500);  // 100-600 bytes
        vTaskDelay(pdMS_TO_TICKS(50 + esp_random() % 200));   // 50-250ms entre recursos
    }
}
```

### 2. Streaming de Vídeo
```c
void simulate_video_streaming(uint32_t duration_ms) {
    ESP_LOGI(TAG, "📺 Iniciando streaming de vídeo (%d segundos)", duration_ms/1000);
    
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t bytes_per_second = 500 * 1024;  // 500 KB/s para vídeo HD
    
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) < duration_ms) {
        // Simular download de chunks de vídeo
        simulate_data_transfer(bytes_per_second / 10);  // Chunk de 100ms
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Ocasionalmente simular buffering
        if (esp_random() % 100 < 5) {  // 5% chance
            ESP_LOGD(TAG, "⏳ Buffering...");
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
    
    ESP_LOGI(TAG, " Streaming concluído");
}
```

### 3. Sincronização de E-mail
```c
void simulate_email_sync(void) {
    ESP_LOGD(TAG, "📧 Sincronizando e-mails...");
    
    // Simular conexão IMAP/POP3
    simulate_tcp_connection("mail.server.com", 993);
    
    // Simular download de headers
    int num_emails = esp_random() % 20;  // 0-19 novos e-mails
    for (int i = 0; i < num_emails; i++) {
        simulate_data_transfer(512);  // 512 bytes por header
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Simular download de e-mails completos
    int full_downloads = esp_random() % 5;  // 0-4 e-mails completos
    for (int i = 0; i < full_downloads; i++) {
        uint32_t email_size = 2048 + esp_random() % 10240;  // 2-12 KB
        simulate_data_transfer(email_size);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    ESP_LOGD(TAG, " Sincronização concluída: %d novos e-mails", num_emails);
}
```

### 4. Transferência de Arquivos
```c
void simulate_file_download(void) {
    uint32_t file_size = 1024 * 1024 + esp_random() % (10 * 1024 * 1024);  // 1-11 MB
    uint32_t download_speed = 200 * 1024;  // 200 KB/s
    
    ESP_LOGI(TAG, "📥 Iniciando download de arquivo (%.1f MB)", file_size / (1024.0 * 1024.0));
    
    uint32_t downloaded = 0;
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    while (downloaded < file_size) {
        uint32_t chunk_size = 8192;  // 8 KB chunks
        if (downloaded + chunk_size > file_size) {
            chunk_size = file_size - downloaded;
        }
        
        simulate_data_transfer(chunk_size);
        downloaded += chunk_size;
        
        // Calcular progresso
        float progress = (float)downloaded / file_size * 100;
        if ((int)progress % 10 == 0) {  // Log a cada 10%
            ESP_LOGD(TAG, " Download: %.1f%% (%.1f KB/s)", 
                     progress, 
                     downloaded / ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) / 1000.0) / 1024);
        }
        
        vTaskDelay(pdMS_TO_TICKS(chunk_size * 1000 / download_speed));
    }
    
    ESP_LOGI(TAG, " Download concluído");
}
```

## Gerenciamento de Múltiplos Clientes

### 1. Coordenador de Clientes
```c
typedef struct {
    client_config_t clients[MAX_CLIENTS];
    int active_clients;
    bool simulation_running;
    uint32_t total_traffic_bytes;
    uint32_t simulation_start_time;
} client_manager_t;

static client_manager_t client_manager = {0};

void start_client_simulation(void) {
    ESP_LOGI(TAG, " Iniciando simulação de %d clientes", MAX_CLIENTS);
    
    client_manager.simulation_running = true;
    client_manager.simulation_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Criar e iniciar cada cliente
    for (int i = 0; i < MAX_CLIENTS; i++) {
        setup_client_profile(&client_manager.clients[i], i);
        
        char task_name[32];
        snprintf(task_name, sizeof(task_name), "client_%d", i);
        
        xTaskCreate(client_simulation_task, 
                   task_name, 
                   8192, 
                   &client_manager.clients[i], 
                   5, 
                   &client_manager.clients[i].task_handle);
        
        // Delay entre inicializações para evitar sobrecarga
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
    // Iniciar task de monitoramento
    xTaskCreate(monitor_clients_task, "monitor", 4096, NULL, 3, NULL);
}
```

### 2. Monitoramento de Clientes
```c
static void monitor_clients_task(void *pvParameters) {
    while (client_manager.simulation_running) {
        int connected_clients = 0;
        uint32_t total_traffic = 0;
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            client_config_t *client = &client_manager.clients[i];
            
            if (client->is_connected) {
                connected_clients++;
                total_traffic += client->bytes_transmitted + client->bytes_received;
            }
        }
        
        ESP_LOGI(TAG, " Clientes conectados: %d/%d, Tráfego total: %.1f MB", 
                 connected_clients, MAX_CLIENTS, total_traffic / (1024.0 * 1024.0));
        
        // Verificar se algum cliente precisa de manutenção
        maintain_client_connections();
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // Monitorar a cada 10 segundos
    }
}
```

## Configurações de Simulação

### 1. Parâmetros Principais
```c
#define MAX_CLIENTS 5                    // Número máximo de clientes simulados
#define TARGET_AP_SSID "ESP32_AP"        // SSID do AP alvo
#define TARGET_AP_PASS "12345678"        // Senha do AP
#define SIMULATION_DURATION_SEC 3600     // 1 hora de simulação
#define TRAFFIC_VARIANCE_PERCENT 30      // Variação no tráfego (±30%)
#define CONNECTION_RETRY_INTERVAL_SEC 30 // Tentar reconectar a cada 30s
```

### 2. Perfis de Tráfego
```c
typedef enum {
    TRAFFIC_PROFILE_LIGHT,      // Uso leve (IoT, básico)
    TRAFFIC_PROFILE_MODERATE,   // Uso moderado (navegação, e-mail)
    TRAFFIC_PROFILE_HEAVY,      // Uso pesado (streaming, downloads)
    TRAFFIC_PROFILE_BURSTY      // Tráfego em rajadas
} traffic_profile_t;
```

### 3. Configurações de Comportamento
```c
typedef struct {
    uint32_t idle_probability_percent;        // Probabilidade de estar idle
    uint32_t max_idle_duration_sec;          // Duração máxima idle
    uint32_t activity_burst_probability;     // Prob. de atividade intensa
    uint32_t connection_stability_percent;   // Estabilidade da conexão
} behavior_config_t;
```

## Como Executar

### 1. Configuração Básica
```bash
cd CLIENTS/

# Configurar número de clientes e comportamento
idf.py menuconfig
# → Component config → Client Simulation Config

# Compilar
idf.py build

# Flash em múltiplos ESP32s ou usar simulação single-device
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

### 2. Configuração Multi-Device
```bash
# Para simular clientes reais em múltiplos ESP32s
for port in /dev/ttyUSB{0,1,2,3,4}; do
    if [ -e "$port" ]; then
        echo "Flashing client to $port"
        idf.py -p "$port" flash &
    fi
done
wait
```

### 3. Modo de Operação
```
1. ESP32s iniciam e escaneiam AP alvo
2. Cada cliente conecta com perfil único
3. Simulação de tráfego inicia
4. Monitoramento contínuo de conexões
5. Reconexão automática se necessário
6. Logs detalhados de atividade
```

## Análise de Logs

### 1. Inicialização de Clientes
```
I (2000) CLIENTS:  Iniciando simulação de 5 clientes
I (2010) CLIENTS:  Iniciando cliente iPhone-12 (SMARTPHONE)
I (4010) CLIENTS:  Iniciando cliente MacBook-Pro (LAPTOP)
I (6010) CLIENTS:  Iniciando cliente SmartSensor-01 (IOT_DEVICE)
```

### 2. Conexões Estabelecidas
```
I (8000) CLIENTS:  Cliente iPhone-12 conectado com sucesso
I (8010) CLIENTS: 📍 IP atribuído: 192.168.4.100
I (10000) CLIENTS:  Cliente MacBook-Pro conectado com sucesso
I (10010) CLIENTS: 📍 IP atribuído: 192.168.4.101
```

### 3. Atividades Simuladas
```
D (15000) CLIENTS:  iPhone-12: Simulando acesso a www.youtube.com
D (15500) CLIENTS: 📺 MacBook-Pro: Iniciando streaming de vídeo (300 segundos)
D (16000) CLIENTS:  SmartSensor-01: Enviando dados de sensor
```

### 4. Monitoramento de Tráfego
```
I (30000) CLIENTS:  Clientes conectados: 5/5, Tráfego total: 45.2 MB
I (40000) CLIENTS:  iPhone-12: 15.3 MB tx, 8.7 MB rx
I (40010) CLIENTS:  MacBook-Pro: 2.1 MB tx, 35.8 MB rx (streaming)
I (40020) CLIENTS:  SmartSensor-01: 0.1 MB tx, 0.05 MB rx
```

### 5. Eventos de Reconexão
```
W (50000) CLIENTS:  Cliente MacBook-Pro desconectado
I (50010) CLIENTS:  Tentando reconectar MacBook-Pro...
I (52000) CLIENTS:  MacBook-Pro reconectado com sucesso
```

## Uso Como Alvos de Ataque

### 1. Vulnerabilidades Simuladas
- **Conexões não protegidas**: Sem PMF ou 802.1X
- **Tráfego não criptografado**: HTTP ao invés de HTTPS para alguns sites
- **Tabelas ARP vulneráveis**: Aceitar spoofing ARP
- **Reconexão automática**: Facilita ataques de deauth

### 2. Comportamento Sob Ataque
```c
void handle_attack_scenario(attack_type_t attack) {
    switch (attack) {
        case ATTACK_DEAUTH:
            // Simular desconexão e tentativa de reconexão
            simulate_disconnection_response();
            break;
            
        case ATTACK_ARP_SPOOF:
            // Continuar operação normalmente (vítima não percebe)
            continue_normal_operation();
            break;
            
        case ATTACK_EVIL_TWIN:
            // Possivelmente conectar ao AP malicioso
            if (should_connect_to_evil_twin()) {
                connect_to_evil_twin();
            }
            break;
    }
}
```

### 3. Logging de Impacto
```c
void log_attack_impact(void) {
    static uint32_t last_disconnection = 0;
    static uint32_t disconnection_count = 0;
    
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (!is_connected && (current_time - last_disconnection) > 5000) {
        disconnection_count++;
        last_disconnection = current_time;
        
        ESP_LOGI(TAG, " IMPACTO DE ATAQUE DETECTADO:");
        ESP_LOGI(TAG, "   Desconexão #%d", disconnection_count);
        ESP_LOGI(TAG, "   Tempo desde última desconexão: %d segundos", 
                 (current_time - last_disconnection) / 1000);
        ESP_LOGI(TAG, "   Possível ataque em andamento");
    }
}
```

## Configurações Avançadas

### 1. Simulação de Diferentes OS
```c
typedef struct {
    char os_name[32];
    char user_agent[128];
    uint32_t dhcp_behavior;
    uint32_t tcp_window_size;
    bool supports_pmf;
} os_profile_t;

os_profile_t os_profiles[] = {
    {"iOS 15", "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0)", DHCP_AGGRESSIVE, 65535, true},
    {"Android 12", "Mozilla/5.0 (Linux; Android 12)", DHCP_STANDARD, 32768, true},
    {"Windows 10", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)", DHCP_STANDARD, 65535, false},
    {"macOS", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)", DHCP_STANDARD, 65535, true}
};
```

### 2. Simulação de Aplicações Específicas
```c
void simulate_specific_apps(client_config_t *client) {
    // Simular aplicações baseadas no tipo de dispositivo
    switch (client->profile.device_type) {
        case CLIENT_TYPE_SMARTPHONE:
            simulate_whatsapp_usage();
            simulate_instagram_usage();
            simulate_spotify_streaming();
            break;
            
        case CLIENT_TYPE_LAPTOP:
            simulate_zoom_meeting();
            simulate_slack_usage();
            simulate_github_sync();
            break;
            
        case CLIENT_TYPE_IOT_DEVICE:
            simulate_mqtt_telemetry();
            simulate_ntp_sync();
            break;
    }
}
```

## Troubleshooting

### Problemas Comuns
1. **Conexões falham**: Verificar SSID e senha do AP
2. **Baixo tráfego**: Ajustar perfis de atividade
3. **Desconexões frequentes**: AP pode ter limitações
4. **Conflitos de MAC**: Verificar unicidade dos MACs

### Debug e Monitoramento
```bash
# Monitor de clientes específicos
idf.py monitor --print_filter "CLIENTS:*"

# Análise de tráfego
idf.py monitor | grep -E "(tx|rx|MB|KB)"

# Status de conexões
idf.py monitor | grep -E "(conectado|desconectado|IP)"
```
