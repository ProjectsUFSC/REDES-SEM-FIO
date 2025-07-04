# Guia de Teste: AP + Clientes

## Passo a Passo para Testar

### 1. Preparar o Access Point
```bash
cd AP
idf.py build
idf.py flash monitor
```

**Saída esperada do AP:**
```
Access Point iniciado!
SSID: ESP32_AP
Canal: 1
Máximo de conexões: 4
Autenticação: WPA2_PSK
Access Point ativo - aguardando conexões...
```

### 2. Preparar o Cliente
```bash
# Em outro terminal
cd CLIENTS
idf.py build
idf.py flash monitor
```

**Saída esperada do Cliente:**
```
 Inicializando ESP32 como Cliente Wi-Fi...
Iniciando conexão ao AP...
 CONECTADO COM SUCESSO! 
IP obtido: 192.168.4.2
Gateway: 192.168.4.1
 Conectado ao AP ESP32_AP com sucesso!
```

### 3. Verificações no AP
Quando o cliente conectar, o AP deve mostrar:
```
Cliente conectado - MAC: xx:xx:xx:xx:xx:xx
```

### 4. Verificações no Cliente
O cliente mostrará informações detalhadas:
```
=== INFORMAÇÕES DA CONEXÃO ===
SSID: ESP32_AP
RSSI: -45 dBm
Canal: 1
IP do Cliente: 192.168.4.2
Gateway (AP): 192.168.4.1

 Testando conectividade com ping para o AP...
 Ping OK - seq=1, ttl=64, time=5ms
 Ping OK - seq=2, ttl=64, time=3ms
 Ping OK - seq=3, ttl=64, time=4ms
```

##  Como Confirmar que Está Funcionando

### No Access Point:
- [ ] Mensagem "Access Point iniciado!"
- [ ] Mensagem "Cliente conectado - MAC: ..."
- [ ] Logs periódicos "Access Point ativo"

### No Cliente:
- [ ] Mensagem " CONECTADO COM SUCESSO! "
- [ ] IP obtido na faixa 192.168.4.x
- [ ] Gateway mostrado como 192.168.4.1
- [ ] Pings bem-sucedidos para o AP
- [ ] Status "Conectado: SIM "

### Testes Adicionais:

#### 1. **Teste de Múltiplos Clientes**
- Conecte até 4 ESP32 como clientes
- Cada um deve receber um IP diferente
- O AP deve reportar cada conexão

#### 2. **Teste de Reconexão**
- Desligue e religue um cliente
- Deve reconectar automaticamente
- AP deve reportar desconexão e nova conexão

#### 3. **Teste de Alcance**
- Afaste gradualmente o cliente do AP
- Observe mudanças no RSSI (força do sinal)
- Teste limite de alcance

#### 4. **Verificação de Rede**
- IP do AP: `192.168.4.1` (fixo)
- IPs dos clientes: `192.168.4.2`, `192.168.4.3`, etc.
- Máscara: `255.255.255.0`

##  Troubleshooting

### Cliente não conecta:
1. Verificar se AP está rodando
2. Verificar SSID e senha corretos
3. Verificar se não excedeu 4 conexões
4. Resetar ambos os dispositivos

### Cliente conecta mas desconecta:
1. Verificar sinal Wi-Fi (RSSI)
2. Verificar alimentação estável
3. Verificar interferências

### Sem IP:
1. Verificar logs do AP
2. Aguardar mais tempo (até 30s)
3. Resetar cliente

### Ping falha:
1. Verificar se obteve IP
2. Verificar gateway correto
3. Verificar se AP responde

##  Métricas de Sucesso

- **Tempo de conexão**: < 10 segundos
- **RSSI**: > -70 dBm para boa conexão
- **Ping RTT**: < 50ms típico
- **Taxa de sucesso**: 100% em ambiente controlado
- **Reconexão**: < 5 segundos após falha

##  Casos de Teste

### Teste 1: Conexão Básica
- [ ] AP inicia corretamente
- [ ] Cliente conecta e obtém IP
- [ ] Ping funciona

### Teste 2: Múltiplos Clientes
- [ ] 2 clientes conectam simultaneamente
- [ ] 3 clientes conectam simultaneamente  
- [ ] 4 clientes conectam simultaneamente
- [ ] 5º cliente é rejeitado

### Teste 3: Estabilidade
- [ ] Conexão mantida por 1 hora
- [ ] Reconexão após reset do cliente
- [ ] Reconexão após reset do AP

### Teste 4: Alcance
- [ ] Conexão a 1 metro
- [ ] Conexão a 5 metros
- [ ] Conexão a 10 metros
- [ ] Limite de alcance identificado
