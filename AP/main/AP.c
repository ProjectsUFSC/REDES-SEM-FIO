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
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

// Configurações do AP
#define AP_SSID "ESP32_AP"
#define AP_PASS "12345678"
#define AP_CHANNEL 1
#define AP_MAX_STA_CONN 20 

// Configurações servidor TCP
#define TCP_SERVER_PORT 3333
#define TCP_KEEPALIVE_IDLE 5
#define TCP_KEEPALIVE_INTERVAL 5
#define TCP_KEEPALIVE_COUNT 3

#define MAX_DISCONNECTIONS_PER_SECOND 5
#define MAX_AUTH_ATTEMPTS_PER_SECOND 8
#define MAX_PACKETS_PER_CLIENT 30
#define BLACKLIST_DURATION_MS 300000
#define MAX_BLACKLIST_ENTRIES 10

typedef struct {
    uint8_t mac[6];
    uint32_t blocked_until;
    uint8_t attack_type; // 1=deauth, 2=auth, 3=packet
    bool active;
} blacklist_entry_t;

typedef struct {
    uint8_t mac[6];
    uint32_t last_packet_time;
    int packet_count;
    int tcp_connections;
    bool active;
} client_monitor_t;

typedef struct {
    uint32_t last_auth_time;
    int auth_attempts;
    int connections;
    int disconnections;
} security_stats_t;

static const char *TAG = "AP_MODE";

static int connected_clients = 0;
static int tcp_clients_served = 0;

static security_stats_t security_stats = {0};
static blacklist_entry_t blacklist[MAX_BLACKLIST_ENTRIES];
static client_monitor_t client_monitors[AP_MAX_STA_CONN];

static int deauth_floods_detected = 0;
static int auth_floods_detected = 0;
static int packet_floods_detected = 0;

bool is_mac_blacklisted(uint8_t* mac)
{
    /*
    @brief Verifica se um MAC address está na blacklist se o tempo expirou ele retira a blacklist.
    @param mac MAC address a ser verificado

    */
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    for (int i = 0; i < MAX_BLACKLIST_ENTRIES; i++) {
        if (blacklist[i].active) {
            if (current_time < blacklist[i].blocked_until) {
                if (memcmp(blacklist[i].mac, mac, 6) == 0) {
                    return true;
                }
            } else {
                blacklist[i].active = false;
                ESP_LOGI(TAG, "\n\n\nMAC %02x:%02x:%02x:%02x:%02x:%02x removido da blacklist (expirou)",
                         blacklist[i].mac[0], blacklist[i].mac[1], blacklist[i].mac[2],
                         blacklist[i].mac[3], blacklist[i].mac[4], blacklist[i].mac[5]);
            }
        }
    }
    return false;
}

void add_to_blacklist(uint8_t* mac, uint8_t attack_type)
{
    /*
    @brief Adiciona um MAC address à blacklist com o tipo de ataque e tempo de bloqueio.
    @param mac MAC address a ser adicionado
    @param attack_type Tipo de ataque (1=deauth, 2=auth, 3=packet)
    @note O tempo de bloqueio é fixo em BLACKLIST_DURATION
    */
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    for (int i = 0; i < MAX_BLACKLIST_ENTRIES; i++) {
        if (blacklist[i].active && memcmp(blacklist[i].mac, mac, 6) == 0) {
            // Atualizar tempo e tipo de ataque
            blacklist[i].blocked_until = current_time + BLACKLIST_DURATION_MS;
            blacklist[i].attack_type = attack_type;
            ESP_LOGI(TAG, "\n\n\nMAC ja bloqueado - tempo atualizado");
            return;
        }
    }
    
    for (int i = 0; i < MAX_BLACKLIST_ENTRIES; i++) {
        if (!blacklist[i].active) {
            memcpy(blacklist[i].mac, mac, 6);
            blacklist[i].blocked_until = current_time + BLACKLIST_DURATION_MS;
            blacklist[i].attack_type = attack_type;
            blacklist[i].active = true;
            
            const char* attack_names[] = {"UNKNOWN", "DEAUTH_FLOOD", "AUTH_FLOOD", "PACKET_FLOOD"};
            
            ESP_LOGI(TAG, "\n\n\nMAC %02x:%02x:%02x:%02x:%02x:%02x bloqueado por %s (%d seg)",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                     attack_names[attack_type], BLACKLIST_DURATION_MS/1000);
            
            // Desautenticar o cliente
            esp_wifi_deauth_sta(0);
            break;
        }
    }
}

