# AMDNR — DLSS 5 Neural Rendering em GPUs AMD (build OptiScaler) — v0.3.3.2

[English](README.md) | [中文](README.zh-CN.md) | **Português** | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

> **Precisamos do seu apoio.** Entre no servidor do Discord — <https://discord.gg/AMDNR> — para
> ajuda, relatos de bugs e builds de teste; cada relato com um log torna a próxima build melhor.

DLSS 5 Neural Rendering rodando em GPUs AMD, integrado ao OptiScaler, para funcionar em qualquer
jogo Direct3D 12 que o OptiScaler já intercepta. Por cima do passo neural: model interleave para um
grande ganho de taxa de quadros, composição residual, geração de quadros XeSS desbloqueada até 6X
(até 10X opcional em jogos D3D12) e FSR Ray Regeneration para jogos que usam DLSS Ray Reconstruction.

**Discord: <https://discord.gg/AMDNR>** — suporte, relatos de bugs (`#bug-report`), builds de
teste.

**Apoie o projeto: <https://ko-fi.com/3zinr>**

> **O runtime do danielblnc é trabalho de Daniel Blanco.** O runtime neural AMD em `Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) é o **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> O AMDNR o distribui sem modificações, com a permissão dele; não é trabalho do AMDNR. Por favor, apoie o projeto dele.
> Os créditos completos de todos os outros estão no fim desta página.

---

## AMDNR - Guia de instalação do OptiScaler

A instalação é bem simples.

### 1. Baixe os arquivos

Baixe estes dois arquivos do GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>):

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

> **Resident Evil Requiem (e a demo) precisa do REFramework.** É um requisito conhecido, não um bug do AMDNR: o OptiScaler depende dele
> para contornar o anti-tamper da Capcom ([wiki do OptiScaler](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-9-Requiem)). Sem ele, o jogo dá crash
> 15-60 s depois de abrir ("An unhandled exception occurred"). Coloque o `dinput8.dll` do `REFramework.zip`, do nightly mais recente
> (<https://github.com/praydog/REFramework-nightly/releases>), ao lado do `dxgi.dll` e troque a tecla do menu do REFramework (por exemplo, para Delete): ela também é Insert.
> Depois de uma atualização do jogo, espere crashes até o REFramework ser atualizado. PRAGMATA, Monster Hunter Wilds e Onimusha provavelmente também precisam dele (não confirmado).

### 5. Inicie o jogo

`HOME` liga e desliga o Neural Rendering durante o jogo (nos dois runtimes; um pequeno aviso
mostra On / Off). Redefina a tecla ao lado da caixa Enable na aba Neural ou em Interface > Keybinds.

É isso.

Inicie o jogo normalmente e pressione:

`INSERT`

Isso abre o menu do OptiScaler / AMDNR, onde você configura o mod como quiser.

### Se não funcionar

Se o jogo ainda não abrir com nenhum dos nomes acima, por favor, relate no canal `#bug-report` do
Discord.

Ao relatar o problema, envie também os arquivos `.log` que tiverem sido gerados na pasta raiz do
jogo.

Esses logs são muito importantes e nos ajudam a identificar o problema bem mais rápido.

