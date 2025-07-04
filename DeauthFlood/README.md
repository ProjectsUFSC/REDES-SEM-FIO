# Deauth Flood Attack - ESP32

## Visão Geral

Este projeto implementa um **ataque de inundação de desautenticação (Deauth Flood)** usando ESP32, onde um cliente malicioso se desconecta e reconecta repetidamente para simular um ataque de deauth/disassociation flood.

## Objetivo do Ataque

O Deauth Flood visa:
- **Simular desautenticações forçadas** em alta frequência
- **Sobrecarregar o sistema de eventos** do Access Point
- **Degradar performance** através de overhead de processamento
- **Testar resiliência** do AP contra desconexões massivas

## Como Funciona

### Ciclo do Ataque

1. **Conectar**: Estabelece conexão legítima com o AP
2. **Aguardar**: Mantém conexão por período mínimo
3. **Desconectar**: Força desconexão abrupta
4. **Repetir**: Imediatamente tenta reconectar
5. **Loop**: Mantém o ciclo em alta frequência

### Configurações de Ataque

```c
#define DEAUTH_INTERVAL_MS 50      // Intervalo entre desconexões (50ms)
#define CONNECTION_HOLD_MS 100     // Tempo mínimo conectado (100ms)
#define MAX_DEAUTH_CYCLES 500      // Máximo de 500 ciclos
```

## Detecção pelo AP

O AP detecta este ataque monitorando:

### Métricas Observadas
- **Taxa de desconexões por segundo** (limite: 8 desconexões/s)
- **Padrões repetitivos** de mesmo MAC
- **Ciclos anômalos** de conectar/desconectar

### Indicadores de Detecção
```
DEAUTH FLOOD DETECTADO! 12 desconexões em 1s (limite: 8)
MAC suspeito: 24:6f:28:aa:bb:cc
MAC bloqueado por DEAUTH_FLOOD (60 seg)
```

## Como Usar

### 📋 Pré-requisitos
- ESP-IDF instalado e configurado
- ESP32 disponível
- AP alvo rodando (projeto `/AP`)

### 🔧 Configuração
1. **Ajustar configurações** em `DeauthFlood.c`:
   ```c
   #define TARGET_SSID "ESP32_AP"      // SSID do AP alvo
   #define TARGET_PASS "12345678"      // Senha do AP
   #define DEAUTH_INTERVAL_MS 50       // Agressividade do ataque
   ```

### 🚀 Compilação e Flash
```bash
cd DeauthFlood
idf.py build
idf.py flash monitor
```

## 📊 Logs Esperados

### 🔥 Durante o Ataque
```
🔥 === INICIANDO ATAQUE DEAUTH FLOOD ===
✅ Conectado ao AP! IP: 192.168.4.101
⏱️ Mantendo conexão por 100ms...
💥 DEAUTH #1 - Forçando desconexão!
❌ Desconectado! Tentando reconectar...
✅ Reconectado! Continuando ataque...
💥 DEAUTH #2 - Forçando desconexão!
```

### 📈 Estatísticas do Ataque
```
📊 === ESTATÍSTICAS DEAUTH FLOOD ===
🎯 Total de ciclos: 500
💥 Desautenticações forçadas: 487
✅ Reconexões bem-sucedidas: 480
❌ Falhas de reconexão: 20
📉 Taxa de sucesso: 96.0%
⏱️ Duração do ataque: 45 segundos
🔥 Média: 10.8 deauths/segundo
```

## 🛡️ Contramedidas do AP

### 🚫 Mitigação Automática
1. **Detecção**: Monitora taxa de desconexões por segundo
2. **Análise**: Identifica padrões repetitivos por MAC
3. **Blacklist**: Adiciona MAC à lista negra por 60 segundos
4. **Bloqueio**: Impede reconexões do atacante

### ⚙️ Configurações de Defesa
```c
#define MAX_DISCONNECTIONS_PER_SECOND 8   // Limite de detecção
#define BLACKLIST_DURATION_MS 60000       // 60 segundos de bloqueio
```

## 🎓 Valor Educacional

### 📚 Conceitos Demonstrados
- **Ataques de Deautenticação**: Simulação de desconexões forçadas
- **Overhead de Processamento**: Impacto no sistema do AP
- **Detecção Comportamental**: Análise de padrões de desconexão
- **Rate Limiting**: Controle de frequência de eventos

### 🔍 Análise de Segurança
- **Simplicidade**: Fácil de implementar e executar
- **Disrupção**: Pode afetar estabilidade do AP
- **Detectabilidade**: Padrões repetitivos são facilmente identificáveis
- **Mitigação**: Blacklist temporal é efetiva

### 💡 Diferenças do Deauth Real
Este ataque **simula** deauth flood através de:
- Auto-desconexão em vez de frames de deauth
- Teste de resiliência do sistema de eventos
- Validação de detecção de padrões anômalos

## ⚠️ Limitações

### 🔧 Simulação vs Ataque Real
- **Frame Injection**: Não injeta frames de deauth reais
- **Monitor Mode**: Não requer modo monitor
- **Compatibilidade**: Funciona dentro das limitações do ESP-IDF
- **Educacional**: Foca em detecção e mitigação

## 🔗 Comparação com Outros Ataques

| Ataque | Alvo | Método | Detectabilidade |
|--------|------|--------|-----------------|
| **DeauthFlood** | Sistema de eventos | Auto-desconexão | Alta |
| ConnectFlood | Slots de associação | Múltiplas conexões | Alta |
| AuthFlood | Sistema de auth | Tentativas falsas | Média |
| PacketFlood | Largura de banda | Volume de dados | Alta |

## ⚠️ Aviso Legal

Este código é destinado **exclusivamente para fins educacionais** e testes em ambientes controlados.

**NÃO** use este código:
- Em redes que não sejam suas
- Para atividades maliciosas
- Em ambientes de produção
- Sem autorização explícita

O uso inadequado pode violar leis locais e regulamentações de segurança cibernética.

## 🔗 Projetos Relacionados

- **`/AP`** - Access Point com sistema de detecção
- **`/CLIENTS`** - Clientes legítimos para comparação
- **`/ConnectFlood`** - Ataque de inundação de conexões
- **`/AuthFlood`** - Ataque de inundação de autenticação
- **`/PacketFlood`** - Ataque de inundação de pacotes

---

📖 **Documentação completa**: `SISTEMA_SEGURANCA_WIFI.md`
