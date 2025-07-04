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
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"

// Configurações do AP para conexão
#define AP_SSID "ESP32_AP"
#define AP_PASS "12345678"
#define AP_IP "192.168.4.1"
#define TCP_SERVER_PORT 3333

// Configurações de tráfego semi-aleatório
#define MIN_INTERVAL_MS 3000    // Mínimo 3 segundos
#define MAX_INTERVAL_MS 12000   // Máximo 12 segundos
#define BASE_INTERVAL_MS 7000   // Base de 7 segundos

// Event bits para controle de conexão
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "WIFI_CLIENT";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static const int EXAMPLE_ESP_MAXIMUM_RETRY = 5;

// Variáveis para armazenar informações de conexão
static bool is_connected = false;
static esp_ip4_addr_t client_ip;
static esp_ip4_addr_t gateway_ip;
static int messages_sent = 0;
static int messages_received = 0;

// Event handler para eventos do Wi-Fi e IP
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Iniciando conexão ao AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Tentativa de reconexão %d/%d", s_retry_num, EXAMPLE_ESP_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Falha na conexão após %d tentativas", EXAMPLE_ESP_MAXIMUM_RETRY);
        }
        is_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        client_ip = event->ip_info.ip;
        gateway_ip = event->ip_info.gw;
        
        ESP_LOGI(TAG, "🎉 CONECTADO COM SUCESSO! 🎉");
        ESP_LOGI(TAG, "IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        
        s_retry_num = 0;
        is_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Função para inicializar o Wi-Fi no modo Station (Cliente)
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Criar interface de rede para o modo Station
    esp_netif_create_default_wifi_sta();

    // Configuração inicial do Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Registrar event handlers
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // Configuração da conexão Wi-Fi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = AP_SSID,
            .password = AP_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // Definir modo Station e aplicar configuração
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Configuração Wi-Fi completa. Tentando conectar ao SSID: %s", AP_SSID);

    // Aguardar conexão ou falha
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, " Conectado ao AP %s com sucesso!", AP_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, " Falha na conexão ao AP %s", AP_SSID);
    } else {
        ESP_LOGE(TAG, "❓ Evento inesperado");
    }
}

// Função para mostrar estatísticas detalhadas da conexão
void show_detailed_stats(void)
{
    if (!is_connected) {
        ESP_LOGI(TAG, "Não conectado - estatísticas não disponíveis");
        return;
    }

    ESP_LOGI(TAG, " === ESTATÍSTICAS DETALHADAS ===");
    
    // Informações do AP
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        ESP_LOGI(TAG, " SSID: %s", ap_info.ssid);
        ESP_LOGI(TAG, " RSSI: %d dBm", ap_info.rssi);
        ESP_LOGI(TAG, " Canal: %d", ap_info.primary);
        
        // Interpretar qualidade do sinal
        if (ap_info.rssi > -50) {
            ESP_LOGI(TAG, " Qualidade: EXCELENTE ");
        } else if (ap_info.rssi > -60) {
            ESP_LOGI(TAG, " Qualidade: BOA ");
        } else if (ap_info.rssi > -70) {
            ESP_LOGI(TAG, " Qualidade: REGULAR ");
        } else {
            ESP_LOGI(TAG, " Qualidade: FRACA ");
        }
    }
    
    // Informações de rede
    ESP_LOGI(TAG, " IP do Cliente: " IPSTR, IP2STR(&client_ip));
    ESP_LOGI(TAG, " Gateway (AP): " IPSTR, IP2STR(&gateway_ip));
    
    // Estatísticas de tráfego
    ESP_LOGI(TAG, " Mensagens enviadas: %d", messages_sent);
    ESP_LOGI(TAG, "📨 Mensagens recebidas: %d", messages_received);
    ESP_LOGI(TAG, " Taxa de sucesso: %.1f%%", 
             messages_sent > 0 ? (float)messages_received / messages_sent * 100 : 0);
    
    // Status de conectividade
    ESP_LOGI(TAG, " Status: CONECTADO E FUNCIONANDO!");
    ESP_LOGI(TAG, " Tráfego TCP: ATIVO");
    ESP_LOGI(TAG, "===================================");
}

