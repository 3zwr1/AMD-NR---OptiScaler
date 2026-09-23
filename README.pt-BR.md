# AMDNR — DLSS 5 Neural Rendering em GPUs AMD (build OptiScaler) — v0.3.1

[English](README.md) | [中文](README.zh-CN.md) | **Português**

> **Precisamos do seu apoio.** Entre no servidor do Discord — <https://discord.gg/QzbzxfKYyh> — para
> ajuda, relatos de bugs e builds de teste; cada relato com um log torna a próxima build melhor.

DLSS 5 Neural Rendering rodando em GPUs AMD, integrado ao OptiScaler, para funcionar em qualquer
jogo Direct3D 12 que o OptiScaler já intercepta. Por cima do passo neural: model interleave para um
grande ganho de taxa de quadros, residual composition, geração de quadros XeSS desbloqueada até 6X e
FSR Ray Regeneration para jogos que usam DLSS Ray Reconstruction.

**Discord: <https://discord.gg/QzbzxfKYyh>** — suporte, relatos de bugs (`#bug-report`), builds de
teste. Se você precisar do `nv` da NVDA para qualquer coisa, ele está disponível
lá; não está nestes arquivos e o caminho AMD não precisa dele.

**Apoie o projeto: <https://ko-fi.com/3zinr>**

---

## AMDNR - Guia de instalação do OptiScaler

A instalação é bem simples.

### 1. Baixe os arquivos

Baixe estes dois arquivos do GitHub:

* `AMDNR-vX.X.X.zip`
* `Runtime.zip`

### 2. Extraia os dois arquivos

Extraia o conteúdo dos dois arquivos `.zip`.

### 3. Copie tudo para a pasta do jogo

Primeiro, copie todos os arquivos de `AMDNR-vX.X.X` para a pasta raiz do jogo — a mesma pasta onde
fica o `.exe` do jogo.

Depois, faça o mesmo com todos os arquivos de `Runtime`.

### 4. Renomeie o OptiScaler.dll

Dentro da pasta do jogo, encontre:

`OptiScaler.dll`

Renomeie para:

`dxgi.dll`

`dxgi.dll` é a opção recomendada.

Se o jogo não abrir ou o mod não carregar, tente renomear o `OptiScaler.dll` para um destes:

* `d3d12.dll`
* `winmm.dll`
* `version.dll`
* `dbghelp.dll`

Teste um nome por vez. Não crie várias cópias do `OptiScaler.dll`.

### 5. Inicie o jogo

`HOME` liga e desliga o Neural Rendering durante o jogo (nos dois runtimes; um pequeno aviso
mostra On / Off). Redefina a tecla ao lado da caixa Enable na aba Neural ou em Interface > Keybinds.

É isso.

Inicie o jogo normalmente e pressione:

`INSERT`

Isso abre o menu do OptiScaler / AMDNR, onde você configura o mod como quiser.

### Se não funcionar

Se o jogo ainda não abrir com nenhum dos nomes acima, relate no canal `#bug-report` do Discord.

Ao relatar o problema, envie também os arquivos `.log` que tiverem sido gerados na pasta raiz do
jogo.

Esses logs são muito importantes e nos ajudam a identificar o problema bem mais rápido.

