# AMDNR — DLSS 5 Neural Rendering em GPUs AMD (build OptiScaler) — v0.3.4.2

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

> **O runtime do danielblnc é trabalho de Daniel Blanco.** O runtime neural AMD dos arquivos `*Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) é o **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> O AMDNR o distribui sem modificações, com a permissão dele; não é trabalho do AMDNR. Por favor, apoie o projeto dele.
> Os créditos completos de todos os outros estão no fim desta página.

> **Novidades do 0.3.4.2 (hotfix):** o menu no Assetto Corsa: a tecla do menu abre ou fecha o menu uma só vez por
> toque, cliques mais curtos que um quadro não se perdem mais, e o seletor de runtime responde a `1` / `2` / `Enter` /
> `Esc`, tem um X na barra de título e fechar o menu conta como "Decide later". O seletor de runtime não abre mais o
> menu sozinho (um aviso no lugar), a seção **Ray Regeneration** da aba Neural não se esconde mais — ela está sempre
> ali e uma linha esmaecida diz por que não está rodando — e o texto sobre Wine / Proton diz que o Ray Regeneration é um
> problema conhecido lá. O AMDNR também **aceita um layout de runtime do danielblnc a
> mais**, então uma build mais nova do danielblnc poderá rodar aqui sem nenhuma atualização do AMDNR.
> **O Neural Rendering é idêntico byte a byte ao 0.3.4.1, com a exceção dessa única linha de layout aceito** (o passe
> neural, os dois runtimes e o pak não mudam):
> vindo do 0.3.4.1 ou do 0.3.4, substitua só o `OptiScaler.dll`; usuários do launcher: ele se atualiza sozinho.
> Detalhes: `CHANGELOG.md`.

> **Novidades do 0.3.4.1 (hotfix):** o Ray Regeneration ficou menos suave no Windows (nitidez de 0.25 quando o jogo
> não envia nenhuma; para desligar: Image > Sharpness, marque Override, controle deslizante em 0); sem o falso pop-up
> "Upscaler failed to run!" no Control Resonant; no Linux / Proton o menu funciona (confirmado por um jogador, também
> com a geração de quadros ligada) e o Ray Regeneration não adiciona nitidez padrão ali (veja "Linux / Proton").
> **AMDNR Launcher 0.3.4.1**, feito a partir do feedback de vocês no Discord: nove idiomas, busca, favoritos,
> ocultar, renomear, CHOOSE GAME .EXE, PLAY, um UNINSTALL completo e mais (veja "AMDNR Launcher"). O Neural
> Rendering não mudou desde o 0.3.4 (mesmo runtime e mesmo pak): vindo do 0.3.4, substitua só o `OptiScaler.dll`;
> quem usa o launcher: ele atualiza por você. Detalhes: `CHANGELOG.md`.

> **Novidades do 0.3.4:** um menu novo (a aba Neural refeita, o mesmo visual em todas as abas e um botão **Save
> report** que compacta seus logs para um relato); o lmxxf ficou mais rápido na RX 7000 (1440p FSR Quality: 73.3 ->
> 52.2 ms por execução da rede numa RX 7800 XT, tempo de rede medido fora de um jogo) e na RX 9070 / 9070 XT (kernels do lmxxf 0.31);
> o lmxxf roda em APUs de portáteis (experimental; o teste de um tester, num jogo: cerca de 29 fps no Shadow of the Tomb Raider num ROG Ally); um
> **Fast mode** opcional para o lmxxf; **AMDNR Screen GI**, a GI em espaço de tela do próprio AMDNR (preview,
> desligada por padrão); e muitas correções. Substitua `OptiScaler.dll`,
> `LmxxfNrRuntime.dll` e `LmxxfNrRuntime.pak` juntos. Detalhes: `CHANGELOG.md`.

---

## AMDNR - Guia de instalação do OptiScaler

A instalação é bem simples. **No Windows, o AMDNR Launcher faz tudo isso por você** (veja "AMDNR Launcher"
abaixo). Para fazer à mão:

### 1. Baixe os arquivos

Baixe estes arquivos da release mais recente no GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>;
o 0.3.4.2 é a tag Alpha0.3.4.2):

* `AMDNR-vX.X.X.zip` (no 0.3.4.2: `AMDNR-v0.3.4.2.zip`), com o runtime lmxxf completo.
* Para o runtime do danielblnc, um zip de runtime: na **RX 9000 e na RX 7000**, o `v0.4.3-Runtime.zip`
  (recomendado), da Alpha0.3.4.1; o `v0.4.1-Runtime.zip` e o `v0.4.0-Runtime.zip` da mesma release continuam
  aceitos. O runtime lmxxf vem
  no `AMDNR-vX.X.X.zip` e não precisa de zip de runtime na RX 7000 e na RX 9000; APUs de portáteis usam só o
  lmxxf. O AMDNR Launcher escolhe o zip certo para a sua GPU. Veja "O que há nos arquivos".

### 2. Extraia os dois arquivos

Extraia o conteúdo dos dois arquivos `.zip`.

### 3. Copie tudo para a pasta do jogo

Primeiro, copie todos os arquivos de `AMDNR-vX.X.X` para a pasta raiz do jogo — a mesma pasta onde
fica o `.exe` do jogo.

Depois, faça o mesmo com todos os arquivos do zip de runtime (por exemplo `v0.4.3-Runtime` na RX 9000 e na RX 7000).

> **Atualizando de um AMDNR anterior?** Copie tudo de novo, sobrescrevendo. No 0.3.4 três arquivos mudaram juntos:
> `OptiScaler.dll` (substitua o arquivo que você renomeou, por exemplo `dxgi.dll`, pelo novo renomeado do mesmo
> jeito), `LmxxfNrRuntime.dll` e `LmxxfNrRuntime.pak` (440 MB, novo no 0.3.4). Não os misture com
> cópias antigas. Você pode manter o seu `OptiScaler.ini`: as configurações novas usam os valores padrão.
> **Do 0.3.4 para o 0.3.4.1 e o 0.3.4.2 só o `OptiScaler.dll` mudou:** substitua esse único arquivo; o `LmxxfNrRuntime.dll`,
> o `LmxxfNrRuntime.pak` e os arquivos do seu zip de runtime ficam como estão. O AMDNR Launcher faz isso por você:
> clique em REPAIR / UPDATE num jogo que mostra "Update available".

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
* `winhttp.dll`
* `wininet.dll`

Teste um nome por vez. Não crie várias cópias do `OptiScaler.dll`. Esses são os nomes com que o mod carrega (mais
`OptiScaler.asi` com um carregador ASI); `d3d11.dll` não é um deles.

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

**O jeito mais fácil: Save report.** Se o menu abrir, clique em **Save report** (a última linha de Neural > Diagnostics, ou a primeira de Advanced > Logging). Ele grava
um zip, `AMDNR-report-<exe do jogo>-<data>.zip`, na pasta do jogo (na Área de Trabalho se a pasta do jogo for somente
leitura, senão em `%TEMP%`), com o `report.txt`, os logs e os arquivos ini, e o menu mostra onde ele ficou. Seu nome
de usuário do Windows e o nome do PC são trocados por marcadores; um nome dentro de um caminho do jogo fora de
`C:\Users\` não é. Anexe o zip em `#bug-report`.