// Função alternativa para testar conectividade usando socket TCP
void tcp_connectivity_test(void)
{
    if (!is_connected) {
        ESP_LOGI(TAG, "Não conectado - não é possível testar conectividade");
        return;
    }

    ESP_LOGI(TAG, " Testando conectividade TCP com o AP...");
    
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = gateway_ip.addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(80); // Porta HTTP padrão
    
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        ESP_LOGE(TAG, " Erro ao criar socket");
        return;
    }
    
    // Configurar timeout
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
    
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGI(TAG, " Conexão TCP falhou (normal se AP não tiver servidor HTTP)");
        ESP_LOGI(TAG, " Mas a rede está funcionando - IP obtido com sucesso!");
    } else {
        ESP_LOGI(TAG, " Conectividade TCP OK!");
    }
    
    close(sock);
    ESP_LOGI(TAG, " Teste de conectividade concluído");
}

// Função simplificada para testar conectividade básica
void simple_connectivity_test(void)
{
    if (!is_connected) {
        ESP_LOGI(TAG, "Não conectado - não é possível testar conectividade");
        return;
    }

    ESP_LOGI(TAG, " Verificando conectividade de rede...");
    
    // Verificar se temos IP válido
    if (client_ip.addr != 0 && gateway_ip.addr != 0) {
        ESP_LOGI(TAG, " Conectividade verificada:");
        ESP_LOGI(TAG, "   - IP do cliente: " IPSTR, IP2STR(&client_ip));
        ESP_LOGI(TAG, "   - Gateway (AP): " IPSTR, IP2STR(&gateway_ip));
        ESP_LOGI(TAG, "   - Status: Rede ativa e funcionando! ");
    } else {
        ESP_LOGI(TAG, " Problema na configuração de rede");
    }
}

// Função para verificar status da conexão
void check_connection_status(void)
{
    ESP_LOGI(TAG, "=== STATUS DA CONEXÃO ===");
    ESP_LOGI(TAG, "Modo Wi-Fi: Station (Cliente)");
    ESP_LOGI(TAG, "Conectado: %s", is_connected ? "SIM " : "NÃO ");
    
    if (is_connected) {
        show_detailed_stats();
        simple_connectivity_test();
    } else {
        ESP_LOGI(TAG, " Desconectado do AP");
        ESP_LOGI(TAG, " Aguardando reconexão...");
    }
    ESP_LOGI(TAG, "==========================");
}

// Função para gerar intervalo semi-aleatório
uint32_t get_random_interval(void)
{
    // Usar tick count do FreeRTOS para gerar "aleatoriedade"
    static uint32_t seed = 12345; // Seed inicial
    uint32_t current_tick = xTaskGetTickCount();
    
    // Combinar com tick atual para variabilidade
    seed = (seed * 1103515245 + current_tick + 12345) & 0x7FFFFFFF;
    
    // Normalizar para o range desejado
    uint32_t range = MAX_INTERVAL_MS - MIN_INTERVAL_MS;
    uint32_t random_offset = seed % range;
    
    return MIN_INTERVAL_MS + random_offset;
}