> O `.exe` geralmente não está onde o atalho aponta. Jogos em Unreal o guardam em
> `<Jogo>\Binaries\Win64\`.

---

### O runtime lmxxf (0.3.0, opcional)

Um segundo runtime neural (licença MIT, do lmxxf) pode executar o passo no lugar do runtime do
danielblnc. Somente RDNA 4. Ele precisa de duas coisas ao lado do jogo:

1. `LmxxfNrRuntime.dll` — neste arquivo, ao lado de `OptiScaler.dll` (é copiado junto com o resto).
2. `LmxxfNrRuntime.pak` (382 MB, incluído no zip do AMDNR) ao lado de `LmxxfNrRuntime.dll` — os
   pesos, os módulos HIP e os shaders do lmxxf em um único arquivo criptografado e autenticado,
   aberto em memória.

No primeiro início que encontrar um runtime instalado sem escolha feita, o menu pergunta qual usar
(`[DlssNr] NrBackend = daniel | lmxxf` no ini registra a escolha; Neural > Neural runtime a troca,
válida no próximo início do jogo). A edição do lmxxf é aplicada com um quadro de atraso, carregada
pelos vetores de movimento, de modo que o quadro nunca espera pela rede (cerca de 30 ms em 1080p,
15 ms em 720p numa RX 9070 XT). Seu log é o `lmxxf_backend.log` ao lado do jogo.

**Compatibilidade (lmxxf).** O runtime vê apenas o que o DLSS vê, então o que varia por jogo é uma
lista curta: formato de cor e HDR, vetores de movimento e sua escala, profundidade e sua direção, a
reactive mask, a textura de exposição, o sinal de Reset e onde o passo fica (antes do Super
Resolution ou depois do Ray Reconstruction). Testado até agora:

| Jogo | API / posição | Notas |
|---|---|---|
| Silent Hill 2 | D3D12, antes do SR | jogo de referência; alocação de cor com padding da Unreal tratada |
| Forza Horizon 6 | D3D12, antes do SR | |
| Stray | D3D11 pela ponte D3D12, antes do SR | |
| GTA V Enhanced | D3D12, antes do SR, HDR, reactive mask de um canal | corrigido nesta build: a máscara era lida como "tudo reactive" e a edição nunca chegava à tela |
| Qualquer jogo com Ray Reconstruction | D3D12, depois do RR (escrito de volta na saída) | suportado a partir desta build; ainda não confirmado em um jogo |

Se um jogo não mostrar efeito: o `lmxxf_backend.log` tem uma linha `lmxxf inputs:` (formatos,
tamanhos, escala de movimento, direção da profundidade, máscara, exposição) e uma linha
`lmxxf stats @N:` a cada 600 quadros (exposição, brilho da entrada, a edição do modelo, a edição
carregada, keep, média da reactive, comprimento dos vetores e fração rejeitada). Anexe o log ao
relato; essas duas linhas normalmente dizem o porquê.

Sob o lmxxf, o bloco Neural runtime tem **Network history** (a entrada temporal do próprio modelo),
e Image look tem o grupo **lmxxf edit**: o edit shaper (Edit detail, Edit colour, Edge guard: ganho
na parte fina da edição do modelo, sua cor em relação à mudança de brilho e um esmaecimento da
edição nas bordas de profundidade) e **Output smoothing** (o passo de suavização de saída do
upstream, requer Network history). Neural passes, Residual strength/limit, nitidez, Debug view 1 e o
filtro Appearance valem para os dois runtimes. Chaves do ini: `AmdLmxxfHistory`, `AmdLmxxfEditDetail`,
`AmdLmxxfEditSaturation`, `AmdLmxxfEdgeGuard`, `AmdLmxxfOutputSmooth` em `[DlssNr]`.

## Requisitos

- Uma GPU AMD com driver atual. O runtime neural usa HIP pelo driver; não é preciso HIP SDK nem
  modo desenvolvedor. Quais chips: RX 9000 (RDNA 4) roda os dois runtimes; RX 7000 (RDNA 3, desktop
  e mobile) roda apenas o do danielblnc; APUs de portáteis (Z1 Extreme / 780M, Z2 Extreme / 890M) e
  RDNA 2 (RX 6000, Steam Deck) ainda não são suportados por nenhum dos dois. A aba Neural diz o que
  sua GPU consegue rodar.
- Um jogo Direct3D 12, Direct3D 11 ou Vulkan. O caminho neural AMD em si é D3D12; jogos D3D11 e
  Vulkan o alcançam pela ponte D3D12 do OptiScaler, o que significa que o upscaler precisa ser um dos
  backends "w/Dx12" (`ffx_12`). Deixe `Dx11Upscaler` / `VulkanUpscaler` em `auto` e esta build o
  escolhe por você quando o neural rendering está ligado.
- Cerca de 2 GB de VRAM livre em resoluções de renderização da classe 1080p.

## O que há nos dois arquivos

**AMDNR-vX.X.X.zip**

| Arquivo | O que é |
|---|---|
| `OptiScaler.dll` | OptiScaler com o backend AMD do DLSS-NR. Renomeie como o guia indica. |
| `OptiScaler.ini` | Configurações. O Neural Rendering vem ativado; o log vem ligado para que um relato tenha o que anexar. |
| `LmxxfNrRuntime.dll` | O runtime neural lmxxf (0.3.0). Usado só quando escolhido; lê o `LmxxfNrRuntime.pak` ao lado, veja "O runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Os pesos, módulos HIP e shaders do runtime lmxxf em um arquivo criptografado (382 MB). Só o runtime lmxxf o lê; é inofensivo mantê-lo junto com o runtime do danielblnc. |
| `OptiScaler\` | FSR, XeSS, o denoiser FidelityFX e o D3D12 Agility SDK que o OptiScaler usa. |
| `Licenses\`, `LICENSE` | Licenças de terceiros e a licença GPL-3.0 desta build. |
| `SHA256SUMS.txt` | Checksums de cada arquivo distribuído, nos dois arquivos. |

**Runtime.zip**

| Arquivo | O que é |
|---|---|
| `dlssnr_amd_pass1..3.dll` | O runtime neural AMD, v0.3.1 do danielblnc, sem modificações. Três cópias para que o multi-pass tenha uma por passe. |
| `dlssnr_on_amd_weights.bin` | Os pesos da rede que o runtime carrega. |

## Configurações que vale conhecer

Abra a aba **Neural**. Os padrões são o arranjo testado mais recente, então o primeiro passo útil é
mudar uma coisa por vez.

- **NR resolution** — a principal alavanca de qualidade/custo. Abaixo de 100% o modelo trabalha numa
  imagem menor e só a sua *correção* é levada de volta ao quadro em resolução completa, então o quadro
  mantém o próprio detalhe. Acima de 100% o custo cresce com o quadrado (150% é 2,25x).
- **Residual strength** — quanto da edição do modelo é aplicado; acima de 1 amplifica. É o controle
  que mais muda a imagem.
- **Residual limit** — um teto para o quanto um pixel pode se mover. Manchas: **abaixe**.
- **Model interleave** — roda o modelo a cada dois quadros para um grande ganho de taxa de quadros.
  Os quadros pulados são preenchidos pelo **Interleave preset**; *Guided fill v2* é o padrão e o que
  está em trabalho ativo. O ritmo dos dois tipos de quadro é automático.
- **Neural passes** — 2 e 3 empilham o modelo, com retornos decrescentes. Sob o lmxxf o histórico
  da rede continua sendo o primeiro passe; os passes extras são apenas refinamento espacial.
- **A geração de quadros vem desligada num ini novo.** Aba Frame Gen: escolha o FG Input (por
  exemplo "DLSSG via Streamline" num jogo com geração de quadros DLSS) e o FG Output (XeFG), marque
  **Active** na seção Frame Generation (XeFG) e pressione Save Settings. Um ini 0.1.0 que a tinha
  ligada não é aproveitado quando você instala o ini do 0.2.0.
- **Geração multiquadro XeFG** — 3X a 6X vem integrado e ligado por padrão (`XeFG\UnlockMFG`), para
  a cópia do OptiScaler e a do próprio jogo. **Apague `XeFGUnlock.asi`** de `OptiScaler\plugins` se
  ainda o tiver: duas cópias do mesmo patch travam o jogo.
- **FSR Ray Regeneration** — só em jogos que usam DLSS Ray Reconstruction (Cyberpunk 2077, Alan
  Wake 2), com o jogo rodando DLSS (spoofing ligado), ray tracing e Ray Reconstruction ativados nas
  próprias configurações. O Neural Rendering então roda depois dele, sobre a sua saída, o que custa
  mais: abaixe a NR resolution se a taxa de quadros cair.

## Se algo der errado

O `OptiScaler.log` aparece na pasta do jogo. Anexe-o em `#bug-report` e diga qual jogo e qual GPU. O
backend AMD também escreve `amd_presr.log` e `amd_bridge.log`, que são os úteis quando o passo neural
em particular se comporta mal.