> O `.exe` geralmente não está onde o atalho aponta. Jogos em Unreal o guardam em
> `<Game>\Binaries\Win64\`.

---

### O runtime lmxxf (0.3.0, opcional)

Um segundo runtime neural (licença MIT, do lmxxf) pode executar o passo no lugar do runtime do
danielblnc. RDNA 4, e RDNA 3 (RX 7000, Strix Halo) pelo backend RDNA 3 do AMDNR - mais lento lá:
comece com NR resolution em 67%. Ele precisa de duas coisas ao lado do jogo:

1. `LmxxfNrRuntime.dll` - neste arquivo, ao lado de `OptiScaler.dll` (é copiado junto com o resto).
2. `LmxxfNrRuntime.pak` (416 MB, incluído no zip do AMDNR) ao lado de `LmxxfNrRuntime.dll` - os
   arquivos de pesos, os módulos HIP e o HLSL do lmxxf em um único arquivo criptografado e
   autenticado. O runtime o abre em memória; nada é extraído para o disco.

No primeiro início que encontrar um runtime instalado sem escolha feita, o menu pergunta qual usar
(`[DlssNr] NrBackend = daniel | lmxxf` no ini registra a escolha; Neural > Neural runtime a troca,
válida no próximo início do jogo). A edição do lmxxf é aplicada com um quadro de atraso, carregada
pelos vetores de movimento, de modo que o quadro nunca espera pela rede (cerca de 17 ms em 1080p
numa RX 9070 XT). Seu log é o `lmxxf_backend.log` ao lado do jogo.

**Compatibilidade (lmxxf).** O runtime vê apenas o que o DLSS vê, então o que varia por jogo é uma
lista curta: formato de cor e HDR, vetores de movimento e sua escala, profundidade e sua direção, a
máscara reativa, a textura de exposição, o sinal de Reset e onde o passo fica (antes do Super
Resolution ou depois do Ray Reconstruction). Testado até agora:

| Jogo | API / posição | Notas |
|---|---|---|
| Silent Hill 2 | D3D12, antes do SR | jogo de referência; alocação de cor com padding da Unreal tratada |
| Forza Horizon 6 | D3D12, antes do SR | |
| Stray | D3D11 pela ponte D3D12, antes do SR | |
| GTA V Enhanced | D3D12, antes do SR, HDR, máscara reativa de um canal | corrigido no 0.3.0: a máscara era lida como "tudo reativo" e a edição nunca chegava à tela |
| Qualquer jogo com Ray Reconstruction | D3D12, depois do RR (escrito de volta na saída) | suportado desde o 0.3.0; ainda não confirmado em um jogo |

Se um jogo não mostrar efeito: o `lmxxf_backend.log` tem uma linha `lmxxf inputs:` (formatos,
tamanhos, escala de movimento, direção da profundidade, máscara, exposição) e uma linha
`lmxxf stats @N:` a cada 600 quadros (exposição, brilho da entrada, a edição do modelo, a edição
carregada, keep, média reativa, comprimento dos vetores e fração rejeitada). Anexe o log ao
relato; essas duas linhas normalmente dizem o porquê.

Sob o lmxxf, o bloco Neural runtime tem **Network history** (a entrada temporal do próprio modelo),
e Image look tem o grupo **lmxxf edit**: o modelador da edição (Edit detail, Edit colour, Edge guard: ganho
na parte fina da edição do modelo, sua cor em relação à mudança de brilho e um esmaecimento da
edição nas bordas de profundidade) e **Output smoothing** (o passo do lado da saída do upstream,
requer Network history). Neural passes, Residual strength/limit, nitidez, Debug view 1 e o filtro
Appearance valem para os dois runtimes.

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, só no lmxxf) roda os 71 blocos
da rede em vez de pular o 42, o 43 e o 46: um pouco mais fiel, cerca de 0.5 ms mais lento em 1080p
(16.6 -> 17.1 ms numa RX 9070 XT). Desligado por padrão.

## Requisitos

- Uma GPU AMD com driver atual. O runtime neural usa HIP pelo driver; não é preciso HIP SDK nem
  modo desenvolvedor. Quais chips: RX 9000 (RDNA 4) roda os dois runtimes; RX 7000 (RDNA 3, desktop
  e mobile) também roda os dois - o lmxxf pelo backend RDNA 3 do AMDNR, mais lento que no RDNA 4; Strix
  Halo (8060S / 8050S) roda o lmxxf; APUs de portáteis (Z1 Extreme / 780M, Z2 Extreme / 890M) e
  RDNA 2 (RX 6000, Steam Deck) não são suportados por nenhum dos dois. A aba Neural diz o que
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
| `LmxxfNrRuntime.dll` | O runtime neural lmxxf (kernels do lmxxf 0.29). Usado só quando escolhido; lê o `LmxxfNrRuntime.pak` ao lado, veja "O runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Os pesos, módulos HIP e shaders do runtime lmxxf em um arquivo criptografado (416 MB). Só o runtime lmxxf o lê; é inofensivo mantê-lo junto com o runtime do danielblnc. |
| `OptiScaler\` | FSR, XeSS, o denoiser FidelityFX e o D3D12 Agility SDK que o OptiScaler usa. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | O dlssg-to-fsr3 do Nukem9, sem modificações e renomeado: as chamadas de DLSS Frame Generation do jogo servidas pela geração de quadros do FSR 3, também em Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, veja `Licenses/`. |
| `Licenses\`, `LICENSE` | Licenças de terceiros, o aviso do AMDNR (`AMDNR_NOTICE.txt`) e a licença GPL-3.0 desta build. |
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
  mantém o próprio detalhe. Acima de 100% o custo cresce com o quadrado (150% é 2.25x). O controle
  anda em passos de 5%: cada novo tamanho de NR pode reter VRAM até o jogo reiniciar, então reinicie
  o jogo depois de muitas mudanças.
- **Residual strength** — quanto da edição do modelo é aplicado; acima de 1 amplifica. É o controle
  que mais muda a imagem.
- **Residual limit** — um teto para o quanto um pixel pode se mover. Manchas: **abaixe**.
- **Model interleave** — roda o modelo a cada dois quadros para um grande ganho de taxa de quadros.
  Os quadros pulados são preenchidos pelo **Interleave preset**; *Guided fill v2* é o padrão e o que
  está em trabalho ativo. O ritmo dos dois tipos de quadro é automático, e o **Adaptive
  interleave** (ligado por padrão) roda o modelo em todos os quadros enquanto a imagem está em
  movimento, então os pulos - e seus artefatos - só acontecem enquanto a imagem está parada.
- **Neural passes** — 2 e 3 empilham o modelo, com retornos decrescentes. Sob o lmxxf o histórico
  da rede continua sendo o primeiro passe; os passes extras são apenas refinamento espacial.
  O danielblnc roda 1 passe em jogos Vulkan (um aviso abaixo do controle diz isso).
- **Colour composition** (Neural > Image look, nos dois runtimes) — *Classic* (padrão) é a imagem
  que você já tinha. *RenoDX (experimental)* roda a composição de cor do RenoDX depois do modelo,
  como o caminho NVIDIA faz: Composition detail e colour, um **Highlight guard** nos dois sentidos
  (2x por padrão) que limita a resposta do modelo em relação ao original, e controles opcionais de
  pele / ambiente. Num quadro display-referred (SDR) ele volta ao Classic, com um aviso no menu. Os
  estilos e presets de NR não mexem nisso.
- **A geração de quadros vem desligada num ini novo.** Aba Frame Gen: escolha o FG Input (por
  exemplo "DLSSG via Streamline" num jogo com geração de quadros DLSS) e o FG Output (XeFG), depois
  marque **Active** na seção Frame Generation (XeFG) e pressione Save Settings. Um ini 0.1.0 que a
  tinha ligada não é aproveitado quando você instala o ini do 0.2.0.
- **Geração multiquadro XeFG** — 3X a 6X vem integrado e ligado por padrão (`XeFG\UnlockMFG`), para
  a cópia do OptiScaler e a do próprio jogo. **Apague `XeFGUnlock.asi`** de `OptiScaler\plugins` se
  ainda o tiver: duas cópias do mesmo patch travam o jogo.
  **Até 10X é opcional** (só jogos D3D12): escolha *XeFG ceiling (restart)* em FG Output na aba
  Frame Gen (4X, 6X padrão, 8X ou 10X; `[XeFG] MaxInterpolatedFrames`), reinicie e depois escolha o
  multiplicador no combo MFG. Acima de 6X é preciso o provedor XeFG do próprio OptiScaler com Extra
  pacing ligado; a cópia do XeSS 3 do próprio jogo fica em 6X no máximo. 10X exige uma tela de 360 Hz
  ou mais e um limite de quadros em taxa de atualização / 10; a latência é alta e o provedor reserva
  cerca de 128 MiB a mais de VRAM em 4K. 7X-10X ainda não foi confirmado em um jogo: testers, por
  favor, enviem o `OptiScaler.log`.
- **FSR Ray Regeneration** — só RDNA 4 (RX 9000) por padrão; só em jogos que usam DLSS Ray Reconstruction (Cyberpunk 2077, Alan
  Wake 2), com o jogo rodando DLSS (spoofing ligado), ray tracing e Ray Reconstruction ativados nas
  próprias configurações. O Neural Rendering então roda depois dele, sobre a sua saída, o que custa
  mais: abaixe a NR resolution se a taxa de quadros cair. Os controles dele (Neural > Quality > Ray
  Regeneration) só aparecem enquanto o jogo está usando Ray Reconstruction. O **perfil path-traced**
  (menos granulado nos rostos com path tracing) é opcional desde o 0.3.3.1: marque-o ali para testá-lo
  em Resident Evil Requiem ou PRAGMATA. No mesmo lugar ficam a intensidade da bias mask, uma
  visualização de depuração do RR e a **suavização de pele** (experimental, para jogos que publicam um
  guia SSS; desligada por padrão, mas ligada por padrão em Resident Evil Requiem desde o 0.3.3.2).

## Se algo der errado

O `OptiScaler.log` aparece na pasta do jogo. Anexe-o em `#bug-report` e diga qual jogo e qual GPU. O
backend AMD também escreve `amd_presr.log` e `amd_bridge.log`, que são os úteis quando o passo neural
em particular se comporta mal. O log da sessão anterior fica guardado como
`OptiScaler.previous.<exe>.log`; depois de um crash, anexe-o também (o log novo então diz "no clean
exit recorded").

**NR frames 0/s, e a aba Neural ou o `amd_presr.log` diz que a DLL do passe é uma build que este AMDNR não
aciona?** Seus `dlssnr_amd_pass1..3.dll` são uma build do danielblnc que este AMDNR não conhece (um conjunto
0.2.16 foi visto por aí), ou falta uma das três. Desde o 0.3.3.2 a aba Neural mostra o nome do arquivo e a
versão e diz o que fazer. Use o `v0.4.0-Runtime.zip` (o mais novo) ou o `Runtime.zip` (0.3.1) desta release,
as três DLLs de passe do mesmo zip: o `dlssnr_amd_pass1.dll` do `v0.4.0-Runtime.zip` tem 10,027,008 bytes,
SHA256 começando com `d62be3d8`. Builds suportadas: 0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0, e 0.4.1 / 0.4.2
antes do lançamento delas. Não instale o instalador próprio do danielblnc nem o `dxgi.dll` / `version.dll` /
`winhttp.dll` dele junto do AMDNR: o AMDNR já roda o runtime dele.

**O lmxxf não faz nada, ou para na hora, num PC com gráficos integrados?** Corrigido no 0.3.3.2. Num Ryzen
de desktop com os gráficos integrados ligados, num notebook com APU AMD e uma Radeon, ou num PC com duas GPUs
AMD, a GPU do jogo muitas vezes não é o dispositivo HIP 0. O lmxxf então falhava no primeiro quadro
(`hipErrorInvalidHandle (400)`, depois "session is poisoned" no `lmxxf_backend.log`) e ficava desligado.
Substitua tanto o `OptiScaler.dll` (o arquivo que você renomeou, por exemplo `dxgi.dll`) quanto o
`LmxxfNrRuntime.dll` pelos do 0.3.3.2. Ainda não testado num PC assim: se o lmxxf continuar parando, a aba
Neural agora diz o motivo; envie o `lmxxf_backend.log` e o `amd_bridge.log` (ele lista os dispositivos HIP).

**Um jogo Vulkan (Indiana Jones and the Great Circle) para na inicialização com "Could not create the
Vulkan device (VK_ERROR_EXTENSION_NOT_PRESENT)"?** Corrigido no 0.3.2: o caminho neural herdado da
NVIDIA pedia ao driver AMD duas extensões de dispositivo exclusivas da NVIDIA. Títulos Vulkan chegam ao
passo neural pela ponte D3D12 do OptiScaler (veja Requisitos).

**O lmxxf travava um jogo Vulkan no primeiro quadro de NR?** Corrigido no 0.3.3; espere uma pausa única de
cerca de 1 s quando o NR começa. Se uma sessão Vulkan parar antes da primeira resposta do lmxxf, o próximo
início usa o runtime do danielblnc e a aba Neural diz o motivo; pressione **Retry lmxxf** ali (ele apaga o
`lmxxf_vk_launch.pending` ao lado do `OptiScaler.dll`) para tentar o lmxxf de novo.

**O danielblnc pausava por segundos e depois parava o NR num jogo Vulkan (Indiana Jones) com 2-3 Neural
passes?** Corrigido no 0.3.3: em jogos Vulkan ele roda 1 passe, e a espera de 80 ms após o envio acabou. O
primeiro quadro de NR de uma sessão ainda pausa cerca de 5 s; um aviso abaixo da escolha de runtime
explica as linhas de log dele. Testers: `[DlssNr] AmdVkLateCopyWait=true` (experimental, desligado por
padrão, ainda não testado em um jogo) deve remover essa pausa; enviem `OptiScaler.log`, `amd_presr.log` e
`dlssnr_on_amd.log`.

**O uso de RAM do lmxxf subia enquanto o NR rodava?** Corrigido no 0.3.3 (eram cerca de 45 GB por hora a
60 quadros de NR por segundo). O que resta: o danielblnc retém VRAM a cada novo tamanho de NR acima de
cerca de 1 MP (desde o 0.3.3.2 os tamanhos são arredondados para 64 px fora dos 100%, então são poucos);
reinicie o jogo depois de muitas mudanças com o danielblnc. Desde o 0.3.3.2, o lmxxf não retém mais cerca de
97 MB a cada mudança de NR resolution ou de modo DLSS: ele cria os buffers da rede uma vez por tamanho de rede e
os reutiliza (resta uma pequena sobra de cerca de 10-25 MB de VRAM por mudança).

**Um jogo com Streamline falha ao iniciar com o erro 0x18 do slInit (visto com NBA 2K27 em AMD)?** O 0.3.3
fecha um caminho pelo qual os hooks de plugins do Streamline do OptiScaler podiam causá-lo, mas não está
confirmado que seja a causa no NBA 2K27. O `OptiScaler.log` agora registra linhas `slInit returned ...` e
`[SLINIT]`: envie o log junto com o relato.

**O Ray Reconstruction do jogo está ligado, mas a aba Neural diz "Ray Regeneration is off in this title"?**
O jogo não publica o que o FSR Ray Regeneration precisa (Satisfactory: sem matrizes de câmera). O upscaling
FSR roda no lugar dele e o NR assume sua posição normal antes do SR; nada no ini muda isso.

**Um jogo Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) mostra "DX12 Error 0x80070057"?**
Esses jogos trazem a própria geração de quadros XeSS. Esta build a deixa com eles (a saída XeFG do
OptiScaler fica desativada ali e a aba Frame Gen explica); use a opção XeSS FG do próprio jogo. Se
ainda acontecer, defina `[FrameGen] Enabled=false` e `[fakenvapi] ForceXeLL=false` e relate com o log.