> O `.exe` geralmente não está onde o atalho aponta. Jogos em Unreal o guardam em
> `<Game>\Binaries\Win64\`.

---

### O runtime lmxxf (0.3.0, opcional)

Um segundo runtime neural (licença MIT, do lmxxf) pode executar o passo no lugar do runtime do
danielblnc. O RDNA 4 o roda nativamente; o RDNA 3 (RX 7000, Strix Halo) o roda pelo backend RDNA 3 do AMDNR, por
3zwr1 - mais lento lá, veja "RX 7000" abaixo: comece com NR resolution em 70% ou menos. APUs de portáteis também o
rodam, de forma experimental (veja "APUs de portáteis" abaixo). Ele precisa de duas coisas ao lado do jogo:

1. `LmxxfNrRuntime.dll` - neste arquivo, ao lado de `OptiScaler.dll` (é copiado junto com o resto).
2. `LmxxfNrRuntime.pak` (440 MB, incluído no zip do AMDNR) ao lado de `LmxxfNrRuntime.dll` - os
   arquivos de pesos, os módulos HIP e o HLSL do lmxxf em um único arquivo criptografado e
   autenticado. O runtime o abre em memória; nada é extraído para o disco.

No primeiro início que encontrar um runtime instalado sem escolha feita, o menu pergunta qual usar
(`[DlssNr] NrBackend = daniel | lmxxf` no ini registra a escolha; Neural > Neural runtime a troca,
válida no próximo início do jogo). A edição do lmxxf é aplicada com um quadro de atraso, carregada
pelos vetores de movimento, de modo que o quadro nunca espera pela rede (cerca de 14.1 ms de tempo de
rede em 1080p numa RX 9070 XT). Seu log é o `lmxxf_backend.log` ao lado do jogo.

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

Os dois runtimes compartilham uma única aba Neural (veja "O menu" abaixo). Os controles que o runtime ativo não tem
aparecem em cinza com uma etiqueta curta, ou ficam ocultos com uma contagem. Só do lmxxf: **Full network**,
**Output smoothing** (Quality > More quality options, requer Network history), **Edit detail**, **Edit colour** e
**Edge guard** (Image look > Model strength: ganho na parte fina da edição do modelo, sua cor em relação à mudança
de brilho e um esmaecimento da edição nas bordas de profundidade) e o limite de altas luzes da auto-exposição.
Novo no lmxxf no 0.3.4: Network output, Encoding, Residual edge fade, Game exposure, Fast mode, a leitura
do ritmo do interleave e a máscara nativa de personagens do modelo com Structure intensity e Character
structure (cada mudança reconstrói a rede: uma travadinha de cerca de 1 s).

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, só no lmxxf) roda os 71 blocos
da rede em vez de pular o 42, o 43 e o 46: um pouco mais fiel, cerca de 0.5 ms mais lento em 1080p
(16.6 -> 17.1 ms numa RX 9070 XT, medido no 0.3.3). Desligado por padrão.

**Fast mode** (Neural > Performance, `[DlssNr] AmdLmxxfFastMode`, lmxxf, opcional, desligado por padrão) roda a
rede um nível de tamanho abaixo (1080 -> 900, 900 -> 720): cerca de 29% menos tempo de rede em 1080p (RX 9070 XT,
medido fora de um jogo), com os detalhes finos um pouco mais suaves. Builds do danielblnc que têm o próprio Fast mode
também mostram ali uma linha Fast mode (`[DlssNr] AmdDanielFastMode`); os runtimes dos zips de runtime desta release
não têm, então a linha fica oculta.

### RX 7000 (RDNA 3): mais rápido com o nível de tamanho da rede (novo no 0.3.4)

A rede do lmxxf roda em poucos tamanhos fixos (níveis): 720 (1280x720), 900 (1600x900) e 1080 (1920x1080), mais
576 e 360 (novos, usados em portáteis). Um nível custa o mesmo, qualquer que seja a parte dele que a imagem
preenche. No RDNA 3 (RX 7000, Radeon 8060S / 8050S e as APUs de portáteis) o tamanho de NR do lmxxf agora se
encaixa por padrão num nível: desce para o nível menor seguinte quando está mais perto dele (mais barato), senão
cresce até preencher o próprio nível (mesmo custo, um pouco mais de detalhe), nunca acima do tamanho do próprio
quadro.

Tempo de rede por execução numa RX 7800 XT (medido por um tester com a sonda do lmxxf; só a rede, média de 30
execuções; o tempo do nível 900 foi medido em 1600x900):

| Configuração do jogo | 0.3.3.2 | 0.3.4 na RX 7000 |
|---|---|---|
| 1440p, FSR Quality (render 1706x960), NR 100% | nível 1080: 73.3 ms | nível 900: 52.2 ms |
| Render 1080p, NR 85% | nível 1080: 73.2 ms | nível 900: 52.2 ms |
| Render 1080p, NR 70% | nível 900: 52.2 ms | nível 720: 34.4 ms |
| Render 1080p, NR 80% | nível 900: 52.2 ms | nível 900, preenchido: 52.2 ms (mais detalhe) |
| Render 1080p, NR 100% | nível 1080: 73.2 ms | sem mudança |

- No jogo o ganho por quadro exibido é menor: com Model interleave a rede roda a cada 2 quadros, e o jogo tem o
  próprio custo. Ainda não medido num jogo.
- A rede vê uma imagem um pouco menor (em 1440p Quality, cerca de 6% menos pixels por lado), então detalhes finos
  podem ficar um pouco mais suaves. `[DlssNr] AmdLmxxfTierSnap=false` volta aos tamanhos do 0.3.3.2. A RX 9000
  mantém os tamanhos do 0.3.3.2, a menos que você o defina como `true`.
- **RX 9000:** `[DlssNr] AmdLmxxfTierSnap=true` (desligado por padrão lá) leva o tamanho de NR do lmxxf a um tamanho
  de rede em toda resolução de render: alguns tamanhos descem um nível (1440p FSR Quality, 1707x960 -> o tamanho 900:
  rede 14.08 -> 9.96 ms por execução numa RX 9070 XT, medido fora de um jogo, imagem um pouco mais suave), outros
  crescem dentro do seu nível (80% de um render 1080p -> 1600x900: mesmo custo, um pouco mais de detalhe). Sem
  definir, a RX 9000 mantém os tamanhos do 0.3.3.2.
- Comece com NR resolution em 70% ou menos (o nível 720 com render em 1080p; o preset Performance é 70%). O custo
  ao lado de NR resolution é calculado pelo nível em que a rede roda; a dica dele mostra o nível.

### APUs de portáteis (experimental, novo no 0.3.4)

O lmxxf roda em APUs de portáteis com 12 ou mais unidades de computação, pelo backend RDNA 3 do AMDNR, por 3zwr1:
**Z1 Extreme, Z2 e Radeon 780M** (gfx1103), **Z2 Extreme, Radeon 890M e 880M** (gfx1150). É experimental e lento. A linha Neural runtime diz "experimental" depois do crédito do RDNA 3.
Primeiros resultados de um tester (ROG Ally, Z1 Extreme): a sonda do lmxxf fora de um jogo, 54.7 ms por execução da
rede no tamanho 360p, 110.9 ms em 576p; num jogo, o teste de um tester (Shadow of the Tomb Raider, 1280x720 com XeSS, preset Handheld),
62 ms por execução da rede em média em 360p com o modelo a cada 4 quadros, cerca de 29 fps com o NR ligado.

- **Não suportados:** Z1 e Radeon 740M (4 unidades de computação), Radeon 760M (8), Radeon 860M / 840M. O runtime
  do danielblnc não roda em APUs de portáteis. A RX 6000 (RDNA 2) está planejada para o 0.4.0; o Steam Deck e
  outras APUs RDNA 2 não são suportados.
- **O que ele faz sozinho** (só enquanto o seu ini não tiver um valor próprio): a rede roda no menor tamanho,
  360p (640x360), e o modelo roda a cada 4 quadros (Model interleave; não é salvo). Neural passes continua em 1.
- **Velocidade, com honestidade:** Como
  referência: uma RX 7800 XT (60 unidades de computação) precisa de 34.4 ms por execução da rede no tamanho 720;
  esses chips têm de 12 a 16 e rodam com clocks mais baixos. Espere uma grande queda de quadros mesmo em 360p com
  o modelo a cada 4 quadros, algum ghosting pelo interleave longo e uma imagem mais suave que numa GPU de desktop.
  O custo de NR no fim da linha de status da aba Neural (e em Diagnostics) mostra o número real no seu aparelho.
- **Configurações:**
  - Mais nítido, porém mais lento: `[DlssNr] AmdLmxxfTierCap=576` (o tamanho de rede 1024x576).
  - Com render em 720p ou 800p, NR resolution em 100% já alimenta o tamanho 360p, então uma NR resolution menor
    não barateia.
  - Model interleave em Off é salvo como `[DlssNr] AmdInterleave=1` (também desligado), para que o padrão do
    portátil não volte no próximo início. Para desligar à mão, escreva 1, não 0.
  - Preset > **Handheld** define NR resolution em 100%, Dynamic NR desligado, o modelo a cada 4 quadros, 1 Neural pass e Full network desligado. O botão só aparece nessas APUs; Quality, Balanced e Performance também mantêm aqui o tamanho de rede 360p (o menu avisa).
- **FSR 4:** nesses chips (e nas Radeon 780M / 760M / 740M em geral) o FSR 4 INT8 não liga mais sozinho: com
  `Dx12Upscaler=auto` o upscaler é o XeSS, e o FSR 3.1 continua FSR 3.1. `[FSR] Fsr4ForceModel=2` ainda o força
  (experimental).
- **Shadow of the Tomb Raider** (e jogos que criam o device D3D12 duas vezes) não trava mais quando o upscaler
  inicia (corrigido no 0.3.4).
- **Driver:** use o driver Adrenalin da própria AMD. O lmxxf precisa do HIP (`amdhip64_7.dll`), que alguns drivers
  de fabricantes de portáteis não trazem; o `amd_bridge.log` então diz que o HIP não está disponível.
- **Use os arquivos do 0.3.4 juntos** (o 0.3.4.1 e o 0.3.4.2 mudam só o `OptiScaler.dll`): só o pak do 0.3.4 tem os módulos
  para portáteis, e o `LmxxfNrRuntime.dll` do 0.3.4 recusa um portátil quando o `OptiScaler.dll` é anterior ao
  0.3.4 ("this handheld needs OptiScaler.dll 0.3.4 or newer").
- **Testers com um portátil:** peça no Discord o kit de teste para portáteis (`handheld-test.zip`). O
  `run_probe.bat` dele mede a rede no seu aparelho e grava `handheld_result.txt` (seu nome de usuário do Windows
  fica mascarado).

## Linux / Proton (Steam Deck, Linux de desktop)

O AMDNR roda sob Proton e Wine como uma build do OptiScaler. **O Neural Rendering não roda no Linux (só no Windows):**
os dois runtimes de NR precisam do HIP da AMD, e o Proton e o Wine não fornecem HIP. Com o NR ligado sob Proton, o NR
não roda, e a aba Neural nem sempre diz por quê. Isso é esperado, não é um crash nem uma instalação quebrada. O AMDNR
Launcher é um programa para Windows que pode rodar sob Proton (experimental, ainda não testado por nós, veja
abaixo); a instalação à mão funciona sem ele.

**O que funciona:** os upscalers FSR (FSR 3.1, e FSR 4 em GPUs e drivers que o suportam), o menu (`INSERT`) e o
**Save report**. Um jogador confirmou no Proton da Steam (RX 9070 XT, vkd3d-proton, Resident Evil Requiem) que o jogo
inicia, o menu abre e recebe a entrada do mouse, e o Save report funciona, também com a geração de quadros ligada.

**Ray Regeneration e geração de quadros:**
- Problema conhecido: o Ray Regeneration pode mostrar manchas rosa / magenta no Proton; a nitidez padrão agora está desligada ali, mas se você ainda as vir, use o FSR puro (FSR 4 na RX 9000) e envie um Save report.
  No Proton, o AMDNR não adiciona nitidez depois do Ray Regeneration quando o jogo não envia nenhuma (no Windows ele
  adiciona 0.25): Image > Sharpness mostra "RR default 0 (off on Proton)", e o Override continua definindo o seu
  próprio valor.
- **A geração de quadros** agora liga sem dar crash, mas os contadores de fps contam também os quadros gerados: com um
  limite de 60 fps ou V-Sync a 60 Hz, são 30 quadros reais, o que parece 30 fps. Deixe-a desligada no Proton por
  enquanto (`[FrameGen] FGOutput=nofg`), ou use-a só quando o jogo chega a cerca de 60 fps sem ela, numa tela acima
  de 60 Hz.

**Requisitos:** um Proton ou Wine atual (testado: Proton 11, que é o Wine 11), com o jogo no vkd3d-proton (D3D12) ou
no DXVK (D3D11), que é o padrão do Proton. Versões mais antigas não foram testadas.

**O AMDNR Launcher no Linux (experimental, ainda não testado por nós).** O launcher é o mesmo programa para
Windows, `AMDNR-Launcher.exe` (autocontido: não é preciso instalar .NET nem outro runtime). Sob Wine / Proton ele
detecta o Wine e mostra um aviso com o que fazer. Ele também procura a sua biblioteca Steam do Linux pela unidade
`Z:` do Wine (`~/.steam/steam` e `~/.local/share/Steam`, e as pastas de biblioteca em `libraryfolders.vdf`). Ainda
não testamos isso nós mesmos: se você tentar, conte para a gente no Discord se funciona. Para tentar:

1. Na Steam, adicione o `AMDNR-Launcher.exe` como jogo não Steam (no menu **Jogos**, a opção de adicionar um jogo
   não Steam à biblioteca; em inglês: **Games > Add a Non-Steam Game to My Library**).
2. Nas **Propriedades > Compatibilidade** dele (em inglês: Properties > Compatibility), force uma versão do Proton
   (Proton Experimental) e depois inicie-o pela Steam.
3. Se o jogo não estiver em **LIBRARY** (BIBLIOTECA), clique em **ADD** (ADICIONAR) e escolha a pasta do jogo (as
   suas pastas do Linux ficam na unidade `Z:`); se o launcher escolher o `.exe` errado, use **CHOOSE GAME .EXE**
   (ESCOLHER O .EXE DO JOGO).
4. Selecione o jogo e clique em **INSTALL** (INSTALAR).
5. Em **Propriedades > Geral > Opções de inicialização** do jogo (em inglês: Properties > General > Launch
   Options), digite `WINEDLLOVERRIDES="dxgi=n,b" %command%` (se o launcher usou outro nome de DLL para o jogo,
   coloque esse nome no lugar de `dxgi`) e depois siga os passos 5 e 6 da instalação à mão abaixo (inicie o jogo
   pela Steam; o botão **PLAY** do launcher fica desativado sob Wine / Proton).

**Instalação à mão** (sem o launcher):

1. Baixe o `AMDNR-vX.X.X.zip` da página de releases (no 0.3.4.2: `AMDNR-v0.3.4.2.zip`). Os zips de runtime do
   danielblnc (`v0.4.3-Runtime.zip` e os outros) só são usados pelo Neural Rendering, então você não precisa deles
   no Linux (copiar um não faz mal).
2. Extraia o zip e copie tudo para a pasta do jogo, ao lado do `.exe` do jogo.
3. Renomeie o `OptiScaler.dll` para `dxgi.dll`.
4. Na Steam, abra **Propriedades > Geral > Opções de inicialização** do jogo (em inglês: Properties > General >
   Launch Options) e digite:

   ```
   WINEDLLOVERRIDES="dxgi=n,b" %command%
   ```

   Isso diz ao Wine para carregar o `dxgi.dll` da pasta do jogo em vez do dele; sem isso o AMDNR não carrega.
   Se você usou outro nome (por exemplo `winmm.dll` ou `version.dll`), coloque esse nome no lugar de `dxgi`, por
   exemplo `WINEDLLOVERRIDES="winmm=n,b" %command%`. Lutris, Heroic e Bottles: adicione o mesmo override (`dxgi` =
   `native,builtin`) nas configurações de DLL overrides ou de variáveis de ambiente do runner.
5. No `OptiScaler.ini`, defina `[FrameGen] FGOutput=nofg` (geração de quadros desligada, veja acima).
6. Inicie o jogo e pressione `INSERT` para abrir o menu. Configure o upscaler ali.

**O menu.** No 0.3.4, com a geração de quadros ligada, o menu podia abrir sem receber a entrada do mouse ou do
teclado, ou nem abrir. O 0.3.4.1 vincula o menu à janela do jogo; um jogador confirmou no Proton que o menu abre e
recebe a entrada do mouse, também com a geração de quadros ligada. Se ainda acontecer no seu sistema, o AMDNR mostra
o aviso "Menu window lost". Então, no `OptiScaler.ini`, defina `[FrameGen] FGOutput=nofg`; se o menu ainda não
responder, defina também `[Menu] OverlayMenu=false` (o menu clássico, que não depende da janela de overlay).

**HDR.** O AMDNR não liga o HDR sob Proton. O HDR vem da sua configuração do Proton e do desktop: uma build do Proton
com suporte a HDR e uma sessão que consiga mostrar HDR (por exemplo o gamescope, ou um desktop Wayland com o HDR
ligado). Se o HDR funciona no jogo sem o AMDNR, ele continua funcionando com o AMDNR; se a opção de HDR do jogo está
em cinza, a correção está na sua configuração do Proton ou do desktop.

**Relatando um problema no Linux:** use o botão **Save report** do menu (mantenha `[Log] LogToFile=true`, o padrão,
para que o relatório tenha o log desta sessão); o relatório mostra se o jogo rodou sob Wine/Proton, vkd3d-proton ou
DXVK. Informe também a sua distribuição, GPU, versão do Mesa e versão do Proton.

## Requisitos

- Windows 10 ou 11 (64 bits) para o Neural Rendering. O AMDNR Launcher é um programa para Windows que pode rodar
  sob Proton (experimental, ainda não testado por nós). Sob Linux / Proton o AMDNR roda como uma build do
  OptiScaler sem NR (veja "Linux / Proton").
- Uma GPU AMD com driver atual. O runtime neural usa HIP pelo driver; não é preciso HIP SDK nem
  modo desenvolvedor. Quais chips:
  - RX 9000 (RDNA 4): os dois runtimes.
  - RX 7000 (RDNA 3, desktop e mobile): os dois runtimes - o lmxxf pelo backend RDNA 3 do AMDNR, mais lento que
    no RDNA 4 (o nível de tamanho da rede vem ligado por padrão, veja acima).
  - Strix Halo (Radeon 8060S / 8050S): lmxxf.
  - APUs de portáteis com 12+ unidades de computação (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M / 880M): lmxxf,
    experimental e lento. Z1 (4 CU), 760M / 740M e 860M / 840M: não suportados.
  - RX 6000 (RDNA 2): ainda não suportada, planejada para o 0.4.0. Steam Deck e APUs RDNA 2: não suportados (para
    o Neural Rendering; para os upscalers sob Proton, veja "Linux / Proton").

  A aba Neural diz o que sua GPU consegue rodar (passe o mouse nas entradas de runtime, ou veja a linha GPU em
  Diagnostics).
- Um jogo Direct3D 12, Direct3D 11 ou Vulkan. O caminho neural AMD em si é D3D12; jogos D3D11 e
  Vulkan o alcançam pela ponte D3D12 do OptiScaler, o que significa que o upscaler precisa ser um dos
  backends "w/Dx12" (`ffx_12`). Deixe `Dx11Upscaler` / `VulkanUpscaler` em `auto` e esta build o
  escolhe por você quando o neural rendering está ligado. Com o Neural Rendering ligado, a lista de Upscaling os chama de "... w/Dx12 - Neural".
- Cerca de 2 GB de VRAM livre em resoluções de renderização da classe 1080p.

## O que há nos arquivos

**AMDNR-vX.X.X.zip**

| Arquivo | O que é |
|---|---|
| `OptiScaler.dll` | OptiScaler com o backend AMD do DLSS-NR (AMDNR 0.3.4.2). Renomeie como o guia indica. |
| `OptiScaler.ini` | Configurações. O Neural Rendering vem ativado; o log vem ligado para que um relato tenha o que anexar. |
| `LmxxfNrRuntime.dll` | O runtime neural lmxxf (0.3.4, sem mudança no 0.3.4.1 e no 0.3.4.2: os kernels do lmxxf, incluindo os do lmxxf 0.31, os kernels c32w do AMDNR, os tamanhos de rede pequenos e a máscara nativa de personagens). Usado só quando escolhido; lê o `LmxxfNrRuntime.pak` ao lado, veja "O runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Os pesos, módulos HIP e shaders do runtime lmxxf em um arquivo criptografado (440 MB, sem mudança no 0.3.4.1 e no 0.3.4.2; novo no 0.3.4: os módulos para portáteis e os kernels do lmxxf 0.31). Só o runtime lmxxf o lê; é inofensivo mantê-lo junto com o runtime do danielblnc. |
| `OptiScaler\` | FSR, XeSS, o denoiser FidelityFX e o D3D12 Agility SDK que o OptiScaler usa. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | O dlssg-to-fsr3 do Nukem9, sem modificações e renomeado: as chamadas de DLSS Frame Generation do jogo servidas pela geração de quadros do FSR 3, também em Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, veja `Licenses/`. |
| `Licenses\`, `LICENSE` | Licenças de terceiros, o aviso do AMDNR (`AMDNR_NOTICE.txt`) e a licença GPL-3.0 desta build. |
| `SHA256SUMS.txt` | Checksums de cada arquivo deste zip e dos arquivos dos zips de runtime do danielblnc que ele lista. |

**Os zips de runtime do danielblnc** (DLSS-NR on AMD by Daniel Blanco, sem modificações, com a permissão dele; use um)

Qual usar: na **RX 9000 e na RX 7000**, o `v0.4.3-Runtime.zip` (recomendado) da Alpha0.3.4.1; o
`v0.4.1-Runtime.zip` e o `v0.4.0-Runtime.zip` da mesma release continuam aceitos. O runtime lmxxf não
precisa de zip de runtime na RX 7000 e na RX 9000; APUs de portáteis usam só o lmxxf. O AMDNR Launcher oferece
0.4.3 (recomendado), 0.4.1 e 0.4.0, e escolhe para você.

| Zip | Release | Runtime do danielblnc |
|---|---|---|
| `v0.4.3-Runtime.zip` | Alpha0.3.4.1 | 0.4.3, **recomendado na RX 9000 e na RX 7000**; as configurações do runtime do danielblnc funcionam com ele |
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1 (e Alpha0.3.4) | 0.4.1, ainda aceito. Network style, Tone curve, Black lift e Game exposure ficam em cinza com ele |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1 (e Alpha0.3.4) | 0.4.0, ainda aceito; as configurações do runtime do danielblnc funcionam com ele |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3, aposentado: não é mais recomendado. Continua funcionando se você já o tem; as configurações do runtime do danielblnc funcionam com ele |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1; as configurações do runtime do danielblnc ficam em cinza com ele |

**O danielblnc 0.4.3 (publicado em 2026-09-28) é suportado desde o primeiro dia:** o layout dele já estava aceito no
host, então o AMDNR 0.3.4, o 0.3.4.1 e o 0.3.4.2 todos o acionam, sem atualização do AMDNR. Os números dele para essa
versão: +20% Reference / +18% Fast em relação ao 0.4.2, medição dele.

Cada um contém:

| Arquivo | O que é |
|---|---|
| `dlssnr_amd_pass1..3.dll` | O runtime neural AMD, sem modificações. Três cópias para que o multi-pass tenha uma por passe. |
| `dlssnr_on_amd_weights.bin` | Os pesos da rede que o runtime carrega. |
| `danielblnc_ATTRIBUTION.txt` | O crédito de Daniel Blanco e os termos sob os quais o AMDNR distribui o runtime dele. |

## O menu (novo no 0.3.4)

Pressione `INSERT`. Todas as abas têm o mesmo visual: abas de texto, uma linha de cabeçalho com Discord e
GitHub (abre esta página), uma linha de créditos (o nome de Daniel Blanco abre a página dele no GitHub), a linha **Components** (quantos dos
sete componentes do OptiScaler estão ativos; clique para ver a lista) e um rodapé com
Menu Scale, Save Settings e Close. A ajuda abre quando você passa o mouse sobre o rótulo
de um controle.

**A aba Neural, de cima para baixo:**

- **Enable Neural Rendering** e a tecla dele (o botão, por exemplo `Home`: clique nele e depois pressione outra
  tecla para trocar).
- **Neural runtime** (danielblnc / lmxxf, com a versão exata dos seus arquivos, por exemplo `lmxxf 0.3.4`) com uma palavra de estado: running, restart the game to switch, not
  installed, not for this GPU ou stopped. Abaixo, o crédito do runtime ativo e uma linha de status, por exemplo
  `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms` (o último número é o custo do NR), e uma linha **Live** fechada com mais
  detalhes. Quando algo
  precisa da sua atenção, aparece uma linha laranja, com um botão quando há correção (Retry lmxxf, Switch to
  danielblnc, Open Upscaling). No estado padrão não há nenhuma.
- **Preset**: Quality / Balanced / Performance colocam NR resolution em 100 / 85 / 70% e desligam o Dynamic NR;
  nada mais. Em APUs de portáteis há um quarto botão, **Handheld** (veja "APUs de portáteis"). **NR style**, e
  **Style slots** (Store / Apply / Clear).
- **Performance**: NR resolution (%) com o custo, Neural passes, Full network, Fast mode, Dynamic NR resolution, Model
  interleave (Interleave preset e a linha de ritmo aparecem abaixo dele enquanto está ligado).
- **Quality**: Residual strength, Residual limit, Temporal stability, Sharpening (CAS) e **More quality options**
  (Network history - uma só caixa para os dois runtimes -, Output smoothing, Stability mode, Residual temporal,
  Residual edge fade, Still-surface steadiness).
- **Image look**: Colour composition, Detail e Colour strength, e três seções recolhíveis: **Model strength** (Tone
  e Structure intensity, Character structure, Edit detail / colour, Edge guard, Native character mask, e Network
  style, Tone curve e Black lift do danielblnc), **Exposure and highlights** (Auto-exposure, o limite de altas
  luzes dela, Highlight colour guard, Game exposure) e **Appearance filter** (com a palavra off / on depois do nome). Um "default" ou "custom" discreto depois do nome de uma
  seção recolhível diz se você mudou algo dentro dela.
- **Ray Regeneration**: uma seção própria. Desde o 0.3.4.2 ela está sempre ali: enquanto o FSR Ray Regeneration não
  está rodando, uma linha esmaecida diz por que, e os controles ficam
  na tela, esmaecidos, até ele rodar.
- **A linha de ferramentas**, fechada no início: **Diagnostics** (Network output, Debug view, a visualização de
  depuração do RR, Edit shaper A/B, NR cost, as leituras de ghosting e de autoajuste, a linha GPU, **Save report**), **Runtime
  options** (Encoding, Every-frame NR, NR slots, Highlight proxy) e **Experimental** (AMDNR Screen-space GI, em preview).

Um controle que o runtime ativo não tem aparece em cinza com uma etiqueta curta (por exemplo "not in lmxxf yet") ou
oculto com uma contagem ("3 danielblnc-only options hidden"); trocar de runtime não move nenhuma outra linha.

**As outras abas:** Upscaling começa com o upscaler, uma linha de status e Render resolution (os antigos Upscale
Ratio Override e Output Scaling); numa placa que não é NVIDIA, "DLSS w/Dx12" não aparece mais. Image tem Sharpness,
Textures, Init Flags e o Magnifier. Frame Gen começa com FG Input e FG Output. Interface tem o overlay de FPS e
Keybinds (um botão por tecla). Advanced começa com Active Quirks, depois Display (V-Sync), Compatibility e Logging. As
configurações, as chaves e o que o Save Settings grava não mudam, exceto onde o `CHANGELOG.md` diz.

## Configurações que vale conhecer

Abra a aba **Neural**. Os padrões são o arranjo testado mais recente, então o primeiro passo útil é
mudar uma coisa por vez.

- **NR resolution** — a principal alavanca de qualidade/custo. Abaixo de 100% o modelo trabalha numa
  imagem menor e só a sua *correção* é levada de volta ao quadro em resolução completa, então o quadro
  mantém o próprio detalhe. Acima de 100% o custo cresce com o quadrado (150% é 2.25x). O controle
  anda em passos de 5%: cada novo tamanho de NR pode reter VRAM até o jogo reiniciar, então reinicie
  o jogo depois de muitas mudanças.
  O custo ao lado mostra 1.00x em 100%; no lmxxf é o preço do nível de tamanho da rede em que ele roda (a dica
  dele mostra o nível). Os botões Preset o colocam em 100 / 85 / 70%.
- **Residual strength** — quanto da edição do modelo é aplicado; acima de 1 amplifica. É o controle
  que mais muda a imagem.
- **Residual limit** — um teto para o quanto um pixel pode se mover. Manchas: **abaixe**.
- **Model interleave** — roda o modelo a cada dois quadros para um grande ganho de taxa de quadros.
  Os quadros pulados são preenchidos pelo **Interleave preset**; o padrão é *Edit accumulation*
  (preset 10, nos dois runtimes): cada quadro é a própria imagem daquele quadro mais a correção
  carregada pelo modelo, então nenhuma imagem anterior é mantida. *Guided fill v2* (preset 6,
  danielblnc) e *Classic carry* (lmxxf) são os preenchimentos mais antigos. O ritmo dos dois tipos de
  quadro é automático no danielblnc e desligado no lmxxf (`[DlssNr] AmdInterleavePacing` entre 0 e 1 ritma os
  dois, ao custo de alguns quadros); uma linha apagada abaixo do preset mostra a medição. O Adaptive interleave
  está desligado nesta build.
- **Neural passes** — 2 e 3 empilham o modelo, com retornos decrescentes. Sob o lmxxf o histórico
  da rede continua sendo o primeiro passe; os passes extras são apenas refinamento espacial.
  O danielblnc roda 1 passe em jogos Vulkan (um aviso abaixo do controle diz isso).
- **Colour composition** (Neural > Image look, nos dois runtimes) — *Classic* (padrão) é a imagem
  que você já tinha. *RenoDX (experimental)* roda a composição de cor do RenoDX depois do modelo,
  como o caminho NVIDIA faz: Composition detail e colour, um **Highlight guard** nos dois sentidos
  (2x por padrão) que limita a resposta do modelo em relação ao original, e controles opcionais de
  pele / ambiente. Num quadro display-referred (SDR), com Network output ou com Encoding sRGB / Gamma 2.2 ele
  volta ao Classic nos dois runtimes; o aviso do menu então oferece um botão que desliga o bloqueio. Os estilos e
  presets de NR não mexem nisso.
- **Native character mask** (Image look > Model strength, `[DlssNr] AutoMask`, ligado por padrão) — o tratamento
  próprio do modelo para rostos e pele. Desmarcar agora age nos dois runtimes (no lmxxf reconstrói a rede: uma
  travadinha de cerca de 1 s); no lmxxf, Structure intensity e Character structure agora também
  agem.
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
- **FSR Ray Regeneration** — RX 9000 e RX 7000 (RDNA 4 e RDNA 3) por padrão; só em jogos que usam DLSS Ray Reconstruction (Cyberpunk 2077, Alan
  Wake 2), com o jogo rodando DLSS (spoofing ligado), ray tracing e Ray Reconstruction ativados nas
  próprias configurações. O Neural Rendering então roda depois dele, sobre a sua saída, o que custa
  mais: abaixe a NR resolution se a taxa de quadros cair. Os controles dele têm seção própria, **Neural > Ray
  Regeneration**, mostrada sempre desde o 0.3.4.2: enquanto o Ray Regeneration não está rodando, uma linha esmaecida diz
  por que (o jogo não ligou o Ray Reconstruction, o Ray Regeneration desistiu deste título e por que, ou há quantos
  segundos ele rodou pela última vez), e, quando a sua placa tem algo a acrescentar, uma terceira linha
  esmaecida diz isso; os controles ficam na tela, esmaecidos, até ele rodar. O **perfil path-traced** (menos
  granulado nos rostos com path tracing) é opcional desde o 0.3.3.1: marque-o ali para testá-lo em Resident Evil
  Requiem ou PRAGMATA. A mesma seção tem a intensidade da bias mask e a **suavização de pele** (experimental,
  para jogos que publicam um guia SSS; desligada por padrão, mas ligada por padrão em Resident Evil Requiem desde o
  0.3.3.2); os controles de ajuste temporal ficam em *More Ray Regeneration options*, e a visualização de
  depuração do RR em Diagnostics. Na RX 7000 (RDNA 3) o FSR Ray Regeneration volta a ser oferecido por padrão (a 0.3.3.2 o oferecia só
  na RDNA 4). A AMD o entrega só para RDNA 4: se o driver o recusar, o jogo recebe FSR sem o denoise.
  `[FSR-RR] FfxDenoiserAllowPreRdna4=false` o limita à RDNA 4; RX 6000 e anteriores só o recebem com `true` (aba
  Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **Nitidez depois do RR** (0.3.4.1): quando o
  jogo não envia nitidez, o AMDNR aplica nitidez de 0.25 depois do RR no Windows (0 no Linux / Proton); para
  desligar: Image > Sharpness, marque Override, controle deslizante em 0. Desde o 0.3.4.2 esse número tem a sua
  própria chave no ini, `[Sharpness] RrDefaultSharpness` (mesmo padrão 0.25): coloque 0.15, 0.10 ou 0 ali sem mexer
  no Override, e um valor que o seu ini guardou em `[Sharpness] Sharpness` com o Override desligado aparece no menu
  como à espera.
- **AMDNR Screen GI** (preview, novo no 0.3.4, desligado por padrão; Neural > Experimental, ou `[AmdGi] Enabled=true`) — a luz rebatida e a oclusão ambiente em espaço de tela do próprio AMDNR, a partir da profundidade do jogo, antes do NR e do upscaler; funciona com o NR ligado ou desligado; cerca de 1 ms em High com render 1080p numa RX 9070 XT (medido fora de um jogo). É espaço de tela: falta a luz que vem de fora da tela. Veja `CHANGELOG.md`.
- **Save report** (Neural > Diagnostics, ou Advanced > Logging) — um zip com todos os logs e os arquivos ini para um relato; veja "Se não
  funcionar" acima.

## Se algo der errado

O `OptiScaler.log` aparece na pasta do jogo. Anexe-o em `#bug-report` e diga qual jogo e qual GPU; o **Save
report** (Neural > Diagnostics, ou Advanced > Logging) o compacta junto com todo o resto. O backend AMD também escreve `amd_presr.log` e
`amd_bridge.log`, que são os úteis quando o passo neural em particular se comporta mal. Os logs das três últimas
sessões ficam guardados como `OptiScaler.previous.<exe>.log` (o mais novo), `OptiScaler.previous-1.<exe>.log` e
`OptiScaler.previous-2.<exe>.log` (`[Log] KeepPreviousLogs`; 1 guarda só um, como antes). Depois de um crash,
anexe-os também: o log novo então diz "no clean exit recorded" (desde o 0.3.4 não mais depois de uma saída
normal).

