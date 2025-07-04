# Auth Flood Attack - ESP32

## Visão Geral

Este projeto implementa um **ataque de inundação de autenticação (Auth Flood)** usando ESP32, onde um cliente malicioso realiza múltiplas tentativas de autenticação com credenciais falsas para sobrecarregar o sistema de autenticação do Access Point.

## Objetivo do Ataque

O Auth Flood visa:
- **Sobrecarregar o sistema de autenticação** do AP
- **Esgotar recursos** de processamento de credenciais
- **Criar logs excessivos** no sistema
- **Potencialmente causar DoS** no serviço de autenticação

## Como Funciona

### Estratégias de Ataque

1. **Tentativas Rápidas**: Múltiplas tentativas de auth por segundo
2. **Credenciais Falsas**: Usa senhas aleatórias/incorretas
3. **MACs Aleatórios**: Pode randomizar endereço MAC
4. **Persistência**: Mantém ataque por período prolongado

### Configurações do Ataque

```c
#define AUTH_FLOOD_INTERVAL_MS 100    // Intervalo entre tentativas (100ms)
#define MAX_AUTH_ATTEMPTS 500         // Máximo de tentativas
#define USE_RANDOM_MAC true           // Randomizar MAC a cada tentativa
#define USE_WRONG_PASSWORD true       // Usar senhas incorretas
```

## Detecção pelo AP

O AP detecta Auth Flood através de:

### Monitoramento de Tentativas
- **Tentativas de auth por segundo** (limite: 10 tentativas/s)
- **Padrões de falha** consecutivos
- **MACs suspeitos** com alta frequência
- **Credenciais inválidas** repetidas

### Indicadores de Detecção
```
AUTH FLOOD DETECTADO! 
15 tentativas de auth em 1s (limite: 10)
98% de tentativas com credenciais inválidas
MAC bloqueado por AUTH_FLOOD (60 seg)
```

##  Como Usar

###  Pré-requisitos
- ESP-IDF instalado e configurado
- ESP32 disponível
- AP alvo rodando (projeto `/AP`)

###  Configuração
1. **Ajustar configurações** em `AuthFlood.c`:
   ```c
   #define TARGET_SSID "ESP32_AP"        // SSID do AP alvo
   #define WRONG_PASSWORDS_LIST {        // Lista de senhas falsas
       "wrongpass1", "wrongpass2", "admin123"
   }
   #define AUTH_FLOOD_INTERVAL_MS 100    // Agressividade do ataque
   ```

###  Compilação e Flash
```bash
cd AuthFlood
idf.py build
idf.py flash monitor
```

##  Logs Esperados

###  Iniciando o Ataque
```
 === INICIANDO AUTH FLOOD ATTACK ===
 Alvo: ESP32_AP
 Estratégia: Senhas falsas + MACs aleatórios
 Intervalo: 100ms (10 tentativas/segundo)
 Randomização de MAC: ATIVADA
```

###  Durante o Ataque
```
 AUTH #1 - MAC: aa:bb:cc:dd:ee:01, Senha: wrongpass123
 Falha de autenticação (esperado)
 AUTH #2 - MAC: aa:bb:cc:dd:ee:02, Senha: admin123
 Falha de autenticação (esperado)
 AUTH #3 - MAC: aa:bb:cc:dd:ee:03, Senha: password
 Falha de autenticação (esperado)
 Taxa atual: 9.8 tentativas/segundo
```

###  Detectado e Bloqueado
```
 DETECÇÃO ATIVADA! AP bloqueou nossas tentativas
 Erro: Conexão rejeitada
 Tentando com novo MAC aleatório...
 Todos os MACs parecem estar bloqueados
 ATAQUE MITIGADO PELO AP
```

###  Estatísticas Finais
```
 === ESTATÍSTICAS AUTH FLOOD ===
 Total de tentativas: 167
 Falhas de autenticação: 167 (100%)
 Autenticações bem-sucedidas: 0
Taxa média: 8.3 tentativas/segundo
MACs únicos utilizados: 45
 Duração antes do bloqueio: 20 segundos
 Objetivo alcançado: Sistema de detecção ativado
```

##  Contramedidas do AP

###  Sistema de Detecção
1. **Rate Limiting**: Monitora tentativas de auth por segundo
2. **Failure Analysis**: Analisa taxa de falhas de autenticação
3. **Blacklist Inteligente**: Bloqueia baseado em padrões
4. **Adaptive Thresholds**: Ajusta limites dinamicamente

###  Configurações de Defesa
```c
#define MAX_AUTH_ATTEMPTS_PER_SECOND 10   // 10 tentativas/segundo máximo
#define AUTH_FAILURE_THRESHOLD 5          // 5 falhas consecutivas
#define BLACKLIST_DURATION_MS 60000       // 60 segundos de bloqueio
```