**GTA V (Legacy) nunca carrega o OptiScaler como `dxgi.dll`?** O `GTA5.exe` não tem `dxgi.dll` nem `d3d11.dll`
na tabela de importação (ele os carrega do System32 em tempo de execução), então um `dxgi.dll` ao lado dele nunca
é tocado. Nomeie o arquivo `OptiScaler.asi` se usar o ASI loader do ScriptHookV (`dinput8.dll`), ou `winmm.dll` /
`version.dll` caso contrário - esses estão na tabela de importação. Somente modo história.
**Um jogo Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) mostra "DX12 Error 0x80070057"?**
Esses jogos trazem a própria geração de quadros XeSS. Esta build a deixa com eles (a saída XeFG do
OptiScaler fica desativada ali e a aba Frame Gen explica); use a opção XeSS FG do próprio jogo. Se
ainda acontecer, defina `[FrameGen] Enabled=false` e `[fakenvapi] ForceXeLL=false` e relate com o log.

**The Last of Us Part I trava ao iniciar?** É a inicialização do Streamline do próprio jogo, um
problema conhecido do OptiScaler: renomeie o `sl.common.dll` na pasta do jogo para
`sl.common.dll.bak` e escolha **FSR 3.1** nas configurações do jogo em vez de DLSS.

Notas completas desta versão: `RELEASE-NOTES.md` no repositório.

