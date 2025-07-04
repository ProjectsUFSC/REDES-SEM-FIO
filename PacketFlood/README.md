# Packet Flood Attack - ESP32

## Visão Geral

Este projeto implementa um **ataque de inundação de pacotes (Packet Flood)** usando ESP32, onde um cliente malicioso sobrecarrega o Access Point com alto volume de tráfego UDP/TCP em alta frequência.

## Objetivo do Ataque

O Packet Flood visa:
- **Sobrecarregar a largura de banda** da rede
- **Esgotar recursos** de processamento do AP
- **Degradar performance** para outros usuários
- **Causar negação de serviço** através de volume

## Como Funciona

### Estratégias de Inundação

1. **Conexão Legítima**: Conecta normalmente ao AP
2. **Múltiplas Conexões TCP**: Abre várias conexões simultâneas
3. **Envio Massivo**: Dispara pacotes em alta frequência
4. **Sobrecarga**: Satura recursos do servidor TCP

### Configurações do Ataque

```c
#define PACKET_FLOOD_INTERVAL_MS 10   // Intervalo entre pacotes (10ms)
#define MAX_FLOOD_PACKETS 1000        // Máximo de pacotes por sessão
#define CONCURRENT_CONNECTIONS 5      // Conexões TCP simultâneas
#define PACKET_SIZE 1024              // Tamanho do payload em bytes
```

## Detecção pelo AP

O AP detecta Packet Flood através de:

### Monitoramento de Taxa
- **Pacotes por segundo** por cliente (limite: 50 pps)
- **Múltiplas conexões TCP** do mesmo IP
- **Volume total de dados** transferidos
- **Padrões anômalos** de tráfego

### Indicadores de Detecção
```
PACKET FLOOD DETECTADO! 
Cliente 24:6f:28:aa:bb:cc enviou 75 pacotes em 1s
Limite excedido: 75 > 50 pps
MAC bloqueado por PACKET_FLOOD (60 seg)
```

## 🛠️ Como Usar

### 📋 Pré-requisitos
- ESP-IDF instalado e configurado
- ESP32 disponível
- AP alvo rodando (projeto `/AP`)

### 🔧 Configuração
1. **Ajustar configurações** em `PacketFlood.c`:
   ```c
   #define TARGET_SSID "ESP32_AP"        // SSID do AP alvo
   #define TARGET_PASS "12345678"        // Senha do AP
   #define PACKET_FLOOD_INTERVAL_MS 10   // Agressividade (10ms = 100 pps)
   ```

### 🚀 Compilação e Flash
```bash
cd PacketFlood
idf.py build
idf.py flash monitor
```

## 📊 Logs Esperados

### 🚀 Iniciando o Ataque
```
🚀 === INICIANDO PACKET FLOOD ATTACK ===
📡 Conectando ao AP: ESP32_AP
✅ Conectado! IP: 192.168.4.103
🎯 Servidor alvo: 192.168.4.1:3333
🔥 Iniciando flood com 5 conexões simultâneas...
```

### 💥 Durante o Flood
```
💥 Conexão TCP #1 estabelecida com servidor
📤 FLOOD #1 - Enviando pacote de 1024 bytes
📤 FLOOD #2 - Enviando pacote de 1024 bytes
💥 Conexão TCP #2 estabelecida com servidor
📤 FLOOD #3 - Multi-conexão ativa!
⚡ Taxa atual: 95 pacotes/segundo
🎯 Total enviado: 50 KB em 5 segundos
```

### 🚫 Detectado e Bloqueado
```
❌ Conexão TCP rejeitada pelo servidor!
🚨 Possível detecção - reduzindo taxa...
❌ Todas as conexões foram terminadas
⏹️ ATAQUE BLOQUEADO - Blacklist ativa
```

### 📈 Estatísticas Finais
```
📊 === ESTATÍSTICAS PACKET FLOOD ===
📤 Total de pacotes enviados: 847
💾 Volume total: 869 KB
⚡ Taxa média: 84.7 pacotes/segundo
🔌 Conexões simultâneas: 5
⏱️ Duração antes do bloqueio: 10 segundos
🎯 Performance: Ataque bem-sucedido até detecção
```

## 🛡️ Contramedidas do AP

### 🔍 Sistema de Detecção
1. **Rate Limiting**: Monitora pacotes por segundo por cliente
2. **Connection Limiting**: Limita conexões TCP por IP
3. **Blacklist Automática**: Bloqueia MACs maliciosos
4. **Throttling**: Reduz recursos para atacantes