**The Last of Us Part I trava ao iniciar?** É a inicialização do Streamline do próprio jogo, um
problema conhecido do OptiScaler: renomeie o `sl.common.dll` na pasta do jogo para
`sl.common.dll.bak` e escolha **FSR 3.1** nas configurações do jogo em vez de DLSS.

Notas completas de cada versão: `CHANGELOG.md` (no zip e no repositório).

## Roteiro

- **0.3.3** (esta build) — lmxxf no RDNA 3 (RX 7000; backend próprio do AMDNR); composição de cor
  RenoDX (experimental, opcional) nos dois runtimes; lmxxf: opção Full network, vazamento de RAM
  corrigido, títulos Vulkan corrigidos (upload preguiçoso de pesos dentro da ponte Vulkan), kernels do
  0.29 (idênticos bit a bit, mais rápidos); danielblnc em títulos Vulkan: 1 Neural pass, mensagens mais
  claras, uma espera de cópia tardia opcional; XeFG até 10X (opcional, D3D12); inicialização do
  Streamline reforçada e diagnósticos; perfil path-traced e suavização de pele do FSR Ray
  Regeneration; robustez em UE5.
- **0.3.2** — os relatos do 0.3.1: títulos Vulkan iniciam e rodam com lmxxf, cores do lmxxf
  alinhadas às do danielblnc (auto-exposição), o combo de runtime, status e ajuste do Ray Reconstruction;
  o dlssg-to-fsr3 do Nukem9 no zip para geração de quadros em Vulkan.