**NR frames 0/s, e a aba Neural ou o `amd_presr.log` diz que a DLL do passe é uma build que este AMDNR não
aciona?** Seus `dlssnr_amd_pass1..3.dll` são uma build do danielblnc que este AMDNR não conhece (um conjunto
0.2.16 foi visto por aí), ou falta uma das três. Desde o 0.3.3.2 a aba Neural mostra o nome do arquivo e a
versão e diz o que fazer. Use o runtime recomendado, com as três DLLs de passe do mesmo zip: na **RX 9000 e na
RX 7000**, o `v0.4.3-Runtime.zip` da Alpha0.3.4.1 (116,484,918 bytes, SHA256 começando com `07dd7774`); o
`v0.4.1-Runtime.zip` e o `v0.4.0-Runtime.zip` da mesma release continuam aceitos (o `dlssnr_amd_pass1.dll` do
`v0.4.1-Runtime.zip` tem 9,916,928 bytes, SHA256 começando com `823063eb`; no `v0.4.0-Runtime.zip`: 10,027,008
bytes, `d62be3d8`). Builds suportadas:
0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0, os zips de runtime citados acima, e as builds mais novas já aceitas
aqui, antes do lançamento delas. Não instale o instalador próprio do danielblnc nem o `dxgi.dll` / `version.dll` /
`winhttp.dll` dele junto do AMDNR: o AMDNR já roda o runtime dele. **O 0.3.4.2 aceita um layout a mais:** uma build mais nova do
danielblnc pode rodar aqui sem esperar uma atualização do AMDNR — a aba Neural nomeia o arquivo e a versão dele como sempre, e as configurações
exclusivas do danielblnc (Network style, Tone curve, Black lift, Game exposure, Fast mode) funcionam com ele. Nada muda
para os arquivos de runtime que você já tem.

