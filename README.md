# Sistema de Segurança Wi-Fi com ESP32

## Visão Geral

Este projeto implementa um **sistema de detecção e mitigação de ataques Wi-Fi** usando ESP32, com um Access Point seguro e múltiplos tipos de ataques de flooding simulados para fins educacionais e testes de segurança.

O sistema permite estudar vulnerabilidades do protocolo IEEE 802.11 em ambiente controlado, implementando tanto vetores de ataque de negação de serviço quanto mecanismos de defesa automatizada.

## Objetivos do Projeto

- **Demonstrar vulnerabilidades** relacionadas a ataques de flooding em redes Wi-Fi IEEE 802.11
- **Implementar sistemas de detecção** em tempo real baseados em anomalias de tráfego
- **Desenvolver contramedidas efetivas** contra ataques de DoS/flooding
- **Fornecer material educacional** sobre segurança wireless
- **Criar ambiente de testes** controlado e reproduzível para pesquisa em segurança

## Arquitetura do Sistema

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   ACCESS POINT  │    │  CLIENTES BONS  │    │ CLIENTES RUINS  │
│                 │    │                 │    │                 │
│ • Detecção      │◄──►│ • Tráfego       │    │ • AuthFlood     │
│ • Mitigação     │    │   normal        │    │ • DeauthFlood   │
│ • Blacklist     │    │ • Mensagens     │    │ • PacketFlood   │
│ • Monitoramento │    │   legítimas     │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Estrutura do Projeto

```
REDES SEM FIO/
├── AP/                    # Access Point com detecção de ataques
├── CLIENTS/               # Clientes legítimos
├── DeauthFlood/           # Ataque de inundação de desautenticação
├── AuthFlood/             # Ataque de inundação de autenticação
├── PacketFlood/           # Ataque de inundação de pacotes
├── SISTEMA_SEGURANCA_WIFI.md  # Documentação técnica
└── README.md              # Este arquivo
```

## Tipos de Ataques Implementados

### 1. Deauth Flood
- **Objetivo**: Simular desautenticações forçadas
- **Método**: Ciclos rápidos de conectar/desconectar
- **Detecção**: Taxa de desconexões por segundo
- **Mitigação**: Bloqueio baseado em padrão

### 2. Auth Flood
- **Objetivo**: Sobrecarregar sistema de autenticação
- **Método**: Tentativas massivas com credenciais falsas
- **Detecção**: Tentativas de auth por segundo
- **Mitigação**: Rate limiting inteligente

### 3. Packet Flood
- **Objetivo**: Saturar largura de banda
- **Método**: Alto volume de tráfego UDP/TCP
- **Detecção**: Pacotes por segundo por cliente
- **Mitigação**: Throttling e desconexão

## Sistema de Defesa do AP

### Métricas Monitoradas

| Métrica | Limite | Ação |
|---------|--------|------|
| Desconexões/seg | 8 | Blacklist 60s |
| Auth attempts/seg | 10 | Blacklist 60s |
| Pacotes/seg/cliente | 50 | Throttling |

### Sistema de Blacklist
- **Capacidade**: 20 MACs simultâneos
- **Duração**: 60 segundos (configurável)
- **Tipos**: Por tipo de ataque detectado
- **Auto-expiração**: Remove automaticamente

### Detecção Automática
- **Tempo real**: Análise contínua de eventos
- **Padrões**: Identificação de comportamento anômalo
- **Adaptativa**: Thresholds configuráveis
- **Logging**: Registros detalhados de segurança

## Quick Start

**Para início rápido (5 minutos)**: Consulte **[QUICK_START.md](QUICK_START.md)**

### Pré-requisitos
```bash
# ESP-IDF v4.4 ou superior
. $HOME/esp/esp-idf/export.sh

# Hardware necessário
# - 2+ ESP32 (1 para AP, 1+ para ataques)
# - Cabos USB para programação
# - Computador com ESP-IDF configurado
```

### Execução Passo a Passo

