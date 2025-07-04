# Etapa 2: Simulação de Tráfego de Rede Normal

## Objetivo Implementado

Criar tráfego de rede **semi-aleatório e realista** entre clientes ESP32 e o AP para estabelecer uma baseline de comportamento "normal" que será usada posteriormente para detectar anomalias.

## Arquitetura Implementada

### **Access Point (Servidor)**
- **Servidor TCP na porta 3333**
- **Echo server** que processa mensagens dos clientes
- **Estatísticas de tráfego** em tempo real
- **Logs detalhados** de cada conexão

### **Clientes (Geradores de Tráfego)**
- **Cliente TCP com intervalos semi-aleatórios**
- **Múltiplos tipos de mensagens** simulando atividades reais
- **Estatísticas de comunicação**
- **Reconexão automática** em caso de problemas

## Algoritmo de Tráfego Semi-Aleatório

### **Intervalos Dinâmicos:**
```c
#define MIN_INTERVAL_MS 3000    // Mínimo 3 segundos
#define MAX_INTERVAL_MS 12000   // Máximo 12 segundos
#define BASE_INTERVAL_MS 7000   // Base de 7 segundos
```

### **Geração de Aleatoriedade:**
```c
uint32_t get_random_interval(void) {
    uint32_t seed = esp_timer_get_time() & 0xFFFFFFFF;
    seed = seed * 1103515245 + 12345;  // LCG Algorithm
    uint32_t range = MAX_INTERVAL_MS - MIN_INTERVAL_MS;
    return MIN_INTERVAL_MS + (seed % range);
}
```

**Resultado:** Intervalos variam entre 3-12 segundos de forma **pseudo-aleatória**

## Tipos de Mensagens Simuladas

### **Padrões de Tráfego Realistas:**
1. **STATUS_UPDATE** - Atualizações de status do dispositivo
2. **HEARTBEAT** - Sinais de vida periódicos
3. **DATA_REQUEST** - Solicitações de dados
4. **SENSOR_DATA** - Dados de sensores simulados
5. **KEEP_ALIVE** - Manutenção de conexão
6. **SYNC_REQUEST** - Sincronização de relógio

### **Formato das Mensagens:**
```
MAC_ADDRESS|MESSAGE_TYPE|MSG_COUNT|RSSI_VALUE|TIMESTAMP
```

**Exemplo:**
```
A4:CF:12:34:56:78|HEARTBEAT|MSG_15|RSSI_-42|TIME_1234567890
```

## 🔧 Funcionalidades Implementadas

### **No Access Point:**
- ✅ **Servidor TCP robusto** com keep-alive
- ✅ **Processamento de mensagens** com echo
- ✅ **Contador de clientes atendidos**
- ✅ **Logs detalhados** de cada transação
- ✅ **Estatísticas em tempo real**

### **Nos Clientes:**
- ✅ **Tráfego semi-aleatório** (3-12 segundos)
- ✅ **6 tipos diferentes** de mensagens
- ✅ **MAC address único** em cada mensagem
- ✅ **Contador de mensagens** enviadas/recebidas
- ✅ **Taxa de sucesso** calculada
- ✅ **Reconexão automática** se perder Wi-Fi

## 📊 Logs e Monitoramento

### **Access Point Logs:**
```
🌐 Servidor TCP ativo na porta 3333 - Aguardando conexões...
🔗 Nova conexão TCP de 192.168.4.2
📨 Mensagem recebida (45 bytes): A4:CF:12:34:56:78|HEARTBEAT|MSG_15|RSSI_-42|TIME_1234567890
📊 Total de mensagens processadas: 15
📤 Resposta enviada: Echo from AP: ... | Messages served: 15 | Clients online: 2
🔌 Conexão TCP encerrada
```

### **Cliente Logs:**
```
📤 Mensagem 15 enviada (45 bytes): A4:CF:12:34:56:78|HEARTBEAT|MSG_15|RSSI_-42|TIME_1234567890
📨 Resposta recebida: Echo from AP: ... | Messages served: 15 | Clients online: 2
⏰ Próxima mensagem em 8534 ms (Enviadas: 15, Recebidas: 15)

📊 === ESTATÍSTICAS DETALHADAS ===
📤 Mensagens enviadas: 15
📨 Mensagens recebidas: 15
📊 Taxa de sucesso: 100.0%
✅ Tráfego TCP: ATIVO
```

## 🎯 Características do Tráfego Normal

### **Padrões Esperados:**
- **Intervalos:** 3-12 segundos (média ~7.5s)
- **Taxa de sucesso:** >95% em condições normais
- **Tipos de mensagem:** Distribuição uniforme entre os 6 tipos
- **Tamanho:** 40-60 bytes por mensagem
- **Latência:** <100ms para echo response

### **Indicadores de Saúde da Rede:**
- ✅ **Conexões TCP bem-sucedidas**
- ✅ **Respostas do servidor recebidas**
- ✅ **RSSI estável** (variação <10dBm)
- ✅ **Sem timeouts** de socket
- ✅ **Reconexões mínimas**

## 🚀 Como Executar

### **1. Compilar e executar AP:**
```bash
cd AP
idf.py build flash monitor
```

### **2. Compilar e executar Clientes:**
```bash
cd CLIENTS  
idf.py build flash monitor
```

### **3. Observar tráfego:**
- **AP:** Mostra conexões e mensagens processadas
- **Clientes:** Mostram intervalos e estatísticas
- **Tráfego:** Varia automaticamente a cada 3-12 segundos

## 📈 Próximos Passos (Etapa 3)

Com o tráfego normal estabelecido, poderemos implementar:

1. **📊 Análise de padrões** - Baseline de comportamento normal
2. **🚨 Detecção de anomalias** - Desvios do padrão estabelecido
3. **📝 Sistema de auditoria** - Logs de eventos suspeitos
4. **⚡ Alertas em tempo real** - Notificações de comportamento anômalo

## ✨ Resultado

**Tráfego de rede realista e variável foi estabelecido com sucesso!** 

- 🎲 **Semi-aleatório:** Intervalos variam naturalmente
- 🔄 **Sustentável:** Funciona continuamente
- 📊 **Monitorável:** Estatísticas completas
- 🛡️ **Robusto:** Resiliente a falhas de rede

**Base sólida para sistema de auditoria criada!** 🎉