### ⚙️ Configurações de Defesa
```c
#define MAX_PACKETS_PER_CLIENT 50       // 50 pacotes/segundo máximo
#define MAX_TCP_CONNECTIONS_PER_IP 3    // 3 conexões TCP por IP
#define BLACKLIST_DURATION_MS 60000     // 60 segundos de bloqueio
```

### 🛡️ Mitigação em Tempo Real
```
📊 Cliente 192.168.4.103: 75 pacotes/segundo
🚨 LIMITE EXCEDIDO! Iniciando contramedidas:
  1. ✅ MAC adicionado à blacklist
  2. ✅ Conexões TCP terminadas
  3. ✅ Futuras conexões bloqueadas
  4. ✅ Recursos liberados para outros clientes
```

## 🎓 Valor Educacional

### 📚 Conceitos Demonstrados
- **Ataques de Volume**: Sobrecarga através de quantidade
- **Rate Limiting**: Controle de taxa em tempo real
- **Resource Exhaustion**: Esgotamento de recursos do servidor
- **Network QoS**: Qualidade de serviço e priorização

### 🔍 Análise de Segurança
- **Simplicidade**: Fácil de implementar
- **Efetividade**: Pode degradar significativamente a rede
- **Detectabilidade**: Facilmente detectável com métricas adequadas
- **Mitigação**: Rate limiting é contramedida efetiva

### 💡 Tipos de Packet Flood
1. **UDP Flood**: Pacotes UDP em alta frequência
2. **TCP SYN Flood**: Exploração do handshake TCP
3. **HTTP Flood**: Requisições HTTP massivas
4. **ICMP Flood**: Ping em alta frequência

## 🌐 Ataques Similares no Mundo Real

### 💻 Ferramentas Profissionais
- **hping3**: Geração de pacotes customizados
- **iperf3**: Teste de largura de banda (pode ser usado maliciosamente)
- **LOIC**: Low Orbit Ion Cannon
- **Slowloris**: Ataque de conexões lentas

### 🔧 Comando Exemplo
```bash
# hping3 UDP flood (apenas para testes autorizados)
hping3 -2 -p 80 --flood 192.168.1.100

# iperf3 stress test
iperf3 -c 192.168.1.100 -t 60 -P 10
```

## 🛡️ Proteções Avançadas

### 🔧 Soluções de Rede
- **DDoS Protection**: Serviços especializados
- **Load Balancers**: Distribuição de carga
- **Rate Limiting**: Em múltiplas camadas
- **Traffic Shaping**: Controle de largura de banda

### 📊 Monitoramento
```python
# Exemplo de detecção (conceitual)
def detect_packet_flood(client_ip, packets_per_second):
    if packets_per_second > THRESHOLD:
        blacklist_client(client_ip)
        alert_administrator(f"Packet flood from {client_ip}")
        return True
    return False
```

### 🏗️ Arquitetura Resiliente
- **Microserviços**: Isolamento de componentes
- **Auto-scaling**: Escalonamento automático
- **Circuit Breakers**: Proteção contra cascata de falhas
- **Graceful Degradation**: Degradação controlada

## ⚠️ Diferenças da Implementação Real

### 🔧 Limitações do ESP32
- **Largura de banda**: Limitada comparada a ataques reais
- **Conexões simultâneas**: Menos que botnets
- **Persistência**: Sem coordenação distribuída
- **Sofisticação**: Padrões mais simples que ataques avançados

### 💪 Ataques DDoS Reais
- **Botnets**: Milhares de dispositivos coordenados
- **Amplificação**: DNS, NTP, memcached reflection
- **Volumetria**: Terabits por segundo
- **Duração**: Ataques sustentados por horas/dias

## ⚠️ Aviso Legal

Este código é destinado **exclusivamente para fins educacionais** e testes autorizados.

**NÃO** use este código:
- Para ataques DDoS reais
- Em infraestrutura de produção
- Contra redes de terceiros
- Sem autorização explícita

O uso inadequado pode violar:
- Leis de crimes cibernéticos
- Termos de serviço de provedores
- Regulamentações de telecomunicações
- Acordos de uso de rede

## 🔗 Projetos Relacionados

- **`/AP`** - Access Point com detecção de packet flood
- **`/CLIENTS`** - Clientes legítimos para comparação
- **`/ConnectFlood`** - Ataque de conexões
- **`/AuthFlood`** - Ataque de autenticação
- **`/DeauthFlood`** - Ataque de desconexões

---

📖 **Documentação completa**: `SISTEMA_SEGURANCA_WIFI.md`
