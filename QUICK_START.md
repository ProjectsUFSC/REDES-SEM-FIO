# Guia de Início Rápido - WiFi Security Research

## Setup Rápido

### 1. Pré-requisitos
```bash
# Verificar ESP-IDF
echo $IDF_PATH
# Se vazio, instalar ESP-IDF primeiro

# Conectar ESP32s via USB
# Verificar portas disponíveis
ls /dev/ttyUSB*  # Linux
# ou
ls /dev/cu.usbserial*  # macOS
```

### 2. Executar Access Point (ESP32 #1)
```bash
cd AP/
idf.py set-target esp32
idf.py build flash monitor
```
**Aguardar**: `ACCESS POINT SEGURO INICIADO`

### 3. Testar Cliente Normal (ESP32 #2)
```bash
# Em novo terminal
cd CLIENTS/
idf.py -p /dev/ttyUSB1 build flash monitor
```
**Aguardar**: `Conectado! IP: 192.168.4.100`

### 4. Executar Ataque (ESP32 #3)
```bash
# Em novo terminal - escolher um ataque
cd DeauthFlood/
idf.py -p /dev/ttyUSB2 build flash monitor
```
**Observar**: AP detecta e bloqueia o ataque

## O que Observar

### No Console do AP:
```
DEAUTH FLOOD DETECTADO! 12 desconexões em 1s
MAC 02:aa:bb:cc:dd:ee bloqueado por DEAUTH_FLOOD
Blacklist ativa: 1/20 entradas
```

### No Console do Atacante:
```
Tentativa #15 - Conectando...
Rejeitado! Possível detecção ativa
ATAQUE BLOQUEADO - Blacklist confirmada
```

### No Console do Cliente:
```
Latência aumentou para 250ms
Conexão mantida (protegido pelo IDS)
```

## Testar Outros Ataques

### DeauthFlood
```bash
cd DeauthFlood/
idf.py -p /dev/ttyUSB2 build flash monitor
```

### AuthFlood  
```bash
cd AuthFlood/
idf.py -p /dev/ttyUSB2 build flash monitor
```

### PacketFlood
```bash
cd PacketFlood/
idf.py -p /dev/ttyUSB2 build flash monitor
```

## Troubleshooting Rápido

### Erro: Permission denied /dev/ttyUSB0
```bash
sudo usermod -a -G dialout $USER
# Logout e login novamente
```

### ESP32 não detectado
```bash
# Instalar drivers CP210x
# Verificar cabo USB (deve ser data, não apenas power)
```

### Flash failed
```bash
# Pressionar BOOT no ESP32 durante flash
# Ou usar:
idf.py -p /dev/ttyUSB0 erase-flash
idf.py -p /dev/ttyUSB0 build flash
```

### WiFi connection failed
```bash
# Verificar se AP está rodando
# Verificar SSID e senha em main/[Module].c
```

## Métricas de Sucesso

### AP Funcionando Corretamente:
- Sistema de detecção ativo
- Thresholds configurados
- Blacklist: 0/20 entradas
- Clientes legítimos conectam normalmente

### Ataque Executando:
- Tentativas de conexão/auth/pacotes
- Taxa alta de atividade
- Logs indicam progresso do ataque

### Defesa Funcionando:
- Detecção em 1-3 segundos
- Blacklist automática ativada
- Ataque interrompido
- Cliente legítimo mantém conectividade

## Próximos Passos

1. **Ajustar Thresholds**: Modificar sensibilidade do IDS
2. **Analisar Logs**: Estudar padrões de detecção
3. **Customizar Ataques**: Modificar intensidade e características
4. **Documentar Resultados**: Registrar métricas de performance
5. **Testar Cenários**: Combinar múltiplos ataques

## Documentação Completa

- **[README.md](README.md)** - Documentação principal
- **[ModuleName/README_TECNICO.md]** - Documentação específica de cada ataque

---