bool detect_deauth_flood(uint8_t* mac)
{
    /*
    @brief Detecta flood de desconexões (ataque deauth).
    @param mac MAC address do cliente que está desconectando
    @return true se flood detectado, false caso contrário
    @note Utiliza um contador de desconexões por segundo. Se exceder o limite, considera flood.
    @note Reseta o contador se não houver desconexões por mais de 1 segundo
    */
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    static uint32_t last_disconnection_time = 0;
    static int disconnections_count = 0;
    
    // Resetar contador se passou mais de 1 segundo
    if (current_time - last_disconnection_time > 1000) {
        disconnections_count = 0;
        last_disconnection_time = current_time;
    }
    
    disconnections_count++;
    
    if (disconnections_count > MAX_DISCONNECTIONS_PER_SECOND) {
        ESP_LOGI(TAG, "\n\n\nDEAUTH FLOOD DETECTADO! %d desconexoes em 1s (limite: %d)", 
                 disconnections_count, MAX_DISCONNECTIONS_PER_SECOND);
        deauth_floods_detected++;
        return true;
    }
    
    return false;
}

bool detect_auth_flood(void)
{
    /*
    @brief Detecta Auth Flood ( ataque de autenticação)
    @note Utiliza um contador de tentativas de autenticação por segundo. Se exceder o limite, considera flood.
    @note Reseta o contador se não houver tentativas de autenticação por mais de 1 segundo
    */
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (current_time - security_stats.last_auth_time > 1000) {
        security_stats.auth_attempts = 0;
        security_stats.last_auth_time = current_time;
    }
    
    security_stats.auth_attempts++;
    
    if (security_stats.auth_attempts > MAX_AUTH_ATTEMPTS_PER_SECOND) {
        ESP_LOGI(TAG, "\n\n\nAUTH FLOOD DETECTADO! %d tentativas em 1s (limite: %d)", 
                 security_stats.auth_attempts, MAX_AUTH_ATTEMPTS_PER_SECOND);
        auth_floods_detected++;
        return true;
    }
    
    return false;
}