- **0.3.1** — correções dos primeiros relatos do 0.3.0 (lmxxf sozinho nunca rodava, NR
  silencioso no Where Winds Meet, crash ao trocar a qualidade do DLSS, GTA V Legacy) e presets de
  estilo do NR com três slots personalizados.
- **0.3.0** — o runtime neural HIP **lmxxf** (RDNA 4) como runtime selecionável ao
  lado do do danielblnc, distribuído como `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak`: histórico
  da rede, Neural passes reais, o modelador da edição, a posição depois do Ray Regeneration, diagnóstico por
  jogo e autocorreção. Muito obrigado ao TheAutomatic, em cujo trabalho no DLSS 5 AMD project esta
  integração se apoia.
- **0.4.0** — o AMDNR Launcher (instalação em um clique dos runtimes e do pak, atualizações) e
  suporte a jogos sem upscaler próprio (classe Stray), em que o OptiScaler fornece o upscaler e o
  passo neural juntos.

---

## Créditos

Esta build é um trabalho de ligação sobre o trabalho de outras pessoas. Se ela lhe for útil, os
agradecimentos pertencem ao upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD — https://github.com/danielblnc/DLSS-NR-on-AMD (`Runtime.zip`, sem modificações)
- **lmxxf** — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (o runtime HIP, MIT)
- **kernels c32w** (0.3.3.2) — kernels RDNA 4 de uma wave do próprio AMDNR para a rede do lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); ideias da documentação pública de WMMA do RDNA 4 da AMD (GPUOpen, ROCm matrix instruction calculator)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, sem modificações)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (a matemática da composição de cor, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), a base do desbloqueio multiquadro do XeFG integrado e do seu ritmo
- **burak113** — o pré-processador do FSR Ray Regeneration (branch ffx-denoise-experimental do OptiScaler, GPL-3.0)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## Copyright / Licença