**O lmxxf não faz nada, ou para na hora, num PC com gráficos integrados?** Corrigido no 0.3.3.2. Num Ryzen
de desktop com os gráficos integrados ligados, num notebook com APU AMD e uma Radeon, ou num PC com duas GPUs
AMD, a GPU do jogo muitas vezes não é o dispositivo HIP 0. O lmxxf então falhava no primeiro quadro
(`hipErrorInvalidHandle (400)`, depois "session is poisoned" no `lmxxf_backend.log`) e ficava desligado.
Substitua tanto o `OptiScaler.dll` (o arquivo que você renomeou, por exemplo `dxgi.dll`) quanto o
`LmxxfNrRuntime.dll` pelos arquivos do 0.3.3.2 ou mais novos. Ainda não testado num PC assim: se o lmxxf continuar
parando, a aba Neural agora diz o motivo; envie o `lmxxf_backend.log` e o `amd_bridge.log` (ele lista os dispositivos HIP).

**Um PC com gráficos integrados e uma Radeon (um Ryzen de desktop com a GPU integrada ligada, ou um notebook): o NR
nunca inicia, e a aba Neural ou a linha da GPU cita a GPU integrada?** O passe neural do AMDNR roda na GPU com que o
jogo desenha. Se o Windows iniciou o jogo na GPU integrada, o NR não roda na sua Radeon de jeito nenhum. Defina o jogo
para a GPU dedicada: Configurações do Windows > Sistema > Tela > Elementos gráficos, adicione o `.exe` do jogo, Opções,
Alto desempenho; depois reinicie o jogo e confira a linha da GPU em Neural > Diagnostics, que cita o adaptador em que o
NR roda (o `OptiScaler.log` tem uma linha `AMD neural: NR runs on ...` quando esse adaptador não é a GPU principal).
Visto no Starfield num Ryzen de desktop.