#### 1. **Configurar o Access Point Inteligente**
```bash
cd AP/
idf.py set-target esp32
idf.py build flash monitor
```
**Resultado**: AP com IDS ativo em `ESP32_AP` (192.168.4.1)

#### 2. **Testar com Cliente Legítimo**
```bash
cd CLIENTS/
idf.py build flash monitor
```
**Resultado**: Cliente conecta normalmente e gera tráfego benigno

#### 3. **Executar Ataques** (escolher um por vez)
```bash
# Deauth Flood Attack  
cd DeauthFlood/
idf.py build flash monitor

# Authentication Flood
cd AuthFlood/
idf.py build flash monitor

# Packet Flood
cd PacketFlood/
idf.py build flash monitor
```

### Monitoramento em Tempo Real
- **AP Logs**: Console do ESP32 do AP mostra detecções e mitigações
- **Attack Logs**: Console do ESP32 atacante mostra progresso do ataque
- **Métricas**: Thresholds, blacklist, performance em tempo real

### Logs de Exemplo

**AP detectando ataque**:
```
ACCESS POINT SEGURO INICIADO
SSID: ESP32_AP, Canal: 1, Max clients: 20
Sistema de detecção: ATIVO
Thresholds: 8 disc/s, 10 auth/s, 50 pkt/s

Cliente conectado! MAC: 24:6f:28:aa:bb:cc
DEAUTH FLOOD DETECTADO! 12 desconexões em 1s
MAC 02:aa:bb:cc:dd:ee bloqueado por DEAUTH_FLOOD
Blacklist ativa: 3/20 entradas
```

**Atacante sendo bloqueado**:
```
INICIANDO DEAUTH FLOOD
Tentativa #1 - Conectando...
Conectado! Desconectando para continuar ataque...
Tentativa #2 - Conectando...
Rejeitado! Possível detecção ativa
ATAQUE BLOQUEADO - Blacklist confirmada
```

## Documentação Detalhada

**[INDICE_DOCUMENTACAO.md](INDICE_DOCUMENTACAO.md)** - Índice completo de toda a documentação

### Documentação Técnica
- **[DOCUMENTACAO_TECNICA.md](DOCUMENTACAO_TECNICA.md)** - Guia técnico completo com arquitetura do sistema, instruções de execução, troubleshooting e configurações avançadas

- **[QUICK_START.md](QUICK_START.md)** - Guia de início rápido para demonstração em 5 minutos

### Documentação por Módulo:
- [`AP/README.md`](AP/README.md) - Sistema de detecção e Access Point inteligente
- [`CLIENTS/README_TECNICO.md`](CLIENTS/README_TECNICO.md) - Simuladores de clientes legítimos
- [`DeauthFlood/README_TECNICO.md`](DeauthFlood/README_TECNICO.md) - Ataque de desautenticação
- [`AuthFlood/README_TECNICO.md`](AuthFlood/README_TECNICO.md) - Ataque de inundação de autenticação
- [`PacketFlood/README_TECNICO.md`](PacketFlood/README_TECNICO.md) - Ataque de inundação de pacotes

### Material Educacional
- **Conceitos de segurança** wireless
- **Técnicas de detecção** de ataques
- **Implementação de contramedidas**
- **Análise de vulnerabilidades**

## Logs e Análise

### Logs do AP (Exemplo)
```
ACCESS POINT SEGURO INICIADO
SSID: ESP32_AP, Canal: 1, Max clients: 20
Sistema de detecção: ATIVO
Thresholds: 8 disc/s, 10 auth/s, 50 pkt/s

Cliente conectado! MAC: 24:6f:28:aa:bb:cc
DEAUTH FLOOD DETECTADO! 12 desconexões em 1s
MAC 02:aa:bb:cc:dd:ee bloqueado por DEAUTH_FLOOD
Blacklist ativa: 3/20 entradas
```

### Logs de Ataque (Exemplo)
```
INICIANDO DEAUTH FLOOD
Tentativa #1 - Conectando...
Conectado! Desconectando para continuar ataque...
Tentativa #2 - Conectando...
Rejeitado! Possível detecção ativa
ATAQUE BLOQUEADO - Blacklist confirmada
```

