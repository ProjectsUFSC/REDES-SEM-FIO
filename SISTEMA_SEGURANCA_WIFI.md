# Sistema de Detecção e Mitigação de Ataques Wi-Fi

## Visão Geral

Este sistema implementa detecção e mitigação de ataques de flooding em redes Wi-Fi ESP32, incluindo três tipos principais de vulnerabilidades: Auth Flood, Deauth Flood e Packet Flood.

## Detecção de Ataques

### Métricas Monitoradas

- **Taxa de Desconexões**: Monitora desconexões por segundo (Deauth Flood)
- **Taxa de Autenticação**: Monitora tentativas de auth por segundo (Auth Flood)  
- **Taxa de Pacotes**: Monitora pacotes por segundo por cliente (Packet Flood)
- **Limites Configuráveis**: 8 desconexões/s, 10 auth/s, 50 pacotes/s
- **Análise Temporal**: Janela de 1 segundo para contagem
- **Identificação de MAC**: Rastreamento por endereço MAC

### Indicadores de Ataque

```
ATAQUE DETECTADO! 12 desconexões em 1 segundo (limite: 8)
MAC suspeito: 02:aa:bb:cc:dd:ee
AUTH FLOOD DETECTADO! 15 tentativas em 1 segundo (limite: 10)
PACKET FLOOD DETECTADO! 65 pacotes em 1 segundo (limite: 50)
```

## Sistema de Mitigação

### Blacklist Automática

- **Duração**: 60 segundos (configurável)
- **Capacidade**: Até 20 MACs simultâneos
- **Auto-Expiração**: Remove automaticamente após timeout

### Contramedidas Ativas

1. **Desautenticação Imediata**: `esp_wifi_deauth_sta()`
2. **Bloqueio Temporário**: Adiciona MAC à blacklist
3. **Rejeição de Reconexões**: Verifica blacklist em novas tentativas
4. **Rate Limiting**: Limita tentativas por segundo

## Melhorias Implementadas

### Configurações Aprimoradas

- **Conexões Simultâneas**: Aumentado de 4 para 20 clientes
- **Logging Detalhado**: MAC, AID, motivo de desconexão
- **Estatísticas de Segurança**: Relatórios em tempo real
- **Múltiplos Tipos de Ataque**: Suporte a Auth, Deauth e Packet Flood

### Monitoramento Avançado

```c
// Configurações de segurança
#define DEAUTH_FLOOD_THRESHOLD 8        // Limite de detecção para deauth
#define AUTH_FLOOD_THRESHOLD 10         // Limite de detecção para auth
#define PACKET_FLOOD_THRESHOLD 50       // Limite de detecção para packets
#define BLACKLIST_DURATION_MS 60000     // 60 segundos de bloqueio
#define MAX_BLACKLIST_ENTRIES 20        // Capacidade da blacklist
#define AP_MAX_STA_CONN 20             // Clientes simultâneos
```

## Clientes Maliciosos

### Características dos Ataques

#### DeauthFlood
- **Intervalos Agressivos**: 100ms entre tentativas
- **Desconexão Automática**: Libera slots para continuar o ataque
- **Estatísticas em Tempo Real**: Taxa de sucesso/falha

#### AuthFlood  
- **Tentativas Massivas**: Múltiplas tentativas de autenticação
- **Credenciais Falsas**: Usa senhas inválidas
- **Rate High**: Alto número de tentativas por segundo

#### PacketFlood
- **Alto Volume**: Gera tráfego UDP intenso
- **Saturação**: Objetivo de esgotar largura de banda
- **Monitoramento**: Pacotes por segundo por cliente

### Parâmetros dos Ataques

```c
#define FLOOD_INTERVAL_MS 100        // Muito agressivo
#define MAX_FLOOD_ATTEMPTS 1000      // 1000 tentativas
#define PACKET_SIZE 1024             // Tamanho do pacote UDP
#define PACKETS_PER_BURST 100        // Rajadas de pacotes
```

## Como Testar

### 1. Compilar e Executar AP

```bash
cd AP
idf.py build flash monitor
```

### 2. Executar Cliente Normal

```bash
cd CLIENTS  
idf.py build flash monitor
```

### 3. Executar Ataques de Flood

```bash
# Deauth Flood
cd DeauthFlood
idf.py build flash monitor

# Auth Flood
cd AuthFlood
idf.py build flash monitor

# Packet Flood
cd PacketFlood
idf.py build flash monitor
```