**A linha de status do lmxxf mostra `c32w=off:nofile` numa RX 9070 / 9070 XT?** Uma pasta antiga
`DLSS5-AMD\native-game-tiled-assets` ao lado do `.exe` do jogo (sobra de uma instalação anterior do lmxxf) é
usada no lugar do `LmxxfNrRuntime.pak`. Ela não tem os kernels c32w, então o lmxxf roda na velocidade antiga.
Apague ou renomeie a pasta `DLSS5-AMD`: o pak tem tudo de que o lmxxf precisa. Um `LmxxfNrRuntime.pak`
anterior ao 0.3.3.2 mostra o mesmo status; troque-o pelo desta versão.
`fk=fff-` na mesma linha significa o mesmo (um pak antigo ou uma pasta solta): o lmxxf continua rodando,
na velocidade antiga.

**danielblnc: o estilo do NR ainda muda quando a NR resolution sai de 100%?** Continua aberto no 0.3.4, no
0.3.4.1 e no 0.3.4.2, e o padrão não mudou. Em 100%, Residual strength 0.99 dá 99% de 1.00 (corrigido no 0.3.3.2);
fora de 100% (também nos passos do Dynamic NR e nos presets Balanced / Performance) strength, limit e edge fade
continuam agindo sobre o resultado inteiro, então o visual pode mudar. O 0.3.4 traz um A/B para achar a correção
certa: Neural > Diagnostics >
**Edit shaper (A/B, not saved)** com Literal, F1 e F2, mais Only below 100% e Carry cap (só danielblnc; o Save
Settings não o grava; as chaves do ini são `[DlssNr] AmdEditShaper`, `AmdEditShaperLimit`, `AmdEditShaperScope` e
`AmdEditShaperCarryCap`). Se algum deles deixar 85% com a cara de 100% no seu jogo, conte para a gente no Discord
com capturas. O lmxxf não é afetado.