## Valor Educacional

### Conceitos de Segurança Abordados
- **Vulnerabilidades IEEE 802.11**: Limitações inerentes dos protocolos wireless
- **Ataques DoS/DDoS**: Técnicas de negação de serviço em camadas 2-4
- **Sistemas IDS**: Detecção de anomalias e análise comportamental
- **Contramedidas**: Rate limiting, blacklist dinâmica, monitoramento
- **Arquitetura Defensiva**: Design de sistemas seguros embarcados

### Competências Desenvolvidas
- **Programação ESP32**: ESP-IDF, FreeRTOS, networking
- **Protocolos de Rede**: TCP/IP, 802.11, DHCP
- **Análise de Segurança**: Identificação e exploração de vulnerabilidades
- **Implementação de Defesas**: Sistemas de mitigação automatizada
- **Debugging Avançado**: Análise de logs e troubleshooting de rede

### Aplicações Práticas
- **Pentesting Wireless**: Validação de segurança de redes corporativas
- **Red Team Exercises**: Simulações de ataque para treinamento
- **Desenvolvimento de Produtos**: IoT e sistemas embarcados seguros
- **Pesquisa Acadêmica**: Estudo de novos protocolos e vulnerabilidades
- **Educação em Cibersegurança**: Material prático para ensino

## Considerações Éticas e Legais

### **USO RESPONSÁVEL - OBRIGATÓRIO**
- PERMITIDO: Redes próprias ou com autorização explícita por escrito
- PERMITIDO: Ambiente de laboratório controlado e isolado
- PERMITIDO: Pesquisa acadêmica com fins educacionais
- PROIBIDO: Redes de terceiros sem autorização
- PROIBIDO: Fins maliciosos, criminosos ou destrutivos
- PROIBIDO: Interferência em infraestrutura crítica

### Checklist Ético Obrigatório
- [ ] Rede própria ou autorização por escrito do proprietário
- [ ] Ambiente completamente isolado de sistemas de produção
- [ ] Objetivos educacionais/pesquisa claramente definidos
- [ ] Sem exposição ou captura de dados sensíveis reais
- [ ] Resultados utilizados exclusivamente para melhorar segurança
- [ ] Conformidade com leis locais de telecomunicações

### Aspectos Legais
- **Regulamentações**: Respeitar leis locais de telecomunicações e cibersegurança
- **Autorização**: Obter consentimento explícito antes de qualquer teste
- **Documentação**: Manter registros detalhados de todas as atividades
- **Responsabilidade**: Uso inadequado é de total responsabilidade do usuário

## Configurações Avançadas

### Personalização de Thresholds do IDS
```c
// Em AP/main/AP.c - Ajustar sensibilidade
#define DEAUTH_FLOOD_THRESHOLD 8      // desconexões/segundo  
#define AUTH_FLOOD_THRESHOLD 10       // tentativas auth/segundo
#define PACKET_FLOOD_THRESHOLD 50     // pacotes/segundo/cliente
#define BLACKLIST_DURATION_MS 60000   // duração do bloqueio (ms)
#define MAX_BLACKLIST_ENTRIES 20      // capacidade máxima
```

### Debug Detalhado
```c
// Habilitar logs verbose para análise
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
esp_log_level_set("*", ESP_LOG_DEBUG);
```

### Monitoramento de Recursos
```c
// Adicionar ao código para monitorar performance
ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
ESP_LOGI(TAG, "CPU usage: %d%%", get_cpu_usage_percent());
```

## Desenvolvimentos Futuros

### Melhorias Planejadas
- **Machine Learning**: IDS inteligente com detecção adaptativa
- **Dashboard Web**: Interface de monitoramento em tempo real
- **API RESTful**: Integração com sistemas externos de segurança
- **Suporte WPA3**: Implementação de protocolos mais modernos
- **Multi-AP Mesh**: Sistema distribuído de detecção
- **Mobile App**: Controle e monitoramento via smartphone