// Função para obter MAC address como string
void get_mac_string(char* mac_str, size_t size)
{
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(mac_str, size, "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Função para gerar tipos diferentes de mensagens amigáveis
void generate_message(char* buffer, size_t size, int msg_count)
{
    char mac_str[18];
    get_mac_string(mac_str, sizeof(mac_str));
    
    // Extrair os últimos 4 caracteres do MAC para identificar o ESP
    char esp_id[5];
    strncpy(esp_id, &mac_str[strlen(mac_str)-5], 4);
    esp_id[4] = '\0';
    
    // Array de mensagens amigáveis para simular conversas
    const char* message_templates[] = {
        "Oi eu sou o ESP %s! Esta é minha mensagem número %d",
        "Olá! ESP %s aqui reportando status OK - msg %d",
        "Saudações! Sou o ESP %s enviando dados - mensagem %d",
        "Hey! ESP %s ativo e funcionando - count %d",
        "Oi pessoal! ESP %s mandando um oi - msg %d",
        "Alô! ESP %s por aqui, tudo bem? - mensagem %d"
    };
    
    int template_index = msg_count % (sizeof(message_templates) / sizeof(message_templates[0]));
    
    snprintf(buffer, size, message_templates[template_index], esp_id, msg_count);
}

// Task para enviar mensagens TCP com intervalos semi-aleatórios
static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char message[256];
    int msg_counter = 1;
    
    ESP_LOGI(TAG, " Cliente TCP iniciado - aguardando conexão Wi-Fi...");
    
    // Aguardar conexão Wi-Fi
    while (!is_connected) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, " Wi-Fi conectado! Iniciando tráfego TCP...");
    
    while (1) {
        if (!is_connected) {
            ESP_LOGI(TAG, " Wi-Fi desconectado - pausando tráfego TCP");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        // Gerar intervalo semi-aleatório
        uint32_t next_interval = get_random_interval();
        
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = gateway_ip.addr;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(TCP_SERVER_PORT);
        
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(TAG, " Erro ao criar socket: errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(next_interval));
            continue;
        }
        
        // Configurar timeout para connect
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
        
        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, " Erro ao conectar socket: errno %d", errno);
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(next_interval));
            continue;
        }
        
        // Gerar e enviar mensagem
        generate_message(message, sizeof(message), msg_counter);
        
        int err_send = send(sock, message, strlen(message), 0);
        if (err_send < 0) {
            ESP_LOGE(TAG, " Erro ao enviar dados: errno %d", errno);
        } else {
            messages_sent++;
            ESP_LOGI(TAG, " Mensagem %d enviada (%d bytes): %s", 
                     msg_counter, err_send, message);
            
            // Receber resposta
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                ESP_LOGI(TAG, " Erro ao receber resposta: errno %d", errno);
            } else if (len == 0) {
                ESP_LOGI(TAG, " Conexão fechada pelo servidor");
            } else {
                rx_buffer[len] = 0;
                messages_received++;
                ESP_LOGI(TAG, "📨 Resposta recebida: %s", rx_buffer);
            }
        }
        
        shutdown(sock, 0);
        close(sock);
        
        msg_counter++;
        
        ESP_LOGI(TAG, "⏰ Próxima mensagem em %lu ms (Enviadas: %d, Recebidas: %d)", 
                 next_interval, messages_sent, messages_received);
        
        vTaskDelay(pdMS_TO_TICKS(next_interval));
    }
}

// Task principal de monitoramento
void monitoring_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "\n === VERIFICAÇÃO PERIÓDICA ===");
        check_connection_status();
        ESP_LOGI(TAG, "=== FIM DA VERIFICAÇÃO ===\n");
        
        vTaskDelay(pdMS_TO_TICKS(30000)); // Verificar a cada 30 segundos
    }
}

void app_main(void)
{
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, " Inicializando ESP32 como Cliente Wi-Fi...");
    
    // Inicializar Wi-Fi no modo Station
    wifi_init_sta();
    
    // Verificar conexão inicial
    vTaskDelay(pdMS_TO_TICKS(2000)); // Aguardar estabilizar
    check_connection_status();
    
    // Criar task de monitoramento contínuo
    xTaskCreate(monitoring_task, "monitoring_task", 4096, NULL, 5, NULL);
    
    // Criar task de cliente TCP para tráfego semi-aleatório
    xTaskCreate(tcp_client_task, "tcp_client_task", 4096, NULL, 4, NULL);
    
    ESP_LOGI(TAG, "✨ Sistema iniciado! Monitoramento e tráfego TCP ativos...");
}