## Roteiro

- **0.3.1** (esta build) — correções dos primeiros relatos do 0.3.0 (lmxxf sozinho nunca rodava, NR
  silencioso no Where Winds Meet, crash ao trocar a qualidade do DLSS, GTA V Legacy) e presets de
  estilo do NR com três slots personalizados.
- **0.3.0** — o runtime neural HIP **lmxxf** (RDNA 4) como runtime selecionável ao
  lado do do danielblnc, distribuído como `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak`: network
  history, Neural passes reais, o edit shaper, a posição depois do Ray Regeneration, diagnóstico por
  jogo e autocorreção. Muito obrigado ao TheAutomatic, em cujo trabalho no DLSS 5 AMD project esta
  integração se apoia.
- **0.4.0** — o AMDNR Launcher (instalação em um clique dos runtimes e do pak, atualizações) e
  suporte a jogos sem upscaler próprio (classe Stray), em que o OptiScaler fornece o upscaler e o
  passo neural juntos.

---

## Créditos

Esta build é um trabalho de ligação sobre o trabalho de outras pessoas. Se ela lhe for útil, os
agradecimentos pertencem ao upstream.

- **DLSS-NR on AMD** — *danielblnc* — <https://github.com/danielblnc/DLSS-NR-on-AMD>
  O runtime neural AMD e os pesos em `Runtime.zip` são a versão v0.3.1 dele, redistribuída sem
  modificações. Tudo o que a rede de fato calcula é dele.
- **DLSS 5 AMD project** — *TheAutomatic* — <https://github.com/TheAutomatic/dlss-5-amd-project>
  Base e referência para o DLSS 5 Neural Rendering em hardware AMD; a integração do runtime lmxxf
  distribuída no 0.3.0 segue o trabalho dele. Muito obrigado.
- **Matheus / dlss-5-amd** — <https://github.com/MatheusGViana/dlss-5-amd-project>
  A ponte AMD pre-SR da qual esta árvore descende.
- **lmxxf / dlss5-on-amd-9070xt-porting** — <https://github.com/lmxxf/dlss5-on-amd-9070xt-porting>
  Runtime de neural rendering HIP de código aberto (MIT); o modo temporal com gate por diferença
  daqui segue o `native_output_smooth` dele.
- **OptiScaler** — *Overclockers* e colaboradores — <https://github.com/Overclockers/OptiScaler-Releases>
  O framework em que isto está construído: os hooks, o encanamento de FSR/XeSS/geração de quadros,
  o menu e a compatibilidade com jogos que torna tudo isto alcançável.

Linhagem do código: OptiScaler → Dagherbou / OptiScaler_DLSSNR → wilsjo2 → esta build. O desbloqueio e o ritmo do XeFG são portados do XeFGUnlock de
Coldwood1026 (GPL-3.0); o tratamento de gama `CubeScale` é do *hhkbble*.

## Aviso legal

Esta build é distribuída sob a licença GPL-3.0 em `LICENSE`; as licenças de bibliotecas de terceiros
estão em `Licenses\`. O runtime neural AMD e seus pesos são redistribuídos sob a autoria original
creditada acima, apenas por conveniência, sem reivindicar propriedade e sem oferecer garantia.

O `nvngx_dlssnr.dll` da NVIDIA não está nestes arquivos. Nada disto é endossado, afiliado ou
suportado pela NVIDIA, pela AMD ou por qualquer distribuidora de jogos. Ele aciona diretamente um
recurso não documentado. Use por sua conta e risco.