bool detect_packet_flood(uint8_t* mac)
{
    /* 
    @brief Detecta packet flood (ataque de inundação de pacotes).
    @note Utiliza um contador de pacotes por cliente por segundo. Se exceder o limite, considera flood.
    @note Reseta o contador se não houver pacotes do cliente por mais de 1 segundo
    @param mac MAC address do cliente a ser monitorado
    */
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    for (int i = 0; i < AP_MAX_STA_CONN; i++) {
        if (client_monitors[i].active && memcmp(client_monitors[i].mac, mac, 6) == 0) {
            if (current_time - client_monitors[i].last_packet_time > 1000) {
                client_monitors[i].packet_count = 0;
                client_monitors[i].last_packet_time = current_time;
            }
            
            client_monitors[i].packet_count++;
            
            if (client_monitors[i].packet_count > MAX_PACKETS_PER_CLIENT) {
                ESP_LOGI(TAG, "\n\n\nPACKET FLOOD DETECTADO! Cliente %02x:%02x:%02x:%02x:%02x:%02x enviou %d pacotes em 1s",
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                         client_monitors[i].packet_count);
                packet_floods_detected++;
                return true;
            }
            return false;
        }
    }
    
    for (int i = 0; i < AP_MAX_STA_CONN; i++) {
        if (!client_monitors[i].active) {
            memcpy(client_monitors[i].mac, mac, 6);
            client_monitors[i].last_packet_time = current_time;
            client_monitors[i].packet_count = 1;
            client_monitors[i].tcp_connections = 0;
            client_monitors[i].active = true;
            ESP_LOGD(TAG, "\n\n\nNovo cliente adicionado ao monitor: %02x:%02x:%02x:%02x:%02x:%02x",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            break;
        }
    }
    
    return false;
}

void remove_client_from_monitor(uint8_t* mac)
{
    /*
    @brief Remove um cliente do monitor de clientes.
    @param mac MAC address do cliente a ser removido
    @note Procura o cliente no monitor e desativa sua entrada se encontrado.
    */
    for (int i = 0; i < AP_MAX_STA_CONN; i++) {
        if (client_monitors[i].active && memcmp(client_monitors[i].mac, mac, 6) == 0) {
            client_monitors[i].active = false;
            ESP_LOGD(TAG, "\n\n\nCliente removido do monitor: %02x:%02x:%02x:%02x:%02x:%02x",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            break;
        }
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data){
    /* 
    @brief Event handler para eventos do Wi-Fi no modo Access Point.
    @note Trata eventos de conexão e desconexão de clientes, detectando tentativas de
    autenticação e desconexão em excesso (floods) e bloqueando MACs suspeitos.
    @note Utiliza funções auxiliares para detectar floods de autenticação, desconexão e pacotes.
    @param arg Argumento passado para o handler (não utilizado aqui)
    @param event_base Base do evento (WIFI_EVENT)
    @param event_id ID do evento (WIFI_EVENT_AP_STACONNECTED ou WIFI_EVENT_AP_STADISCONNECTED)
    @param event_data Dados do evento (wifi_event_ap_staconnected_t ou wifi_event_ap_stadisconnected_t)
    @note Registra eventos de conexão e desconexão de clientes, monitorando tentativas de ataque e mantendo um contador de clientes conectados.
    @note Se um cliente tentar se conectar e seu MAC estiver na blacklist, ele será desconectado imediatamente.
    */
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        
        if (is_mac_blacklisted(event->mac)) {
            ESP_LOGI(TAG, "\n\n\nTentativa de conexao de MAC bloqueado: %02x:%02x:%02x:%02x:%02x:%02x", 
                     event->mac[0], event->mac[1], event->mac[2], 
                     event->mac[3], event->mac[4], event->mac[5]);
            esp_wifi_deauth_sta(event->aid);
            return;
        }
        
        if (detect_auth_flood()) {
            ESP_LOGI(TAG, "\n\n\nAUTH FLOOD DETECTADO - Bloqueando atacante!");
            add_to_blacklist(event->mac, 2); // 2 = AUTH_FLOOD
            return;
        }
        
        connected_clients++;
        ESP_LOGI(TAG, "\n\n\nCliente conectado! MAC: %02x:%02x:%02x:%02x:%02x:%02x, AID: %d, Total: %d/%d", 
                 event->mac[0], event->mac[1], event->mac[2], 
                 event->mac[3], event->mac[4], event->mac[5],
                 event->aid, connected_clients, AP_MAX_STA_CONN);
                 
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        

        if (detect_deauth_flood(event->mac)) {
            ESP_LOGI(TAG, "\n\n\nDEAUTH FLOOD DETECTADO - Bloqueando atacante!");
            add_to_blacklist(event->mac, 1); // 1 = DEAUTH_FLOOD
        }
        
        remove_client_from_monitor(event->mac);
        
        // Proteger contra contador negativo
        if (connected_clients > 0) {
            connected_clients--;
            ESP_LOGI(TAG, "\n\n\nCliente desconectado! MAC: %02x:%02x:%02x:%02x:%02x:%02x, AID: %d, Motivo: %d, Total: %d/%d", 
                     event->mac[0], event->mac[1], event->mac[2], 
                     event->mac[3], event->mac[4], event->mac[5],
                     event->aid, event->reason, connected_clients, AP_MAX_STA_CONN);
        } else {
            ESP_LOGI(TAG, "\n\n\nEvento de desconexao duplicado ignorado! MAC: %02x:%02x:%02x:%02x:%02x:%02x", 
                     event->mac[0], event->mac[1], event->mac[2], 
                     event->mac[3], event->mac[4], event->mac[5]);
        }
    }
}

void show_ap_status(void)
{
    ESP_LOGI(TAG, "\n\n\n=== STATUS DO ACCESS POINT ===");
    ESP_LOGI(TAG, "SSID: %s", AP_SSID);
    ESP_LOGI(TAG, "Canal: %d", AP_CHANNEL);
    ESP_LOGI(TAG, "Clientes conectados: %d/%d", connected_clients, AP_MAX_STA_CONN);
    ESP_LOGI(TAG, "Servidor TCP: Porta %d", TCP_SERVER_PORT);
    ESP_LOGI(TAG, "Mensagens processadas: %d", tcp_clients_served);
    ESP_LOGI(TAG, "Autenticacao: WPA2_PSK");
    ESP_LOGI(TAG, "IP do AP: 192.168.4.1");
    ESP_LOGI(TAG, "Protecao anti-flood: ATIVA");
    ESP_LOGI(TAG, "==============================");
}

void wifi_init_ap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .password = AP_PASS,
            .max_connection = AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(AP_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Access Point iniciado!");
    ESP_LOGI(TAG, "SSID: %s", AP_SSID);
    ESP_LOGI(TAG, "Canal: %d", AP_CHANNEL);
    ESP_LOGI(TAG, "Máximo de conexões: %d", AP_MAX_STA_CONN);
    ESP_LOGI(TAG, "Autenticação: %s", (strlen(AP_PASS) == 0) ? "Aberta" : "WPA2_PSK");
}

void process_client_message(int sock, char* rx_buffer, int len, uint8_t* client_mac)
{
    tcp_clients_served++;
    
    if (detect_packet_flood(client_mac)) {
        ESP_LOGI(TAG, "\n\n\nPACKET FLOOD DETECTADO - Bloqueando cliente!");
        add_to_blacklist(client_mac, 3); // 3 = PACKET_FLOOD
        
        char block_response[] = "Connection blocked due to flood detection";
        send(sock, block_response, strlen(block_response), 0);
        return;
    }
    
    ESP_LOGI(TAG, "\n\n\nMensagem recebida (%d bytes): %s", len, rx_buffer);
    ESP_LOGI(TAG, "Total de mensagens processadas: %d", tcp_clients_served);
    
    char response[256];
    snprintf(response, sizeof(response), 
             "Echo from AP: %s | Messages: %d | Clients: %d | Security: ACTIVE",
             rx_buffer, tcp_clients_served, connected_clients);
    
    int to_write = strlen(response);
    while (to_write > 0) {
        int written = send(sock, response + (strlen(response) - to_write), to_write, 0);
        if (written < 0) {
            ESP_LOGE(TAG, "Erro ao enviar resposta: errno %d", errno);
            break;
        }
        to_write -= written;
    }
    
    ESP_LOGI(TAG, "\n\n\nResposta enviada: %s", response);
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = TCP_KEEPALIVE_IDLE;
    int keepInterval = TCP_KEEPALIVE_INTERVAL;
    int keepCount = TCP_KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(TCP_SERVER_PORT);
    ip_protocol = IPPROTO_IP;

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Não foi possível criar socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    ESP_LOGI(TAG, "\n\n\nServidor TCP criado na porta %d", TCP_SERVER_PORT);

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Erro no bind do socket: errno %d", errno);
        ESP_LOGE(TAG, "LWIP_SO_REUSEADDR não configurado corretamente");
        goto CLEAN_UP;
    }

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Erro no listen: errno %d", errno);
        goto CLEAN_UP;
    }

    ESP_LOGI(TAG, "\n\n\nServidor TCP ativo na porta %d - Aguardando conexoes...", TCP_SERVER_PORT);

    while (1) {
        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        
        if (sock < 0) {
            ESP_LOGE(TAG, "Erro no accept: errno %d", errno);
            break;
        }

        // Configurar keep-alive
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "\n\n\nNova conexao TCP de %s", addr_str);

        char rx_buffer[128];
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        
        if (len < 0) {
            ESP_LOGE(TAG, "Erro ao receber dados: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGI(TAG, "Conexão fechada pelo cliente");
        } else {
            rx_buffer[len] = 0; // Null-terminate
            

            uint8_t client_mac[6];
            uint32_t client_ip = ((struct sockaddr_in *)&source_addr)->sin_addr.s_addr;
            
            client_mac[0] = 0x02; 
            client_mac[1] = 0x00;
            client_mac[2] = (client_ip >> 24) & 0xFF;
            client_mac[3] = (client_ip >> 16) & 0xFF;  
            client_mac[4] = (client_ip >> 8) & 0xFF;
            client_mac[5] = client_ip & 0xFF;
            
            process_client_message(sock, rx_buffer, len, client_mac);
        }

        shutdown(sock, 0);
        close(sock);
        ESP_LOGI(TAG, "\n\n\nConexao TCP encerrada");
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

void show_advanced_security_stats(void)
{
    ESP_LOGI(TAG, "\n\n\n=== RELATORIO DE SEGURANCA AVANCADO ===");
    ESP_LOGI(TAG, "ATAQUES DETECTADOS:");
    ESP_LOGI(TAG, "  Deauth Floods: %d", deauth_floods_detected);
    ESP_LOGI(TAG, "  Auth Floods: %d", auth_floods_detected);
    ESP_LOGI(TAG, "  Packet Floods: %d", packet_floods_detected);
    
    int total_attacks = deauth_floods_detected + auth_floods_detected + packet_floods_detected;
    ESP_LOGI(TAG, "TOTAL DE ATAQUES: %d", total_attacks);
    
    int active_blacklist = 0;
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    for (int i = 0; i < MAX_BLACKLIST_ENTRIES; i++) {
        if (blacklist[i].active && current_time < blacklist[i].blocked_until) {
            active_blacklist++;
        }
    }
    
    ESP_LOGI(TAG, "MACs bloqueados: %d/%d", active_blacklist, MAX_BLACKLIST_ENTRIES);
    
    int active_monitors = 0;
    for (int i = 0; i < AP_MAX_STA_CONN; i++) {
        if (client_monitors[i].active) {
            active_monitors++;
        }
    }
    
    ESP_LOGI(TAG, "Clientes monitorados: %d/%d", active_monitors, AP_MAX_STA_CONN);
    ESP_LOGI(TAG, "Clientes conectados: %d/%d", connected_clients, AP_MAX_STA_CONN);
    ESP_LOGI(TAG, "Mensagens TCP processadas: %d", tcp_clients_served);
    
    if (total_attacks == 0) {
        ESP_LOGI(TAG, "\n\n\nSTATUS: REDE SEGURA - Nenhum ataque detectado");
    } else if (total_attacks < 5) {
        ESP_LOGI(TAG, "\n\n\nSTATUS: REDE SOB ATAQUES LEVES");
    } else if (total_attacks < 20) {
        ESP_LOGI(TAG, "\n\n\nSTATUS: REDE SOB ATAQUES MODERADOS");
    } else {
        ESP_LOGI(TAG, "\n\n\nSTATUS: REDE SOB ATAQUES INTENSOS!");
    }
    
    ESP_LOGI(TAG, "================================================");
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Inicializando ESP32 como Access Point...");
    
    wifi_init_ap();
    
    show_ap_status();
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    xTaskCreate(tcp_server_task, "tcp_server_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "\n\n\nServidor TCP iniciado!");
    
    while(1) {
        ESP_LOGI(TAG, "\n\n\nAccess Point ativo - aguardando conexoes...");
        show_ap_status();
        show_advanced_security_stats(); // Mostrar relatório de segurança 
        vTaskDelay(pdMS_TO_TICKS(15000)); // Log a cada 15 segundos para monitorar ataques
    }
}