###  Resposta Automática
```
 Análise de autenticação:
  - Tentativas/segundo: 15 (limite: 10)
  - Taxa de falhas: 100% (suspeito)
  - Padrão detectado: AUTH_FLOOD

 CONTRAMEDIDAS ATIVADAS:
  1.  MAC adicionado à blacklist
  2.  Tentativas futuras bloqueadas
  3.  Rate limiting aplicado
  4.  Log de segurança gerado
```

##  Valor Educacional

###  Conceitos Demonstrados
- **Ataques de Autenticação**: Exploração de sistemas de login
- **Brute Force**: Tentativas de quebra por força bruta
- **Rate Limiting**: Proteção contra ataques automatizados
- **Behavioral Analysis**: Detecção baseada em padrões

###  Análise de Segurança
- **Facilidade**: Relativamente simples de implementar
- **Detectabilidade**: Padrões são facilmente identificáveis
- **Impacto**: Pode afetar disponibilidade do serviço
- **Contramedidas**: Rate limiting e blacklist são efetivos

###  Variações do Ataque
1. **Dictionary Attack**: Lista de senhas comuns
2. **Credential Stuffing**: Reutilização de credenciais vazadas
3. **Password Spraying**: Poucos passwords, muitos usuários
4. **Hybrid Attack**: Combinação de técnicas

##  Ataques Similares no Mundo Real

###  Ferramentas Profissionais
- **Hydra**: Brute force para múltiplos protocolos
- **John the Ripper**: Quebra de passwords
- **Hashcat**: GPU-accelerated password cracking
- **Medusa**: Parallel login brute-forcer

###  Comando Exemplo
```bash
# Hydra SSH brute force (apenas para testes autorizados)
hydra -l admin -P passwords.txt ssh://192.168.1.100

# Aircrack-ng para WPA/WPA2
aircrack-ng -w wordlist.txt capture.cap
```

##  Proteções Profissionais

###  Autenticação Robusta
- **Multi-Factor Authentication (MFA)**: Segundo fator obrigatório
- **Account Lockout**: Bloqueio após X tentativas
- **Progressive Delays**: Aumenta delay a cada falha
- **CAPTCHA**: Verificação humana após tentativas

###  Monitoramento Avançado
```python
# Exemplo de detecção avançada (conceitual)
def detect_auth_flood(client_ip, attempts_per_minute, failure_rate):
    if attempts_per_minute > 50 and failure_rate > 0.9:
        trigger_rate_limiting(client_ip)
        alert_security_team(f"Auth flood from {client_ip}")
        return True
    return False
```

###  Arquitetura Defensiva
- **Authentication Servers**: Servidores dedicados
- **Load Balancing**: Distribuição de carga de auth
- **Honeypots**: Detectar e estudar atacantes
- **Threat Intelligence**: Blacklists externos

##  Boas Práticas de Senha

###  Para Administradores
- **Senhas complexas**: Mínimo 12 caracteres
- **Rotação regular**: Mudança periódica
- **Senhas únicas**: Não reutilizar credenciais
- **Password managers**: Uso de gerenciadores

###  Configurações Seguras
```c
// Configurações de segurança recomendadas
#define MIN_PASSWORD_LENGTH 12
#define MAX_AUTH_ATTEMPTS_PER_HOUR 20
#define ACCOUNT_LOCKOUT_DURATION 1800  // 30 minutos
#define REQUIRE_COMPLEX_PASSWORDS true
```

##  Aspectos Legais e Éticos

###  Considerações Importantes
- **Testes autorizados**: Apenas em infraestrutura própria
- **Compliance**: Respeitar regulamentações (LGPD, GDPR)
- **Disclosure**: Reportar vulnerabilidades encontradas
- **Documentação**: Manter logs de atividades de teste

###  Checklist Ético
- [ ] Autorização por escrito do proprietário
- [ ] Ambiente isolado de produção
- [ ] Objetivos educacionais claramente definidos
- [ ] Sem intenção maliciosa
- [ ] Resultados usados para melhorar segurança

##  Aviso Legal

Este código é destinado **exclusivamente para fins educacionais** e testes autorizados.

**NÃO** use este código:
- Para quebrar senhas reais
- Em sistemas de produção
- Contra infraestrutura de terceiros
- Sem autorização explícita

O uso inadequado pode violar:
- Leis de acesso não autorizado
- Regulamentações de proteção de dados
- Termos de serviço de sistemas
- Códigos de ética profissional

##  Projetos Relacionados

- **`/AP`** - Access Point com detecção de auth flood
- **`/CLIENTS`** - Clientes legítimos para comparação
- **`/ConnectFlood`** - Ataque de inundação de conexões
- **`/PacketFlood`** - Ataque de volume de tráfego
- **`/DeauthFlood`** - Ataque de desautenticação

---

 **Documentação completa**: `SISTEMA_SEGURANCA_WIFI.md`