**O menu abria e fechava duas vezes a cada toque na tecla, ou teclado e mouse paravam de funcionar em toda a área
de trabalho com o menu aberto (Assetto Corsa)?** Corrigido no 0.3.4: um segundo toque na tecla do menu ou do NR em
menos de 400 ms é ignorado (`[Hotfix] MenuToggleDebounceMs`, 0 = o comportamento antigo), e com o menu aberto o
hook de teclado ou mouse de baixo nível do jogo é pulado, mas a tecla continua chegando ao Windows
(`[Hotfix] MenuLowLevelHookPassThrough=false` = o comportamento antigo). Ainda não confirmado no Assetto Corsa: se
continuar acontecendo, envie o zip do relatório.

**O menu abria sozinho no seletor de runtime, os cliques não faziam nada, ou a tecla do menu só escondia o menu
enquanto ficava pressionada (Assetto Corsa)?** Corrigido no 0.3.4.2: a tecla do menu abre ou fecha o menu uma só vez
por toque físico (uma mensagem de tecla que chega atrasada é ignorada), cliques e teclas do menu mais curtos que um
quadro são reproduzidos, o seletor de runtime responde a `1` / `2` / `Enter` / `Esc` e ao X da sua barra de título, e
fechar o menu conta como "Decide later"; o seletor não abre mais o menu sozinho. Ainda não confirmado pelo
jogador do Assetto Corsa: se continuar acontecendo, envie o zip do relatório. Para voltar à tecla do menu e aos cliques
do 0.3.4.1: adicione `DiagInputHooksSkip=presslatch,clickreplay` em `[Hotfix]` no seu `OptiScaler.ini` (sem chave
nova; o ini incluído só descreve a linha em um comentário).