O AMDNR é Copyright (c) 2026 3zwr1 (AMDNR). É um fork do OptiScaler, distribuído sob a licença GPL-3.0
em `LICENSE`.

O trabalho próprio do AMDNR tem um termo adicional pela seção 7(b) da GPL-3.0 (veja
`Licenses/AMDNR_NOTICE.txt`): qualquer cópia, fork ou obra derivada que o use deve manter seus avisos e
dar crédito a **AMDNR by 3zwr1** (<https://github.com/3zwr1/AMD-NR---OptiScaler>).

**Copyright do menu do AMDNR.** O menu do AMDNR — o layout, o design, os textos e o código que o AMDNR adicionou para ele — é Copyright (c) 2026 3zwr1 (AMDNR). Ele faz parte deste fork GPL-3.0, com estes termos adicionais (GPL-3.0 section 7): (b) quem reutilizar qualquer parte dele deve manter esta linha de copyright e dar crédito a AMDNR by 3zwr1 de forma visível, no menu e no README; (c) você não pode apresentá-lo, nem uma cópia modificada dele, como trabalho seu; versões modificadas devem ser claramente marcadas como alteradas; (e) nenhum direito é concedido sobre o nome ou o logo do AMDNR; outros projetos não podem usá-los.

O trabalho do upstream creditado acima continua sendo dos seus autores, sob as próprias licenças; o
AMDNR não reivindica copyright sobre ele.

O código-fonte será publicado com o AMDNR 0.5.0.

## Aviso legal

Esta build é distribuída sob a licença GPL-3.0 em `LICENSE`; as licenças de bibliotecas de terceiros
estão em `Licenses\`. O runtime neural AMD e seus pesos são redistribuídos sob a autoria original
creditada acima, apenas por conveniência, sem reivindicar propriedade e sem oferecer garantia.

O `nvngx_dlssnr.dll` da NVIDIA não está nestes arquivos. Nada disto é endossado, afiliado ou
suportado pela NVIDIA, pela AMD ou por qualquer distribuidora de jogos. Ele aciona diretamente um
recurso não documentado. Use por sua conta e risco.