### Áreas de Pesquisa
- **IoT Security**: Segurança para dispositivos IoT embarcados
- **5G/6G Wireless**: Adaptação para redes de próxima geração
- **AI-driven Attacks**: Ataques baseados em inteligência artificial
- **Quantum-resistant Crypto**: Preparação para era pós-quântica
- **Edge Computing**: Processamento distribuído de segurança

### Roadmap de Desenvolvimento
```
Versão 1.0 (Atual): Ataques básicos + IDS simples
Versão 2.0: ML integration + Web dashboard
Versão 3.0: Enterprise features + API
Versão 4.0: Multi-AP coordination + Cloud integration
```

## Contribuições e Suporte

### Como Contribuir
1. **Fork** o repositório
2. **Crie uma branch** para sua feature (`git checkout -b feature/nova-funcionalidade`)
3. **Commit** suas mudanças (`git commit -m 'Adiciona nova funcionalidade'`)
4. **Push** para a branch (`git push origin feature/nova-funcionalidade`)
5. **Abra um Pull Request** com descrição detalhada

### Reportar Problemas
- Utilize o sistema de **Issues** do repositório
- Inclua logs completos e configuração usada
- Descreva passos para reproduzir o problema
- Especifique hardware e versão do ESP-IDF

### Suporte Técnico
Para dúvidas técnicas, consulte em ordem:
1. **[DOCUMENTACAO_TECNICA.md](DOCUMENTACAO_TECNICA.md)** - Troubleshooting completo
2. **Issues** existentes no repositório
3. **ESP-IDF Documentation** oficial
4. **Logs de debug** com `ESP_LOG_DEBUG` habilitado

## 📄 Licença e Direitos

###  Licença de Uso
Este projeto é distribuído sob **Licença Educacional**:
-  **Uso acadêmico**: Permitido com atribuição
-  **Pesquisa**: Permitido com citação adequada
-  **Ensino**: Permitido em instituições educacionais
-  **Uso comercial**: Requer autorização expressa dos autores
-  **Distribuição modificada**: Sem autorização dos autores

### 🏛️ Instituição Acadêmica
- **Desenvolvido para**: Pesquisa em Cibersegurança
- **Objetivo**: Educação e conscientização sobre segurança WiFi
- **Orientação**: Prof. [Nome do Orientador]
- **Instituição**: [Nome da Universidade/Instituto]

## � Recursos e Referências

###  Documentação Técnica
- **[ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)**
- **[IEEE 802.11 Standard](https://www.ieee802.org/11/)**
- **[NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)**

###  Segurança e Pesquisa
- **[OWASP IoT Security](https://owasp.org/www-project-internet-of-things/)**
- **[WiFi Security Research](https://www.krackattacks.com/)**
- **[CVE Database](https://cve.mitre.org/)**

###  Ferramentas Complementares
- **[Wireshark](https://www.wireshark.org/)** - Análise de tráfego de rede
- **[Aircrack-ng](https://www.aircrack-ng.org/)** - Suite de segurança WiFi
- **[ESP32 DevKitC](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html)** - Hardware de referência

---

## **AVISO LEGAL IMPORTANTE**

Este projeto é destinado **EXCLUSIVAMENTE** para fins educacionais, pesquisa acadêmica e testes em ambientes controlados. O uso inadequado das técnicas implementadas pode:

- **Violar leis** locais e internacionais de telecomunicações
- **Causar interferência** em infraestrutura crítica
- **Resultar em responsabilização legal** do usuário
- **Comprometer segurança** de terceiros

**OS AUTORES NÃO SE RESPONSABILIZAM** pelo uso inadequado, ilegal ou malicioso deste código. A utilização implica **TOTAL RESPONSABILIDADE** do usuário em garantir conformidade ética e legal.

---

**Para documentação técnica completa e relatório acadêmico**: consulte [`DOCUMENTACAO_TECNICA.md`](DOCUMENTACAO_TECNICA.md) e [`README_ACADEMICO.md`](README_ACADEMICO.md)

**Este projeto contribui para o avanço da pesquisa em cibersegurança através de experimentação controlada e responsável, promovendo o desenvolvimento de sistemas WiFi mais seguros.**