**Uncharted: Legacy of Thieves Collection fecha alguns segundos depois de iniciar numa RX 9000 com o Neural Rendering
ligado?** Conhecido, não corrigido no 0.3.4.2: o jogo executa seu trabalho em fibras pequenas de 192 KiB, e a primeira
inicialização do HIP (o driver compila seus kernels auxiliares dentro do jogo) estoura essa pilha no primeiro quadro
de NR. Mantenha `[DlssNr] Enabled=false` no `OptiScaler.ini` desse jogo até o 0.3.5. Trocar de runtime não ajuda: os
dois runtimes iniciam o HIP na mesma thread.

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

**Você não encontra as configurações do Ray Regeneration?** Desde o 0.3.4.2 a seção está sempre no menu: aperte
**Insert**, abra a aba **Neural** e desça até **Ray Regeneration** (no 0.3.4.1 e anteriores a seção era desenhada só
enquanto o Ray Regeneration estava rodando, ou quando ele tinha desistido do jogo, e foi isso que aconteceu com um
jogador de RX 7000). Enquanto ele não está rodando, uma linha esmaecida sob o título diz por que: o jogo não ligou o
Ray Reconstruction, o Ray Regeneration desistiu deste título e por que, ou há quantos segundos ele rodou pela última
vez. Numa RX 7000 uma terceira linha esmaecida acrescenta que ali ele é oferecido por padrão, mas o driver pode
recusá-lo (então o jogo fica com o denoiser dele); em RX 6000 e anteriores ela diz que ele não é oferecido nessa GPU,
que a AMD publica o denoiser para RDNA 4, e que `[FSR-RR] FfxDenoiserAllowPreRdna4=true` o oferece de todo jeito. Os controles ficam na tela, esmaecidos, e voltam a agir assim que ele roda. Para fazê-lo rodar, nas
configurações gráficas do próprio jogo: escolha **DLSS** como upscaler (não FSR, não XeSS), ligue o **ray tracing** ou o
path tracing e ligue o **Ray Reconstruction** (DLSS-RR); a aba Upscaling passa a mostrar "FSR Ray Regeneration". Qual
configuração mudar para qual problema está no guia de configurações do Ray Regeneration
**RR-BEST-SETTINGS.md** (não está no zip).

**O Ray Regeneration parece granulado ou com ruído?** Julgue-o primeiro com o **Neural Rendering desligado** (desmarque **Enable Neural Rendering** no topo da aba Neural, aperte Home enquanto joga, ou use
`[DlssNr] Enabled=false`): o passe neural roda depois do Ray Regeneration, sobre a saída dele,
então uma captura feita com o NR ligado não diz nada sobre o denoiser. Depois, conforme o tipo de granulado — granulado
que rasteja numa cena parada, pontinhos brilhantes, granulado nos rostos, rastros atrás de personagens em movimento —
as configurações para testar estão nesse mesmo guia, **RR-BEST-SETTINGS.md**. **No 0.3.4.2 nenhum valor
padrão do denoiser e nenhum da nitidez mudou**: os números são os do 0.3.4.1. O que mudou: a nitidez que o AMDNR
adiciona depois do Ray Regeneration agora tem a sua própria chave no ini, `[Sharpness] RrDefaultSharpness` (mesmo
padrão 0.25), então 0.15, 0.10 ou 0 é uma edição do ini, e não um build novo. Confira o seu ini antes de perseguir o
granulado com o controle de nitidez: um valor em `[Sharpness] Sharpness` não faz nada enquanto o `OverrideSharpness`
estiver desligado, e ele passa a valer no instante em que você marca **Override** no menu - e é por isso que Image > Sharpness agora o
marca ("ini Sharpness 1.00 waits for Override"). Duas coisas que não vamos
disfarçar: parte do granulado é a amostragem de raios do próprio jogo — o denoiser da AMD não foi feito para consertar
ruído que chega correlacionado, e um jogo que oferece DLSS Ray Reconstruction desliga o denoiser dele e nos entrega o
sinal cru — e o granulado que rasteja numa cena parada tem, do nosso lado, uma causa estrutural que nenhum controle
remove por completo. Esse é um problema conhecido; o trabalho para ele é o 0.3.5.

**O Ray Reconstruction do jogo está ligado, mas a aba Neural diz "Ray Regeneration is off in this title"?**
O jogo não publica o que o FSR Ray Regeneration precisa: o plugin DLSS dele passa matrizes de câmera vazias
(Satisfactory), que o Ray Reconstruction da NVIDIA trata como opcionais e o FSR Ray Regeneration exige. O upscaling
FSR roda no lugar dele e o NR assume sua posição normal antes do SR; a aba Upscaling diz o mesmo. Desde o 0.3.4 ele
fica desligado a sessão inteira num jogo Unreal com essa assinatura. Desligue o Ray Reconstruction no jogo e
restaure as configurações de denoiser da própria engine.

**Um jogo Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) mostra "DX12 Error 0x80070057"?**
Esses jogos trazem a própria geração de quadros XeSS. Esta build a deixa com eles (a saída XeFG do
OptiScaler fica desativada ali e a aba Frame Gen explica); use a opção XeSS FG do próprio jogo. Se
ainda acontecer, defina `[FrameGen] Enabled=false` e `[fakenvapi] ForceXeLL=false` e relate com o log.

**The Last of Us Part I trava ao iniciar?** É a inicialização do Streamline do próprio jogo, um
problema conhecido do OptiScaler: renomeie o `sl.common.dll` na pasta do jogo para
`sl.common.dll.bak` e escolha **FSR 3.1** nas configurações do jogo em vez de DLSS.

Notas completas de cada versão: `CHANGELOG.md` (no zip e no repositório).

## Roteiro