## Logs Esperados

### AP Detectando Ataque

```
I (12345) AP_MODE: Cliente conectado! MAC: 02:aa:bb:cc:dd:ee, AID: 1, Total: 1/20
I (12346) AP_MODE: Cliente conectado! MAC: 02:11:22:33:44:55, AID: 2, Total: 2/20
I (12347) AP_MODE: DEAUTH FLOOD DETECTADO!
I (12347) AP_MODE: MAC 02:66:77:88:99:aa adicionado à blacklist por 60 segundos
I (12350) AP_MODE: Tentativa de conexão de MAC bloqueado: 02:66:77:88:99:aa
I (12355) AP_MODE: AUTH FLOOD DETECTADO!
I (12356) AP_MODE: PACKET FLOOD DETECTADO!
```

### Cliente Malicioso Atacando

```
I (5678) DEAUTH_FLOOD: ATAQUE DE DEAUTH - Tentativa #15/1000
I (5678) DEAUTH_FLOOD: Conectando para desautenticar...
I (5680) DEAUTH_FLOOD: Conexão falhada/rejeitada - Tentativa #15
I (5690) DEAUTH_FLOOD: Taxa de sucesso: 12.5%

I (6678) AUTH_FLOOD: ATAQUE DE AUTH - Tentativa #25/1000
I (6678) AUTH_FLOOD: Tentando autenticar com senha falsa...
I (6680) AUTH_FLOOD: Autenticação rejeitada - Tentativa #25

I (7678) PACKET_FLOOD: ATAQUE DE PACKET - Enviando rajada #10
I (7678) PACKET_FLOOD: Enviados 100 pacotes UDP de 1024 bytes
I (7680) PACKET_FLOOD: Taxa: 65 pacotes/segundo
```

## Configurações Avançadas

### Parâmetros Ajustáveis

```c
// Sensibilidade da detecção
#define DEAUTH_FLOOD_THRESHOLD 5        // Mais sensível
#define DEAUTH_FLOOD_THRESHOLD 15       // Menos sensível

#define AUTH_FLOOD_THRESHOLD 5          // Mais sensível
#define AUTH_FLOOD_THRESHOLD 20         // Menos sensível

#define PACKET_FLOOD_THRESHOLD 30       // Mais sensível
#define PACKET_FLOOD_THRESHOLD 100      // Menos sensível

// Duração da punição
#define BLACKLIST_DURATION_MS 120000    // 2 minutos
#define BLACKLIST_DURATION_MS 30000     // 30 segundos

// Capacidade da rede
#define AP_MAX_STA_CONN 50              // Rede grande
#define AP_MAX_STA_CONN 5               // Rede pequena
```

## Considerações de Segurança

### Vantagens

- **Detecção Automática**: Sem intervenção manual
- **Resposta Rápida**: Bloqueio em tempo real
- **Baixo Impacto**: Não afeta clientes legítimos
- **Configurável**: Adaptável a diferentes cenários
- **Múltiplos Ataques**: Detecta Auth, Deauth e Packet Flood

### Limitações

- **Falsos Positivos**: Muitos clientes legítimos podem disparar limites
- **Bypass com MAC Spoofing**: Atacante pode usar MACs diferentes
- **Recursos Limitados**: Blacklist tem capacidade máxima
- **Tempo de Recuperação**: MACs bloqueados ficam inacessíveis temporariamente
- **Ataques Sofisticados**: Pode não detectar ataques muito lentos

## Casos de Uso Educacionais

### Aprendizado

- **Vulnerabilidades Wi-Fi**: Demonstração prática de ataques de flooding
- **Sistemas de Defesa**: Implementação de contramedidas
- **Análise de Tráfego**: Monitoramento de padrões de rede
- **Segurança de IoT**: Proteção de dispositivos ESP32

### Experimentos

1. **Teste de Limites**: Ajustar sensibilidade da detecção
2. **Análise de Performance**: Impacto na performance da rede
3. **Comparação de Ataques**: Diferentes estratégias maliciosas
4. **Otimização de Defesas**: Melhorar algoritmos de detecção

## Próximos Passos

- **Machine Learning**: Detecção mais inteligente de padrões
- **Logging Remoto**: Envio de logs para servidor central
- **Dashboard Web**: Interface visual para monitoramento
- **Integração com SIEM**: Correlação com outros eventos de segurança

---

**AVISO**: Este código é para fins educacionais e de teste. Use apenas em redes próprias ou com autorização explícita.