- **0.3.4.2** (esta build) — hotfix: o menu do Assetto Corsa (a tecla do menu abre ou fecha o menu uma só vez por
  toque, cliques mais curtos que um quadro são reproduzidos, o seletor de runtime responde às teclas e fechar o menu
  conta como "Decide later"), o seletor de runtime não abre mais o menu sozinho, a seção Ray Regeneration está sempre
  na aba Neural e diz por que não está rodando, um layout de runtime do danielblnc a mais aceito,
  correções do texto sobre Wine /
  Proton, acréscimos no README (nomes de proxy, Uncharted, PCs híbridos, `AmdLmxxfTierSnap` na RX 9000, as duas
  entradas de FAQ sobre o Ray Regeneration); NR idêntico
  byte a byte ao 0.3.4.1, com a exceção dessa única linha de layout aceito.
- **0.3.4.1** — hotfix: nitidez do Ray Regeneration quando o jogo não envia nenhuma (Windows;
  nenhuma por padrão no Linux / Proton), o falso pop-up "Upscaler failed to run!" do Control Resonant, o menu no
  Linux / Proton vinculado à janela do jogo, o Save report indica vkd3d-proton / DXVK; AMDNR Launcher 0.3.4.1
  (nove idiomas, busca, favoritos, ocultar, renomear, CHOOSE GAME .EXE, PLAY, um UNINSTALL completo); NR sem mudança.
- **0.3.4** — o menu novo (a aba Neural refeita, o mesmo visual em todas as abas, Save report); lmxxf
  mais rápido na RX 7000 (o nível de tamanho da rede por padrão) e na RX 9070 / 9070 XT (kernels do
  lmxxf 0.31); lmxxf em APUs de portáteis (experimental; novos tamanhos de rede 360p e 576p); o lmxxf
  ganha Network output, Encoding, Residual edge fade, a máscara nativa de personagens e um Fast mode opcional; AMDNR Screen GI (preview);
  configurações do runtime do danielblnc (Network style, Tone curve, Black lift, Game exposure) e uma proteção da
  cor das altas luzes; ajuste e diagnóstico do Ray Regeneration; correções da entrada do menu no Assetto Corsa, do Shadow of the Tomb Raider, do Marvel's Midnight Suns e do The
  Last of Us Part II, da saída limpa e dos logs.
- **0.3.3.x** — lmxxf no RDNA 3 (RX 7000; backend próprio do AMDNR); composição de cor
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
- **0.4.0** — a RX 6000
  (RDNA 2) e suporte a jogos sem upscaler próprio (classe Stray), em que o OptiScaler fornece o upscaler e o
  passo neural juntos.

---

## Créditos

Esta build é um trabalho de ligação sobre o trabalho de outras pessoas. Se ela lhe for útil, os
agradecimentos pertencem ao upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD by Daniel Blanco — https://github.com/danielblnc/DLSS-NR-on-AMD (os arquivos `*Runtime.zip`, sem modificações)
- **lmxxf** (Kien) — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (o port da rede, os kernels e o runtime HIP, MIT)
- **TheAutomatic** — `LmxxfNrRuntime.cpp`, `LmxxfNrApi.h`, `LmxxfProductionOptions.h`: portions contributed to lmxxf by TheAutomatic (MIT)
- **kernels do lmxxf 0.31** no `LmxxfNrRuntime.pak` (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)) — do lmxxf (Kien, MIT), compilados pelo AMDNR a partir das fontes e da receita de compilação do lmxxf; a parte do AMDNR é o carregamento, os pinos SHA-256, a seleção por GPU e os fallbacks
- **kernels c32w** (0.3.3.2) — kernels RDNA 4 de uma wave do próprio AMDNR para a rede do lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); ideias da documentação pública de WMMA do RDNA 4 da AMD (GPUOpen, ROCm matrix instruction calculator)
- **O backend RDNA 3 do AMDNR** (0.3.3; as builds para portáteis gfx1103 / gfx1150 no 0.3.4), a política de níveis de tamanho da rede e os tamanhos de rede pequenos (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, sem modificações)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (a matemática da composição de cor, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), a base do desbloqueio multiquadro do XeFG integrado e do seu ritmo
- **burak113** — o pré-processador do FSR Ray Regeneration (branch ffx-denoise-experimental do OptiScaler, GPL-3.0)
- **Screen-space GI** (o efeito herdado; retirado do menu no 0.3.4, `[AmdRtgi] Enabled` no ini) — um efeito que o AMDNR herdou da linhagem OptiScaler-AMD-PreSR; o crédito é dos autores originais. Ele precisa da pasta `experimental_lighting` do pacote do danielblnc, que o AMDNR não distribui.
- **AMDNR Screen GI** (preview do 0.3.4) — trabalho próprio do AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), escrito a partir de artigos publicados (Therrien, Levesque e Gilet 2023; Jimenez et al. 2016; Schied et al. 2017; e os outros listados em `CHANGELOG.md` e `Licenses/AMDNR_NOTICE.txt`)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (novo no 0.3.4) é um programa para Windows 10 / 11 que também pode rodar sob Proton no Linux
(experimental, ainda não testado por nós: veja "Linux / Proton"). Ele instala e atualiza o AMDNR jogo a jogo:
encontra os seus jogos (Steam, Epic, o app Xbox, Ubisoft Connect, o EA app, GOG, Rockstar, Battle.net e Amazon
Games), escolhe o nome da DLL, baixa a build e o runtime do danielblnc que você escolher, verifica cada instalação
com o Doctor dele e se atualiza sozinho. Baixe o `AMDNR-Launcher.exe`, o AMDNR Launcher da release mais
recente, na página de releases: <https://github.com/3zwr1/AMD-NR---OptiScaler/releases>

**Novo no Launcher 0.3.4.1: vocês pediram, a gente fez** (a partir do primeiro feedback no Discord):

- Nove idiomas: inglês, árabe, chinês (simplificado), francês, espanhol, português, italiano, russo e polonês. O
  launcher segue o idioma do seu Windows (ou inglês, se ele não for um desses); escolha outro em LANGUAGE (IDIOMA)
  ou em SETTINGS (CONFIGURAÇÕES), onde a escolha também aparece na primeira inicialização. Os resultados do
  Doctor, as mensagens de instalação e o relatório do COLLECT LOGS (COLETAR LOGS) ficam em inglês para que o
  suporte possa lê-los.
- Busca na biblioteca; favoritos (uma estrela, e os jogos com estrela primeiro); ocultar jogos (HIDDEN os mostra de
  novo); renomear um jogo.
- CHOOSE GAME .EXE: escolha você mesmo o exe do jogo quando o launcher escolheu o errado ou nenhum. Cyberpunk 2077 e
  The Witcher 3 (REDengine) agora são encontrados na pasta certa sem isso.
- PLAY e OPEN FOLDER na página de cada jogo; STORES liga e desliga lojas inteiras; as pastas que você adiciona à mão
  continuam na lista, também enquanto o disco delas está desconectado, até você removê-las.
- UNINSTALL pergunta antes e remove tudo o que o mod colocou, inclusive o que ele gravou enquanto o jogo rodava (logs,
  caches, crash dumps, relatórios não terminados); ele mantém os zips de Save report já prontos e uma DLL com o nome
  de proxy que não é mais um OptiScaler (a do próprio jogo).
- COLLECT LOGS em qualquer jogo, instalado ou não, com um relatório da varredura.
- Portáteis e APUs (ROG Ally Z1 Extreme e outras APUs Ryzen) são reconhecidos, com um aviso de que o AMDNR neles
  ainda está em teste.
- Linux (experimental, ainda não testado por nós): o mesmo exe para Windows pode rodar sob Proton, mostra ali um
  aviso com o que fazer e também procura jogos na sua biblioteca Steam do Linux; os passos estão em
  "Linux / Proton". Conte para a gente no Discord se funciona.

O código-fonte está em `Launcher/OpenSource/` do repositório GitHub deste projeto, com licença própria,
`Launcher/OpenSource/LICENSE.txt`. Ele **não** é coberto pela licença GPL-3.0 (`LICENSE`) deste repositório: é
source-available, todos os direitos reservados, Copyright (c) 2026 3zwr1 (AMDNR). O manifesto do launcher é
`Launcher/manifest.json`. Veja também a seção 7 de `Licenses/AMDNR_NOTICE.txt`.

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
