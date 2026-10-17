# AMDNR — DLSS 5 Neural Rendering em GPUs AMD (build OptiScaler) — v0.3.5.6

[English](README.md) | [中文](README.zh-CN.md) | **Português** | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

> **Precisamos do seu apoio.** Entre no servidor do Discord — <https://discord.gg/AMDNR> — para
> ajuda, relatos de bugs e builds de teste; cada relato com um log torna a próxima build melhor.

DLSS 5 Neural Rendering rodando em GPUs AMD, integrado ao OptiScaler, para funcionar em qualquer
jogo Direct3D 12 que o OptiScaler já intercepta. Por cima do passo neural: model interleave para um
grande ganho de taxa de quadros, composição residual, geração de quadros XeSS desbloqueada até 6X
(até 10X opcional em jogos D3D12) e FSR Ray Regeneration para jogos que usam DLSS Ray Reconstruction. Desde o 0.3.5,
o **AMDNR Anywhere** (preview) leva o Neural Rendering a jogos sem upscaler próprio, pelo AMDNR Launcher, sem gravar
nada na pasta do jogo (veja "AMDNR Anywhere"). Desde o 0.3.5.6 a lista de upscalers também oferece duas opções
experimentais e opcionais feitas por outros autores, o **Model E** (HelixSR) e o **Model L** (d4r), que o AMDNR
Launcher baixa das releases dos próprios autores (veja "Model E e Model L").

**Discord: <https://discord.gg/AMDNR>** — suporte, relatos de bugs (`#bug-report`), builds de
teste.

**Apoie o projeto: <https://ko-fi.com/3zinr>**

> **O runtime do danielblnc é trabalho de Daniel Blanco.** O runtime neural AMD dos arquivos `*Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) é o **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> O AMDNR o distribui sem modificações, com a permissão dele; não é trabalho do AMDNR. Por favor, apoie o projeto dele.
> Os créditos completos de todos os outros estão no fim desta página.

> **Novidades do 0.3.5.6** (2026-10-17, release Alpha0.3.5.6, com o AMDNR Launcher 0.3.5.25):
> - **Duas novas opções de upscaler, experimentais e opcionais:** o **Model E** (HelixSR, de lonewolf0622) e o
>   **Model L** (d4r, de countervolts), baixados pelo launcher das releases dos próprios autores quando você os escolhe.
>   O AMDNR não hospeda nem distribui os arquivos deles nem os da NVIDIA. O Model L é só para RX 9070 / RX 9070 XT e
>   roda com o runtime lmxxf ou com o Neural Rendering desligado; o runtime do danielblnc faz par com o Model E. Leia
>   "Model E e Model L" antes de ligá-los.
> - **O Full Model Look é o NR style padrão:** um visual mais completo nos dois runtimes, rodando depois do upscaling
>   nas placas que aguentam o custo - a verificação de custo e a proteção contra TDR o levam de volta para antes do
>   upscaling onde precisam, e dizem isso. Escolher um visual em NR style também define o Neural placement e o Full
>   network dele, e o menu avisa quando a sua própria configuração de placement está segurando o visual.
> - **Mais detalhe nos rostos**, sempre ligado, nos dois runtimes (`[DlssNr] AmdFaceDetail=false` o desliga).
> - **Menos ghosting com o lmxxf:** os rastros atrás de objetos em movimento ficam mais curtos, antes e depois do
>   upscaling, e o clarão depois de um corte de cena tem cerca de metade da força. O ghosting diminuiu, não sumiu.
> - **RX 7000 e portáteis:** o Neural Rendering agora roda por padrão com a rede um tamanho menor (Fast mode do lmxxf),
>   o que custa menos tempo por quadro. `[DlssNr] AmdLmxxfFastMode=false` o desliga; a RX 9000 continua com ele
>   desligado.
> - **Controles novos e renomeados:** Game exposure (Auto / On / Off), Anti-halo (antes Halo protection) e, no
>   danielblnc, **Refine passes** (os passes 2 e 3 rodam com a rede um tamanho menor). A **NR resolution** agora vai até
>   200% nos dois runtimes.
> - **O menu mostra só o que o seu runtime consegue usar:** um controle que um runtime não consegue rodar fica oculto
>   nesse runtime (a chave dele no ini continua funcionando), e a entrada do runtime nomeia a versão do lmxxf que
>   realmente roda (por exemplo `lmxxf 0.41`).
> - **Correções:** os falsos "timeouts" do danielblnc (testado no Silent Hill 2) - uma espera que desiste não desliga
>   mais o Neural Rendering, o texto que dizia que o driver reiniciou a GPU sumiu (não houve reinício nenhum), e o aviso
>   "runtime failed" só é gravado depois de uma falha real. O FSR 3 / FSR 4 não deve mais cair para o FSR 2.1.2 quando
>   outra opção de upscaler falha ou não está pronta: a sua própria escolha de FSR é mantida. A memória da geração de
>   quadros XeFG é contada corretamente (inclusive com a configuração 6X).
> - **Substitua todos os arquivos do zip juntos** (`OptiScaler.dll` e `LmxxfNrRuntime.dll` precisam vir da mesma
>   release); usuários do launcher: ele atualiza por você. O que ainda está em aberto: "Limites conhecidos (0.3.5.6)".
>   Detalhes: `CHANGELOG.md`.

> **Entre o 0.3.5 e o 0.3.5.6** (do 0.3.5.1 ao 0.3.5.5R4): o danielblnc 0.6.0 é acionado e agora é o runtime do
> danielblnc recomendado (ele também roda na RX 6000, com o HIP SDK da AMD instalado); o lmxxf 0.41 do Kien (MIT) é o
> conjunto padrão na RX 9000; o NVIDIA look (o NR style padrão do 0.3.5.3 ao 0.3.5.5R4); uma proteção contra
> travamentos que reduz ou para o NR depois de uma falha de GPU contada; o AMDNR Launcher redesenhado (treze idiomas,
> ASK AI); e muitas correções de crash a partir dos seus relatos. Detalhes: `CHANGELOG.md` e as notas de release no
> GitHub.

> **Novidades do 0.3.5:** **AMDNR Anywhere** (preview): Neural Rendering para jogos que não têm DLSS, XeSS nem FSR 2
> próprios - um único botão **PLAY ANYWHERE** no AMDNR Launcher, nada gravado na pasta do jogo; RX 9000 (RDNA 4)
> nesta release (veja "AMDNR Anywhere"). **O Ray Regeneration tem a sua própria aba**, logo depois de Upscaling, e
> as linhas de status dela dizem, por placa e por API, o que roda e por que não; na RX 7000, desde o 0.3.5.1, ele volta a ser
> oferecido como preview, ligado por padrão (desde o 0.3.5.3 o denoiser ali é o AMDNR Ray Denoiser (preview); a caixa da aba Upscaling ou `[FSR-RR] FfxDenoiserAllowPreRdna4=false` o desliga e o jogo fica com o
> denoiser dele). **O passe neural pode rodar depois
> do upscaling** (`[DlssNr] AmdPlacement=post`, Neural > Performance > Neural placement; o padrão `pre` não mudou no
> 0.3.5 - desde o 0.3.5.6 o NR style padrão pede `post`, veja "Configurações que vale conhecer"). **A aba
> Frame Gen diz por que nada é gerado** e nomeia os cinco passos (veja a entrada da FAQ sobre geração de quadros).
> **Mais rápido na RX 7000 e nos portáteis da classe Z1 Extreme:** cerca de 10% menos tempo de rede, mesma imagem
> bit a bit (o conjunto de módulos 0.3.5 no `LmxxfNrRuntime.pak`). **O lmxxf 0.37 do Kien (MIT) vem ligado por
> padrão na RX 9000:** cerca de 20% menos tempo de rede numa RX 9070 XT (experimental na RX 9060 / 9060 XT;
> `[DlssNr] AmdLmxxfL37=false` o desliga; desde o 0.3.5.3 o conjunto padrão ali é o lmxxf 0.41). Nos portáteis RDNA 3, o **FSR 4 (INT8)** é uma opção experimental que
> você liga manualmente, e os slots de estilo personalizados agora guardam o visual completo. Jogos com
> resolução dinâmica não reconstroem mais a rede a cada passo, a edição carregada não some mais quando você se move
> num portátil com Model interleave, Uncharted: Legacy of Thieves não fecha mais numa RX 9000, e muitas outras
> correções. **Os três arquivos mudam: substitua `OptiScaler.dll`, `LmxxfNrRuntime.dll` e `LmxxfNrRuntime.pak`
> juntos; usuários do launcher: ele atualiza por você.** Detalhes: `CHANGELOG.md`.

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
o 0.3.5.6 é a tag Alpha0.3.5.6):

* `AMDNR-vX.X.X.zip` (no 0.3.5.6: `AMDNR-v0.3.5.6.zip`), com o runtime lmxxf completo.
* Para o runtime do danielblnc, um zip de runtime: o `v0.6.0-Runtime.zip` (recomendado; RX 9000, RX 7000 e, com o HIP
  SDK da AMD instalado, RX 6000), da Alpha0.3.5.5R4; o `v0.5.1-Runtime.zip` (Alpha0.3.5), o `v0.5.0-Runtime.zip`
  (Alpha0.3.4.2; ele pode congelar alguns jogos em algumas placas), o `v0.4.3-Runtime.zip` (Alpha0.3.4.2 e
  Alpha0.3.4.1), o `v0.4.1-Runtime.zip` e o `v0.4.0-Runtime.zip` (Alpha0.3.4.1) continuam aceitos na RX 9000 e na
  RX 7000. O runtime lmxxf vem no `AMDNR-vX.X.X.zip` e não precisa de zip de runtime na RX 7000 e na RX 9000; APUs de
  portáteis usam só o lmxxf. O AMDNR Launcher escolhe o zip certo para a sua GPU. Veja "O que há nos arquivos".
* Nada para baixar à mão para o Model E ou o Model L: o AMDNR Launcher os baixa das releases dos próprios autores
  quando você os escolhe (veja "Model E e Model L").

### 2. Extraia os dois arquivos

Extraia o conteúdo dos dois arquivos `.zip`.

### 3. Copie tudo para a pasta do jogo

Primeiro, copie todos os arquivos de `AMDNR-vX.X.X` para a pasta raiz do jogo — a mesma pasta onde
fica o `.exe` do jogo.

Depois, faça o mesmo com todos os arquivos do zip de runtime (por exemplo `v0.6.0-Runtime` na RX 9000, na RX 7000 e na
RX 6000).

> **Atualizando de um AMDNR anterior?** Copie tudo de novo, sobrescrevendo. **Substitua estes três juntos:**
> `OptiScaler.dll` (substitua o arquivo que você renomeou, por exemplo `dxgi.dll`, pelo novo renomeado do
> mesmo jeito), `LmxxfNrRuntime.dll` e `LmxxfNrRuntime.pak` (440 MB). Não os misture com cópias antigas: no 0.3.5.6 o
> `OptiScaler.dll` passa novos dados de quadro ao `LmxxfNrRuntime.dll`, e um runtime mais antigo roda sem eles. Você
> pode manter o seu `OptiScaler.ini`: as configurações novas usam os valores padrão. Os arquivos do seu zip de runtime
> do danielblnc ficam como estão (o Doctor do launcher oferece o recomendado para a sua placa). O AMDNR Launcher faz
> isso por você: UPDATE ALL, ou REPAIR / UPDATE num jogo que mostra "Update available" (o launcher se atualiza
> primeiro).

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
`C:\Users\` não é. Anexe o zip em `#bug-report`. O **COLLECT LOGS** do AMDNR Launcher grava o mesmo zip para qualquer
jogo e, desde o 0.3.5.1, acrescenta o log de crash e o dump mais novo depois de um crash.

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

Os dois runtimes compartilham uma única aba Neural (veja "O menu" abaixo). Desde o 0.3.5.6 um controle que o runtime
ativo não consegue rodar fica oculto nesse runtime, com uma contagem (a chave dele no ini continua funcionando). Só do
lmxxf: **Full network**,
**Output smoothing** (Quality > More quality options, requer Network history), **Edit detail**, **Edit colour** e
**Edge guard** (Image look > Model strength: ganho na parte fina da edição do modelo, sua cor em relação à mudança
de brilho e um esmaecimento da edição nas bordas de profundidade) e o limite de altas luzes da auto-exposição.
Novo no lmxxf no 0.3.4: Network output, Encoding, Residual edge fade, Game exposure, Fast mode, a leitura
do ritmo do interleave e a máscara nativa de personagens do modelo com Structure intensity e Character
structure (cada mudança reconstrói a rede: uma travadinha de cerca de 1 s).

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, só no lmxxf) roda os 71 blocos
da rede em vez de pular o 42, o 43 e o 46: um pouco mais fiel, cerca de 0.5 ms mais lento em 1080p
(16.6 -> 17.1 ms numa RX 9070 XT, medido no 0.3.3). Desligado por padrão.

**Fast mode** (Neural > Performance, `[DlssNr] AmdLmxxfFastMode`, lmxxf) roda a rede um nível de tamanho abaixo
(1080 -> 900, 900 -> 720; 720 -> 576 e 576 -> 360 onde os tamanhos pequenos estão ligados): cerca de 29% menos tempo de
rede em 1080p (RX 9070 XT, medido fora de um jogo), com os detalhes finos um pouco mais suaves. **Desde o 0.3.5.6 ele
vem ligado por padrão na RX 7000, na Radeon 8060S / 8050S e nas APUs de portáteis** e desligado por padrão na RX 9000;
`false` (ou a caixa de marcação) o desliga, `true` o liga, e a caixa mostra o que roda. Builds do danielblnc que têm o próprio Fast mode
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
  podem ficar um pouco mais suaves. `[DlssNr] AmdLmxxfTierSnap=false` volta aos tamanhos do 0.3.3.2. Desde o 0.3.5.2
  a RX 9000 também se encaixa nos níveis por padrão.
- **RX 9000:** `[DlssNr] AmdLmxxfTierSnap` (ligado por padrão lá desde o 0.3.5.2; opcional antes) leva o tamanho de NR
  do lmxxf a um tamanho de rede em toda resolução de render: alguns tamanhos descem um nível (1440p FSR Quality,
  1707x960 -> o tamanho 900: rede 14.08 -> 9.96 ms por execução numa RX 9070 XT, medido fora de um jogo, imagem um
  pouco mais suave), outros crescem dentro do seu nível (80% de um render 1080p -> 1600x900: mesmo custo, um pouco mais
  de detalhe). `false` mantém os tamanhos do 0.3.3.2.
- Comece com NR resolution em 70% ou menos (o nível 720 com render em 1080p; o preset Performance é 70%). O custo
  ao lado de NR resolution é calculado pelo nível em que a rede roda; a dica dele mostra o nível.
- **0.3.5: cerca de 10% menos tempo de rede na RX 7000**, mesma imagem bit a bit: o conjunto de módulos 0.3.5 no
  `LmxxfNrRuntime.pak` (medido e conferido por hash por um tester numa RX 7800 XT; a tabela acima é a do 0.3.4).
- **0.3.5 na RX 9000: o lmxxf 0.37 do Kien (MIT) vem ligado por padrão** - cerca de 20% menos tempo de rede numa
  RX 9070 XT no tamanho 1080 (14.0 -> cerca de 11.3 ms, medido fora de um jogo; a imagem não é idêntica bit a bit
  à do 0.3.4.2). Na RX 9060 / 9060 XT ele também vem ligado, junto com os kernels c32w e FastK, como experimento
  (ainda não rodado nessa placa). Para desligar: `[DlssNr] AmdLmxxfL37=false`, `AmdLmxxfC32w=false`,
  `AmdLmxxfFastK=false`.
- **Desde o 0.3.5.3 o conjunto padrão na RX 9000 é o lmxxf 0.41 do Kien (MIT)** (`[DlssNr] AmdLmxxfL41=false` o
  desliga); desde o 0.3.5.6 a entrada Neural runtime nomeia o conjunto que realmente roda. **Desde o 0.3.5.5R4 a
  RX 7000 e os portáteis da classe Z1 Extreme rodam conjuntos de kernels mais novos**, com a mesma imagem bit a bit
  (conferida por hash nas máquinas de testers).
- **0.3.5.6: o Fast mode vem ligado por padrão no RDNA 3** (RX 7000, Radeon 8060S / 8050S e as APUs de portáteis): a
  rede roda um tamanho abaixo daquele que o tamanho de render usaria (um render 1080p roda no tamanho 900), o que custa
  menos tempo por quadro, com os detalhes finos um pouco mais suaves. A RX 9000 não muda (desligado). Ainda não há
  relatos de jogos nessas GPUs: por favor, envie o seu. `[DlssNr] AmdLmxxfFastMode=false` (ou a caixa Fast mode) o
  desliga.

### APUs de portáteis (experimental, novo no 0.3.4)

O lmxxf roda em APUs de portáteis com 12 ou mais unidades de computação, pelo backend RDNA 3 do AMDNR, por 3zwr1:
**Z1 Extreme, Z2 e Radeon 780M** (gfx1103), **Z2 Extreme, Radeon 890M e 880M** (gfx1150). É experimental e lento. A linha Neural runtime diz "experimental" depois do crédito do RDNA 3.
Primeiros resultados de um tester (ROG Ally, Z1 Extreme): a sonda do lmxxf fora de um jogo, 54.7 ms por execução da
rede no tamanho 360p, 110.9 ms em 576p; num jogo, o teste de um tester (Shadow of the Tomb Raider, 1280x720 com XeSS, preset Handheld),
62 ms por execução da rede em média em 360p com o modelo a cada 4 quadros, cerca de 29 fps com o NR ligado. **O 0.3.5
tira cerca de 10% desses números na classe Z1 Extreme** (Z1 Extreme, Z2, Radeon 780M: cerca de 50 ms em 360p, cerca
de 105 ms em 576p, mesma imagem bit a bit, conferida por hash por um tester); os módulos do Z2 Extreme / 890M / 880M
não mudam.

- **Não suportados:** Z1 e Radeon 740M (4 unidades de computação), Radeon 760M (8), Radeon 860M / 840M. O runtime
  do danielblnc não roda em APUs de portáteis. As placas de desktop RX 6000 (RDNA 2) rodam o runtime 0.6.0 do
  danielblnc (veja "Requisitos"; o lmxxf ali está planejado para o 0.3.6); o Steam Deck e outras APUs RDNA 2 não são
  suportados.
- **O que ele faz sozinho** (só enquanto o seu ini não tiver um valor próprio): o tamanho da rede é limitado a 576
  (1024x576; desde o 0.3.5.5R3, antes era 360p), e desde o 0.3.5.6 o Fast mode vem ligado por padrão, então a rede roda
  um tamanho abaixo, em 360p (640x360); o modelo roda a cada 4 quadros (Model interleave; não é salvo). Desde o
  0.3.5.5R3 os seus Neural passes são mantidos (antes ficavam presos em 1); o custo de NR os leva em conta.
- **Velocidade, com honestidade:** Como
  referência: uma RX 7800 XT (60 unidades de computação) precisa de 34.4 ms por execução da rede no tamanho 720;
  esses chips têm de 12 a 16 e rodam com clocks mais baixos. Espere uma grande queda de quadros mesmo em 360p com
  o modelo a cada 4 quadros, algum ghosting pelo interleave longo e uma imagem mais suave que numa GPU de desktop.
  O custo de NR no fim da linha de status da aba Neural (e em Diagnostics) mostra o número real no seu aparelho.
- **Configurações:**
  - Mais nítido, porém mais lento: `[DlssNr] AmdLmxxfFastMode=false` (o tamanho de rede 1024x576;
    `[DlssNr] AmdLmxxfTierCap=360` mantém o menor tamanho mesmo com o Fast mode desligado).
  - Com o Fast mode ligado, um render em 720p ou 800p com NR resolution em 100% já roda o tamanho 360p, então uma NR
    resolution menor não barateia.
  - Model interleave em Off é salvo como `[DlssNr] AmdInterleave=1` (também desligado), para que o padrão do
    portátil não volte no próximo início. Para desligar à mão, escreva 1, não 0.
  - Preset > **Handheld** (desde o 0.3.5.1) define NR resolution em 80%, Dynamic NR desligado, o modelo a cada 4 quadros com Edit accumulation, 1 Neural pass, Full network ligado, Fast mode ligado e Before upscaling, mais valores de visual próprios (Residual limit 0.32, Temporal stability 0.70, Sharpening 0.10, Output smoothing 0.65 e mais alguns; a dica do botão lista todos). O botão só aparece nessas APUs; Quality, Balanced e Performance também mantêm aqui o tamanho de rede do portátil (o menu avisa).
- **FSR 4:** O FSR 4 (INT8) está disponível como opção experimental nos portáteis RDNA 3 (aba Upscaling); não validado pela AMD.
  A caixa **FSR 4 (INT8) - Experimental on this GPU (restart)** grava `[FSR] Fsr4ForceModel=2`; ele nunca liga sozinho
  (com `Dx12Upscaler=auto` o upscaler é o XeSS). Cerca de 1,5-3 ms por quadro num Z1 Extreme (estimativa); com NR,
  FSR 4 ou o tamanho 576, não os dois.
- **Shadow of the Tomb Raider** (e jogos que criam o device D3D12 duas vezes) não trava mais quando o upscaler
  inicia (corrigido no 0.3.4).
- **Driver:** use o driver Adrenalin da própria AMD. O lmxxf precisa do HIP (`amdhip64_7.dll`), que alguns drivers
  de fabricantes de portáteis não trazem; o `amd_bridge.log` então diz que o HIP não está disponível.
- **Movimento com Model interleave (corrigido no 0.3.5):** a edição carregada sumia assim que você se movia ("o
  efeito some ao se mover"): com Model interleave a rede roda em quadros de duração desigual, e a proteção da edição
  carregada supunha quadros iguais. Agora ela lê os vetores do quadro anterior com a duração deste quadro (nos dois
  runtimes).
- **Use juntos os arquivos de uma mesma release:** substitua o `OptiScaler.dll`, o `LmxxfNrRuntime.dll` e o
  `LmxxfNrRuntime.pak` juntos; o runtime recusa um portátil quando o `OptiScaler.dll` é anterior
  ao 0.3.4 ("this handheld needs OptiScaler.dll 0.3.4 or newer").
- **Testers com um portátil:** peça no Discord o kit de teste para portáteis (`handheld-test.zip`). O
  `run_probe.bat` dele mede a rede no seu aparelho e grava `handheld_result.txt` (seu nome de usuário do Windows
  fica mascarado).

## AMDNR Anywhere (preview, novo no 0.3.5)

**O que é.** Neural Rendering para jogos que não têm DLSS, XeSS nem FSR 2 próprios - e nada é gravado na pasta do
jogo. O AMDNR roda dentro de um host de captura de janela: o host captura a janela do jogo, a escala para a sua tela,
e o Neural Rendering roda sobre a imagem capturada; o nosso menu é desenhado dentro do host (a sua tecla
do menu, `INSERT` por padrão) com uma aba **Anywhere** própria. O host é o
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)**, numa build própria do AMDNR: o `Magpie.exe` dela é recompilado com as mudanças do AMDNR (a oferta de código-fonte, um diff a partir da tag do fork e as opções de build ficam ao lado, em `Anywhere-host-source/`) e o pacote não traz nenhum arquivo da NVIDIA. O AMDNR Launcher o baixa da release do AMDNR no GitHub (cerca de 130 MB no 0.3.5.6, uma vez por versão do host).

**Como usar.** No AMDNR Launcher, um jogo sem upscaler mostra **PLAY ANYWHERE** no lugar de INSTALL. Clique nele: o
launcher baixa o host (na primeira vez), inicia o jogo, e o host captura a janela dele. Rode o jogo **em janela ou em
janela sem bordas**, não em tela cheia exclusiva, e aperte a sua tecla do menu para abrir o menu do AMDNR dentro do
host. Um jogo com upscaler próprio continua na rota normal de INSTALL: o Anywhere é para os jogos que não têm nenhum.

**As configurações do host ficam no menu**, em Host settings, na aba Anywhere (o launcher também tem um painel
próprio, veja abaixo): o tamanho da
janela do jogo (720p / 900p / 1080p - uma recomendação do que definir no jogo; o host captura a janela que o jogo
abrir, seja qual for), a lista de estágios (V1: um passe de FSR 3 para a tela; V2: FSR 3 em 1x e depois um passe de
preenchimento), o nível de NR (Auto / 720 / 900 / 1080), VRR, o ritmo dos quadros e a **taxa de quadros do host**
(Default = a taxa de atualização da sua tela, no máximo 60; Auto = a taxa que a rede sustentou na última sessão; de
30 a 120; Display refresh = sem limite). Elas valem no **próximo** PLAY ANYWHERE, e o launcher mostra um resumo
somente leitura ao lado do botão. No `OptiScaler.ini` do próprio host elas são `[DlssNr] AnywhereWindow`,
`AnywhereEffect`, `AnywhereNrTier`, `AnywhereVrr`, `AnywherePacing` e `AnywhereHostFps`. Limite o jogo também, com o
limitador dele, entre 60 e 90 fps: o host só consegue mostrar quadros que o jogo desenhou, e cada quadro do host roda
a rede uma vez. **Desde o AMDNR Launcher 0.3.5.25 o launcher também tem um painel de configurações do Anywhere** (o
runtime de NR, Neural passes, o tamanho neural, FSR Auto / FSR 4 / FSR 3 / Off - Off mantém o NR -, geração de quadros
(experimental), movimento para o upscaling, nitidez, e Model E / Model L (experimentais; com o Neural Rendering dentro
do Anywhere eles ainda estão em teste)); ele grava o mesmo ini do host e vale no próximo PLAY ANYWHERE.

**O que o host não consegue dar à rede.** Uma janela capturada não tem profundidade, vetores de movimento, jitter
nem exposição próprios; o host estima o movimento. Por isso a aba Anywhere diz o que é estimado; as linhas da aba
Neural que não podem agir ali ficam ocultas ou são recusadas com um motivo (Ray Regeneration, Screen GI e Model
interleave - ele gastaria a edição carregada sobre um movimento estimado); e uma linha de status na página Anywhere
diz quando a rede passa do orçamento de quadro do host e o que abaixar. **Uma janela de jogo de 1920x1080 ou menor é
o caso exato em pixels:** uma janela maior é reduzida primeiro ao teto da rede e ampliada de volta, e a página diz
isso, com a fração dos pixels da tela que a rede viu. A linha NR resolution mostra o tamanho real da rede dentro do
host.

**Estado: preview.** Só RX 9000 (RDNA 4) nesta release; a RX 7000 vem depois de ser testada ali. Uma leve cintilação
ou um leve engasgo em movimentos rápidos pode permanecer (abaixe o nível de NR para 720, limite o jogo a 60-90 fps,
deixe o Model interleave desligado - o host o recusa). O host de captura é uma build própria do AMDNR, baixada da release do AMDNR no GitHub. Relatos: o zip do **Save report** do menu dentro do host (o título dele cita o jogo escalado),
ou o COLLECT LOGS do launcher.

## Model E e Model L (experimental, opcional, novo no 0.3.5.6)

Mais duas opções de Super Resolution (upscaler) ao lado do FSR e do XeSS. **Elas não são trabalho do AMDNR nem modelos
do AMDNR, e nenhuma delas está no zip do AMDNR.** Quando você escolhe uma no AMDNR Launcher (GAME SETTINGS de um jogo >
**SR model**: Auto / Model E / Model L), o launcher mostra uma vez o aviso daquele modelo, pede que você o aceite e baixa
os arquivos da release do próprio autor, conferidos por hash. O AMDNR não hospeda nem distribui os arquivos deles nem
nenhum arquivo da NVIDIA. Os dois são experimentais: espere problemas, e envie um relato quando algo der errado.
**Auto** mantém a rota normal do jogo (e tira os arquivos do Model L da pasta do jogo).

**Model E - HelixSR, de lonewolf0622** (<https://github.com/lonewolf0622/HelixSR>), sob a licença própria dele.
- O HelixSR monta a rede dele no seu PC a partir do arquivo DLSS da NVIDIA, que o launcher baixa do GitHub da própria
  NVIDIA (ou pega do seu jogo); o que ele monta fica no seu PC.
- No jogo: Upscaling > **HelixSR (Model E) - restart required** (jogos D3D12; `[Upscalers] FfxSRProvider=helix_e`).
  Passa a valer no próximo início do jogo.
- Funciona com **os dois** runtimes de NR, danielblnc e lmxxf. Nos nossos testes ele rodou num jogo numa RX 9070 XT.

**Model L - d4r, de countervolts** (<https://github.com/countervolts/d4r>), sob a licença própria do d4r.
- **O que é:** o d4r roda **o runtime DLSS da NVIDIA, sob a EULA da NVIDIA, por meio do ZLUDA / HIP** numa placa AMD. O
  launcher baixa o pacote do d4r da release do próprio d4r no GitHub e o coloca na pasta do jogo como `AMDNR-SR-L\`;
  o AMDNR não hospeda nem distribui nenhum arquivo da NVIDIA. **O autor do d4r não o testou no Windows.**
- **Só RX 9070 / RX 9070 XT** (testado numa RX 9070 XT; a RX 9060 XT e placas mais antigas são recusadas) e, no jogo,
  **só jogos D3D12**.
- **Ele roda com o runtime lmxxf, ou com o Neural Rendering desligado. O runtime do danielblnc faz par com o Model E:**
  com o danielblnc como runtime de NR a linha do Model L fica em cinza ("Model L runs with the lmxxf runtime (or with
  Neural Rendering off); danielblnc's runtime pairs with Model E. Pick lmxxf under Neural Rendering first." - em
  português: o Model L roda com o runtime lmxxf (ou com o Neural Rendering desligado); o runtime do danielblnc faz par
  com o Model E; escolha primeiro o lmxxf em Neural Rendering), e um jogo iniciado com os dois roda o seu upscaler FSR
  no lugar.
- No jogo: Upscaling > **Model L (d4r) - restart required**, em cinza com o motivo quando não pode rodar ali (não é um
  jogo D3D12, não foi preparado no launcher, não é uma RX 9070 / 9070 XT, o runtime do danielblnc, ou ainda falta
  reiniciar). Escolhê-lo grava `[DlssNr] AmdSrModel=L` (escolher outro upscaler remove essa linha); passa a valer no
  próximo início do jogo.
- **Experimental:** a velocidade dele ainda não foi ajustada e ele pode rodar bem mais devagar que o FSR. Se ele não
  conseguir iniciar, o jogo deve manter o seu upscaler FSR (não o FSR 2.1.2), e o `OptiScaler.log` diz o porquê numa
  linha `[MODELL`.

Dentro do **AMDNR Anywhere** os dois são oferecidos nas configurações do Anywhere do launcher; junto com o Neural
Rendering ali eles ainda estão em teste.

## Limites conhecidos (0.3.5.6)

- **O ghosting com o lmxxf diminuiu, mas não sumiu:** um rastro mais curto ainda pode seguir objetos em movimento
  rápido.
- **O Fast mode na RX 7000 e nos portáteis** é um padrão novo, ainda sem relatos de jogos: por favor, envie o seu.
  `[DlssNr] AmdLmxxfFastMode=false` volta ao tamanho de rede anterior.
- **O Full Model Look depois do upscaling custa mais**, porque a rede recebe a imagem no tamanho da tela. Onde a
  verificação de custo ou o registro de TDR deste jogo diz que a placa não aguenta, ele roda antes do upscaling e a
  linha de status da aba Neural diz isso; o seu próprio `[DlssNr] AmdPlacement=pre` também o mantém antes do upscaling
  (avisado uma vez: escolha o Full Model Look de novo em NR style, ou apague a linha).
- **Fallback para o FSR 2.2:** em alguns jogos D3D11 e Vulkan um upscaler que falha ainda pode cair para o FSR 2.2.
- **O Model L** é só para RX 9070 / RX 9070 XT e D3D12, não faz par com o runtime do danielblnc, é experimental e pode
  ser bem mais lento que o FSR; o d4r não foi testado pelo autor dele no Windows.
- **Model E e Model L dentro do AMDNR Anywhere**, junto com o Neural Rendering, ainda estão em teste.
- **danielblnc na RX 7000:** o modo no mesmo quadro do runtime dele custa muito tempo por quadro no RDNA 3, por
  design; use o lmxxf ali.
- **Linux / Proton:** o Neural Rendering não roda ali (veja abaixo).

## Linux / Proton (Steam Deck, Linux de desktop)

O AMDNR roda sob Proton e Wine como uma build do OptiScaler. **O Neural Rendering não roda no Linux (só no Windows):**
os dois runtimes de NR precisam do HIP do driver AMD para Windows, que o Proton e o Wine não fornecem. Com o NR ligado
sob Proton, o NR não roda, e desde o 0.3.5 a aba Neural e o relatório dizem isso (em vez de "Idle"). Isso é esperado,
não é um crash nem uma instalação quebrada. O AMDNR
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
- **Títulos Vulkan** (jogos RTX Remix, id Tech 8), no Proton como no Windows: o Ray Reconstruction e a geração de
  quadros própria do AMDNR são respondidos com "not supported" por design (o denoiser é D3D12, e os buffers de ray
  tracing ficam no device Vulkan); desde o 0.3.5 as abas Ray Regeneration e Frame Gen dizem isso em vez de pedir que
  você os ligue num jogo que os deixa em cinza. O DLSS super resolution funciona pela ponte.
- Desde o 0.3.5, `[Spoofing] Dxgi=true` sob Proton (o caminho de upgrade para o FSR 4) não dá mais falha na
  inicialização.

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

1. Baixe o `AMDNR-vX.X.X.zip` da página de releases (no 0.3.5.6: `AMDNR-v0.3.5.6.zip`). Os zips de runtime do
   danielblnc (`v0.6.0-Runtime.zip` e os outros) só são usados pelo Neural Rendering, então você não precisa deles
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
- Uma GPU AMD com AMD Software: Adrenalin Edition 26.9.1 ou mais recente. O runtime neural usa HIP pelo driver; não é preciso HIP SDK nem
  modo desenvolvedor (exceto na RX 6000, abaixo). Quais chips:
  - RX 9000 (RDNA 4): os dois runtimes.
  - RX 7000 (RDNA 3, desktop e mobile): os dois runtimes - o lmxxf pelo backend RDNA 3 do AMDNR, mais lento que
    no RDNA 4 (o nível de tamanho da rede e, desde o 0.3.5.6, o Fast mode vêm ligados por padrão, veja acima).
  - Strix Halo (Radeon 8060S / 8050S): lmxxf.
  - APUs de portáteis com 12+ unidades de computação (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M / 880M): lmxxf,
    experimental e lento. Z1 (4 CU), 760M / 740M e 860M / 840M: não suportados.
  - RX 6000 (RDNA 2, desktop): o runtime 0.6.0 do danielblnc (`v0.6.0-Runtime.zip`), com o HIP SDK da AMD (HIP 7.2)
    instalado, que coloca o runtime HIP de que a RX 6000 precisa numa pasta própria (o AMDNR procura ali); o lmxxf
    ainda não roda na RX 6000 (planejado para o 0.3.6). Steam Deck e APUs RDNA 2: não suportados (para o Neural
    Rendering; para os upscalers sob Proton, veja "Linux / Proton").

  A aba Neural diz o que sua GPU consegue rodar (passe o mouse nas entradas de runtime, ou veja a linha GPU em
  Diagnostics).
- **AMDNR Anywhere** (preview): Windows, uma placa RX 9000 (RDNA 4) nesta release e o AMDNR Launcher, que baixa o
  host de captura; o jogo roda em janela ou em janela sem bordas. Veja "AMDNR Anywhere".
- **Model E / Model L** (experimentais, opcionais): o AMDNR Launcher, que os baixa, e um jogo D3D12; o Model L também
  precisa de uma RX 9070 / RX 9070 XT e do runtime lmxxf (ou do Neural Rendering desligado). Veja "Model E e Model L".
- Um jogo Direct3D 12, Direct3D 11 ou Vulkan. O caminho neural AMD em si é D3D12; jogos D3D11 e
  Vulkan o alcançam pela ponte D3D12 do OptiScaler, o que significa que o upscaler precisa ser um dos
  backends "w/Dx12" (`ffx_12`). Deixe `Dx11Upscaler` / `VulkanUpscaler` em `auto` e esta build o
  escolhe por você quando o neural rendering está ligado. Com o Neural Rendering ligado, a lista de Upscaling os chama de "... w/Dx12 - Neural".
- Cerca de 2 GB de VRAM livre em resoluções de renderização da classe 1080p.

## O que há nos arquivos

**AMDNR-vX.X.X.zip**

| Arquivo | O que é |
|---|---|
| `OptiScaler.dll` | OptiScaler com o backend AMD do DLSS-NR (AMDNR 0.3.5.6). Renomeie como o guia indica. |
| `OptiScaler.ini` | Configurações. O Neural Rendering vem ativado; o log vem ligado para que um relato tenha o que anexar. |
| `LmxxfNrRuntime.dll` | O runtime neural lmxxf (0.3.5.6: um teste, no histórico da rede, de onde veio cada pixel, que encurta os rastros de ghosting, e um quadro de reset que não pisca mais com tanta força; ele precisa vir da mesma release que o `OptiScaler.dll`. 0.3.5: confere cada módulo HIP do pak com a lista de digests do próprio pak antes de usá-lo, cita os dois lados quando nenhum adaptador HIP corresponde à GPU do jogo, mantém os passos de resolução dinâmica sem reconstruir a rede e não lê mais as variáveis de ambiente do lmxxf que alteram a imagem; os kernels do lmxxf, incluindo os do lmxxf 0.31, os kernels c32w do AMDNR, os tamanhos de rede pequenos e a máscara nativa de personagens). Usado só quando escolhido; lê o `LmxxfNrRuntime.pak` ao lado, veja "O runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Os pesos, módulos HIP e shaders do runtime lmxxf em um arquivo criptografado (465 MB; 0.3.5: o conjunto de módulos para a RX 7000 e para os portáteis da classe Z1 Extreme - Z1 Extreme, Z2, Radeon 780M - é cerca de 10% mais rápido, mesma imagem; a RX 9000 recebe os módulos do lmxxf 0.37 / 0.39 do Kien (MIT) ao lado do seu conjunto base sem mudança; os módulos do Z2 Extreme / 890M / 880M e do Strix Halo não mudam). Só o runtime lmxxf o lê; é inofensivo mantê-lo junto com o runtime do danielblnc. |
| `OptiScaler\` | FSR, XeSS, o denoiser FidelityFX e o D3D12 Agility SDK que o OptiScaler usa. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | O dlssg-to-fsr3 do Nukem9, sem modificações e renomeado: as chamadas de DLSS Frame Generation do jogo servidas pela geração de quadros do FSR 3, também em Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, veja `Licenses/`. |
| `Licenses\`, `LICENSE` | Licenças de terceiros, o aviso do AMDNR (`AMDNR_NOTICE.txt`) e a licença GPL-3.0 desta build. |
| `SHA256SUMS.txt` | Checksums de cada arquivo deste zip e dos arquivos dos zips de runtime do danielblnc que ele lista. |

**Fora deste zip** (o AMDNR Launcher baixa cada um, conferido por hash, só quando você o escolhe): os zips de runtime
do danielblnc (abaixo); o host de captura do AMDNR Anywhere (build própria do AMDNR, da release do AMDNR no GitHub); o
HelixSR do Model E e o d4r do Model L, das releases dos próprios autores (veja "Model E e Model L"); e o OptiPatcher,
das releases do projeto OptiScaler (GAME SETTINGS).

**Os zips de runtime do danielblnc** (DLSS-NR on AMD by Daniel Blanco, sem modificações, com a permissão dele; use um)

Qual usar: o `v0.6.0-Runtime.zip` (recomendado) da Alpha0.3.5.5R4 - na RX 9000 e na RX 7000, e o único para a
RX 6000; o `v0.5.1-Runtime.zip` (Alpha0.3.5), o `v0.5.0-Runtime.zip` (Alpha0.3.4.2), o `v0.4.3-Runtime.zip`
(Alpha0.3.4.2 e Alpha0.3.4.1), o `v0.4.1-Runtime.zip` e o `v0.4.0-Runtime.zip` (Alpha0.3.4.1) continuam aceitos na
RX 9000 e na RX 7000. O runtime lmxxf não precisa de zip de runtime na RX 7000 e na RX 9000; APUs de portáteis usam só
o lmxxf. O AMDNR Launcher oferece 0.6.0 (recomendado, listado primeiro), 0.5.1, 0.5.0, 0.4.3, 0.4.1 e 0.4.0 para a sua
placa, e escolhe para você.

| Zip | Release | Runtime do danielblnc |
|---|---|---|
| `v0.6.0-Runtime.zip` | Alpha0.3.5.5R4 | 0.6.0, **recomendado na RX 9000 e na RX 7000, e o da RX 6000** (com o HIP SDK da AMD instalado); as configurações do runtime do danielblnc funcionam com ele |
| `v0.5.1-Runtime.zip` | Alpha0.3.5 | 0.5.1, ainda aceito; as configurações do runtime do danielblnc funcionam com ele |
| `v0.5.0-Runtime.zip` | Alpha0.3.4.2 | 0.5.0, ainda aceito (ele pode congelar alguns jogos em algumas placas: um mais novo é mais seguro); as configurações do runtime do danielblnc funcionam com ele |
| `v0.4.3-Runtime.zip` | Alpha0.3.4.2 (e Alpha0.3.4.1) | 0.4.3, ainda aceito; as configurações do runtime do danielblnc funcionam com ele |
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1 (e Alpha0.3.4) | 0.4.1, ainda aceito. Network style, Tone curve, Black lift e Game exposure ficam em cinza com ele |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1 (e Alpha0.3.4) | 0.4.0, ainda aceito; as configurações do runtime do danielblnc funcionam com ele |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3, aposentado: não é mais recomendado. Continua funcionando se você já o tem; as configurações do runtime do danielblnc funcionam com ele |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1; as configurações do runtime do danielblnc ficam em cinza com ele |

**O danielblnc 0.6.0 é o runtime do danielblnc recomendado** (o launcher o lista primeiro para a sua placa). Desde o
0.3.5.5R4 o AMDNR roda o runtime dele no modo no mesmo quadro: sob o AMDNR, o modo assíncrono dele completava três
tarefas e depois mais nada, então um `Async=1` no `dlssnr_on_amd.ini` dele roda como mesmo quadro, com um aviso único
(`[DlssNr] AmdDanielAsync=true` o deixa passar). Desde o 0.3.5.6 o runtime do danielblnc faz par com o Model E, não com
o Model L. Uma build do danielblnc mais nova que a 0.6.0 não é acionada por esta release.

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
- **Neural runtime** (danielblnc / lmxxf, com a versão que roda, por exemplo `danielblnc 0.6.0` ou `lmxxf 0.41`; desde o 0.3.5.6 a entrada do lmxxf nomeia o conjunto de kernels do lmxxf que roda, não a release do AMDNR) com uma palavra de estado: running, restart the game to switch, not
  installed, not for this GPU ou stopped. Abaixo, o crédito do runtime ativo e uma linha de status, por exemplo
  `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms` (o último número é o custo do NR), e uma linha **Live** fechada com mais
  detalhes. Quando algo
  precisa da sua atenção, aparece uma linha laranja, com um botão quando há correção (Retry lmxxf, Switch to
  danielblnc, Open Upscaling). No estado padrão não há nenhuma.
- **Preset**: Quality / Balanced / Performance colocam NR resolution em 100 / 85 / 70% e desligam o Dynamic NR;
  nada mais. Em APUs de portáteis há um quarto botão, **Handheld** (veja "APUs de portáteis"). **NR style** (primeiro
  os três visuais - **Full Model Look**, o padrão desde o 0.3.5.6, **NVIDIA look** e **AMDNR soft** - e depois quatro
  estilos com nome; veja "Configurações que vale conhecer"), e **Style slots** (Store / Apply / Clear).
- **Performance**: **Neural placement** (Before / After upscaling, novo no 0.3.5; veja "Configurações que vale
  conhecer"), NR resolution (%) com o custo (de 25 a 200% desde o 0.3.5.6), Neural passes, **Refine passes**
  (danielblnc, novo no 0.3.5.6), Full network, Fast mode, Dynamic NR resolution, Model
  interleave (Interleave preset e a linha de ritmo aparecem abaixo dele enquanto está ligado).
- **Quality**: Residual strength, Residual limit, Temporal stability, Sharpening (CAS) e **More quality options**
  (Network history - uma só caixa para os dois runtimes -, Output smoothing, Stability mode, Residual temporal,
  Residual edge fade, Still-surface steadiness).
- **Image look**: Colour composition, Detail e Colour strength, e três seções recolhíveis: **Model strength** (Tone
  e Structure intensity, Character structure, Edit detail / colour, Edge guard, Native character mask, e Network
  style, Tone curve e Black lift do danielblnc), **Exposure and highlights** (Auto-exposure, o limite de altas
  luzes dela, Highlight colour guard, **Game exposure** Auto / On / Off) e **Appearance filter** (com a palavra off / on
  depois do nome; o controle **Anti-halo** dele se chamava Halo protection antes do 0.3.5.6). Um "default" ou "custom"
  discreto depois do nome de uma seção recolhível diz se você mudou algo dentro dela.
- **Ray Regeneration**: não fica mais nesta aba desde o 0.3.5.3; os controles e o status dele ficam na aba **Ray Regeneration** própria (logo depois de Upscaling). Aqui só a linha laranja abaixo do runtime pode dizer "Ray Regeneration is off in this title" ou "Ray Regeneration could not start on this GPU" (passe o mouse nela para ver o motivo).
- **Screen-space GI** (seção nova no 0.3.5.3): **AMDNR Screen-space GI**, desligado por padrão, com um "by 3zwr1" discreto depois do nome; abaixo dele, enquanto está ligado: GI quality (Low / Medium / High / Ultra; o Auto escolhe um para a sua GPU até você apertar um, e a etiqueta dele mostra o tempo de GPU medido, ou uma estimativa até o GI rodar), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) e Reset GI. Em toda GPU AMD, com ou sem runtime de NR; dentro do AMDNR Anywhere aparece em cinza com o motivo.
- **A linha de ferramentas**, fechada no início: **Diagnostics** (Network output, Debug view, Edit shaper A/B, NR
  cost, as leituras de ghosting e de autoajuste, a linha GPU, **Save report**; a visualização de depuração do RR está
  na aba Ray Regeneration desde o 0.3.5) e **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy); **Experimental** só aparece enquanto o seu ini tiver ligado o Screen-space GI herdado e retirado (`[AmdRtgi] Enabled`), e só com a caixa dele, marcada "retired".

Desde o 0.3.5.6 um controle que o runtime ativo não consegue rodar fica oculto nesse runtime, com uma contagem (por
exemplo Refine passes e Tone intensity no lmxxf; Edit detail, Edit colour, Edge guard, Output smoothing e Full network
no danielblnc); a chave dele no ini continua funcionando. Um controle que só funciona em alguma condição continua ali,
em cinza com o motivo (por exemplo Residual limit, Debug view, Neural passes); trocar de runtime não move nenhuma
outra linha.

**As outras abas:** Upscaling começa com o upscaler, uma linha de status e Render resolution (os antigos Upscale
Ratio Override e Output Scaling); numa placa que não é NVIDIA, "DLSS w/Dx12" não aparece mais. Desde o 0.3.5.6 a lista
de upscalers também tem **HelixSR (Model E) - restart required** (jogos D3D12) e **Model L (d4r) - restart required**
(em cinza com o motivo onde não pode rodar; veja "Model E e Model L"). **Ray Regeneration**
(nova no 0.3.5) vem depois de Upscaling enquanto o Ray Regeneration roda no título: as linhas de status (por placa e
por API), a linha **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): Automatic é o da AMD na RX 9000 e o AMDNR Ray Denoiser em todas as outras placas, os três primeiros trocam com o jogo rodando, e Off diz ao jogo que o Ray Reconstruction não é suportado, então ele fica com o denoiser dele, depois de reiniciar), os controles, More Ray Regeneration options e um bloco
Diagnostics próprio com a visualização de depuração do RR e o número de ruído (granulado na entrada e na saída,
cintilação com a câmera parada); ela nunca é desenhada dentro do AMDNR Anywhere. Image tem Sharpness, Textures, Init
Flags e o Magnifier. Frame Gen começa com FG Input e FG Output e, desde o 0.3.5, uma linha que nomeia o passo que
ainda falta antes de qualquer coisa ser gerada. Interface tem o overlay de FPS e Keybinds (um botão por tecla).
Advanced começa com Active Quirks, depois Display (V-Sync), Compatibility e Logging. Dentro do AMDNR Anywhere o menu
mostra uma aba **Anywhere** (a linha de captura e rede, as configurações do host) no lugar das abas Frame Gen e
Advanced. As configurações, as chaves e o que o Save Settings grava não mudam, exceto onde o `CHANGELOG.md` diz.

## Configurações que vale conhecer

Abra a aba **Neural**. Os padrões são o arranjo testado mais recente, então o primeiro passo útil é
mudar uma coisa por vez.

- **NR style** (topo da aba Neural, `[DlssNr] AmdNrLook = auto | 0 | 1`, nos dois runtimes) — os três primeiros itens
  são visuais completos. **Full Model Look** (`auto`, o padrão desde o 0.3.5.6): a rede do NVIDIA look (Style 0, todos
  os 71 blocos) com forças de modelo maiores - Structure intensity 2 nos dois runtimes, Edit detail 1.5 e Edit colour
  1.25 no lmxxf -, sem residual limit nem edge guard, Temporal stability 0.5, enquanto o output smoothing, o histórico
  da rede e as outras proteções de estabilidade continuam ligados; ele pede Neural placement After upscaling enquanto
  você não tiver definido o placement por conta própria (veja Neural placement abaixo). **NVIDIA look** (`0`, o padrão
  do 0.3.5.3 ao 0.3.5.5R4): o mais próximo da saída da própria NVIDIA, Before upscaling. **AMDNR soft** (`1`): o visual
  anterior do AMDNR, mais suave e mais estável. Um visual fornece os seus valores só enquanto você não tiver definido
  esses controles por conta própria, e sob Model interleave as proteções mantêm os valores anteriores. Escolher um
  visual no combo também grava o Neural placement dele (Full Model Look: After upscaling; os outros dois: Before) e o
  Full network como valores seus. Os quatro estilos com nome depois deles definem os seus controles deslizantes por
  cima do visual que estiver rodando.
- **NR resolution** — a principal alavanca de qualidade/custo. Abaixo de 100% o modelo trabalha numa
  imagem menor e só a sua *correção* é levada de volta ao quadro em resolução completa, então o quadro
  mantém o próprio detalhe. Acima de 100% o custo cresce com o quadrado (150% é 2.25x, 200% é 4x). Desde o 0.3.5.6 o
  controle vai de 25% a 200% nos dois runtimes (antes parava em 150%); a rede do lmxxf continua com o teto de
  1920x1080, e a linha diz isso quando esse é o limite. O controle
  anda em passos de 5%: cada novo tamanho de NR pode reter VRAM até o jogo reiniciar, então reinicie
  o jogo depois de muitas mudanças.
  O custo ao lado mostra 1.00x em 100%; no lmxxf é o preço do nível de tamanho da rede em que ele roda (a dica
  dele mostra o nível). Os botões Preset o colocam em 100 / 85 / 70%.
- **Neural placement** (Neural > Performance, `[DlssNr] AmdPlacement = pre | post`, novo no 0.3.5, nos dois runtimes)
  — onde o passe neural roda. `pre` (o padrão da própria chave, e o que toda build antes do 0.3.5.6 rodava) edita a
  imagem em resolução de render que o upscaler está prestes a ler. `post` edita, em vez disso, a imagem pronta do
  upscaler, em resolução de exibição: mais nítido, porque o upscaler não refiltra mais a edição, e mais caro - no lmxxf
  a rede roda no tamanho da tela até o teto dela de 1920x1080, então uma tela 1080p paga o nível máximo em qualquer NR
  resolution, e uma tela 1440p ou 4K recebe uma edição de 1080 linhas ampliada de volta (o danielblnc não tem esse
  teto, então o `post` custa mais para ele) - um pouco menos tolerante em movimento, e o HUD entra se o jogo o compõe
  antes do upscaling. **Desde o 0.3.5.6, enquanto a linha estiver vazia ou em `auto`, o Full Model Look padrão pede
  `post` em toda placa**; a verificação de custo, o registro de TDR deste jogo e a proteção contra TDR em tempo real
  calculam o preço dele e o levam de volta para `pre` durante a sessão quando a placa não aguenta (a linha de status
  da aba Neural e o `amd_bridge.log` dizem por quê). Um `pre` que você mesmo define (a linha, o preset Handheld ou o
  launcher) mantém o visual antes do upscaling; a linha de status diz isso, e escolher de novo o Full Model Look em NR
  style grava `post`. Recusado (volta para `pre`, uma linha no `amd_bridge.log`) dentro do AMDNR Anywhere, no modo de
  imagem final e depois que o Ray Regeneration rodou no título. A linha NR resolution mostra os dois tamanhos
  enquanto `post` roda.
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
- **Refine passes** (Neural > Performance, `[DlssNr] AmdRefineMode = full | refine`, só danielblnc, novo no
  0.3.5.6) — o modo de passes próprio do AMDNR. *Full* (o padrão) roda todos os passes no tamanho completo. *Refine*
  roda o passe 1 no tamanho completo e os passes 2 e 3 com a rede um tamanho menor, sobre a resposta do passe anterior,
  e depois leva a edição deles de volta para a imagem em tamanho completo: todos os passes continuam rodando, os extras
  custam menos. Sem efeito com 1 passe. O runtime do lmxxf monta um único tamanho de rede por sessão, então roda o
  Refine como Full e a linha fica oculta ali.
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
- **Mais detalhe nos rostos** (novo no 0.3.5.6, nos dois runtimes, sempre ligado) — enquanto o Character structure
  segue o Structure intensity (a caixa marcada, o padrão), rostos e pele recebem 1.5 vezes a estrutura da cena, no
  máximo 4 (com o Full Model Look: 2.00 -> 3.00); sem passe extra e sem custo extra. Um Character structure que você
  mesmo define sempre prevalece, e o valor em cinza no menu mostra o número elevado. `[DlssNr] AmdFaceDetail=false`
  (só no ini) o desliga.
- **Game exposure** (Image look > Exposure and highlights, `[DlssNr] AmdUseGameExposure = auto | 1 | 0`, nos dois
  runtimes) — desde o 0.3.5.6 os itens dele são **Auto / On / Off**. Auto usa o valor de brilho do próprio jogo quando
  o jogo publica um; On sempre o entrega à rede; Off nunca entrega, e a auto-exposição do próprio runtime assume -
  tente Off se o NR deixar um jogo escuro ou claro demais. Vale a partir do próximo quadro do modelo.
- **Anti-halo** (Image look > Appearance filter, `[AmdLook] AntiHalo`, 0..1, padrão 0.901; chamado Halo protection
  antes do 0.3.5.6, mesma chave) — limita a edição do filtro ao lado de bordas fortes para que não se forme nenhum anel
  claro ou escuro; mais alto = bordas mais seguras, detalhe um pouco mais suave ao lado delas. Precisa do Effect mix do
  filtro acima de 0.
- **A geração de quadros vem desligada num ini novo**, e são cinco passos para ligá-la: FG Input e FG Output na aba
  Frame Gen (por exemplo "DLSSG via Streamline" num jogo com geração de quadros DLSS, e XeFG), **Save Settings**,
  reiniciar o jogo por completo, ligar a geração de quadros **do próprio** jogo e, então, marcar **Active** em Frame
  Generation. Desde o 0.3.5 a aba e o log nomeiam o passo que está faltando. A lista completa, com o que desligar no
  jogo, está na FAQ abaixo ("Geração de quadros: sem ganho de fps?").
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
- **FSR Ray Regeneration** — RX 9000 (RDNA 4); na RX 7000 (RDNA 3) como preview, ligado por padrão, com o AMDNR Ray Denoiser como denoiser (veja abaixo); só em jogos que usam DLSS Ray Reconstruction (Cyberpunk 2077, Alan
  Wake 2), com o jogo rodando DLSS (spoofing ligado), ray tracing e Ray Reconstruction ativados nas
  próprias configurações. O Neural Rendering então roda depois dele, sobre a sua saída, o que custa
  mais: abaixe a NR resolution se a taxa de quadros cair. Desde o 0.3.5 os controles dele ficam na aba **Ray
  Regeneration** própria, logo depois de Upscaling, desenhada enquanto o Ray Regeneration roda no título; desde o 0.3.5.3 a aba Neural não tem nenhuma linha de Ray Regeneration, só a linha laranja abaixo do runtime quando o Ray Regeneration está desligado num título ou não conseguiu iniciar nesta GPU. A linha **Denoiser backend** da aba (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) oferece Automatic (AMD FSR Ray Regeneration na RX 9000, o AMDNR Ray Denoiser (preview) em todas as outras placas), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) e Off (the game's own); os três primeiros trocam com o jogo rodando, e Off diz ao jogo que o Ray Reconstruction não é suportado, para que ele fique com o denoiser dele (no próximo início do jogo). Num título **Vulkan** o Ray Reconstruction é "not supported" por design (o denoiser é
  D3D12) e a aba diz isso. O **perfil path-traced** (menos granulado nos rostos com path tracing) é opcional desde o
  0.3.3.1: marque-o ali para testá-lo em Resident Evil Requiem ou PRAGMATA. A mesma aba tem a intensidade da bias
  mask e a **suavização de pele** (experimental, para jogos que publicam um guia SSS; desligada por padrão, mas
  ligada por padrão em Resident Evil Requiem desde o 0.3.3.2); os controles de ajuste temporal ficam em *More Ray
  Regeneration options*, e a visualização de depuração do RR e o número de ruído (granulado na entrada e na saída,
  cintilação com a câmera parada) ficam no bloco Diagnostics próprio da aba. Na RX 7000 (RDNA 3) o Ray Regeneration é
  oferecido como preview (0.3.5.1; o 0.3.5 não o oferecia por padrão): o jogo pode ligar o Ray Reconstruction e os
  ajustes do Ray Regeneration aparecem, e o AMDNR Ray Denoiser (preview) tira o ruído dela, também nas APUs RDNA 3 / 3.5 (o AMD FSR Ray Regeneration é feito para RDNA 4); a página do Ray Regeneration diz o que está rodando. A caixa da aba Upscaling
  **Preview: Ray Regeneration on this card (restart)** (com a etiqueta "preview - on by default") vem marcada por
  padrão; desmarque-a, ou defina `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, e o jogo fica com o denoiser dele. RX 6000 e anteriores só o recebem como opção, com `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (aba Upscaling: **Offer Ray Regeneration on this GPU (restart)**, com a etiqueta "preview"), também com o AMDNR Ray Denoiser. **Nitidez depois do RR** (0.3.4.1): quando o
  jogo não envia nitidez, o AMDNR aplica nitidez de 0.25 depois do RR no Windows (0 no Linux / Proton); para
  desligar: Image > Sharpness, marque Override, controle deslizante em 0. Desde o 0.3.4.2 esse número tem a sua
  própria chave no ini, `[Sharpness] RrDefaultSharpness` (mesmo padrão 0.25): coloque 0.15, 0.10 ou 0 ali sem mexer
  no Override, e um valor que o seu ini guardou em `[Sharpness] Sharpness` com o Override desligado aparece no menu
  como à espera.
- **AMDNR Screen-space GI by 3zwr1** (desligado por padrão; Neural > Screen-space GI, ou `[AmdGi] Enabled=true`) - a luz rebatida e a oclusão ambiente em espaço de tela do próprio AMDNR, a partir da profundidade do jogo, antes do NR, do upscaler e da interface; funciona com o NR ligado ou desligado; Low / Medium / High / Ultra (Auto por GPU), cerca de 0.6 ms em High com render 1080p numa RX 9070 XT (medido). É espaço de tela: falta a luz que vem de fora da tela. Veja `CHANGELOG.md`.
- **Model E / Model L** (aba Upscaling, novo no 0.3.5.6) — opções de upscaler experimentais feitas por outros autores
  e baixadas pelo AMDNR Launcher; o Model L roda com o lmxxf ou com o NR desligado. Veja "Model E e Model L".
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
versão e diz o que fazer. Use o runtime recomendado, com as três DLLs de passe do mesmo zip:
o `v0.6.0-Runtime.zip` da Alpha0.3.5.5R4 (176,366,023 bytes, SHA256 começando com `d2bb1702`; na RX 9000, na RX 7000 e
na RX 6000); o `v0.5.1-Runtime.zip` (Alpha0.3.5), o `v0.5.0-Runtime.zip` (Alpha0.3.4.2; 149,550,553 bytes, SHA256
começando com `7a49ab0e`), o `v0.4.3-Runtime.zip` (Alpha0.3.4.2 e Alpha0.3.4.1; 116,484,918 bytes, SHA256 começando
com `07dd7774`), o
`v0.4.1-Runtime.zip` e o `v0.4.0-Runtime.zip` (Alpha0.3.4.1) continuam aceitos (o `dlssnr_amd_pass1.dll` do
`v0.4.1-Runtime.zip` tem 9,916,928 bytes, SHA256 começando com `823063eb`; no `v0.4.0-Runtime.zip`: 10,027,008
bytes, `d62be3d8`). Builds suportadas:
0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0 e os zips de runtime citados acima, até a 0.6.0. Não instale o instalador
próprio do danielblnc nem o `dxgi.dll` / `version.dll` /
`winhttp.dll` dele junto do AMDNR: o AMDNR já roda o runtime dele. **Uma build do danielblnc mais nova que a 0.6.0 não
é acionada por esta release:** a aba Neural nomeia o arquivo e a versão dele e diz isso. Com a 0.6.0, a 0.5.1, a 0.5.0,
a 0.4.3, a 0.4.1 e a 0.4.0 as configurações exclusivas do danielblnc (Network style, Tone curve, Black lift, Game
exposure, Fast mode) funcionam; desde o 0.3.5.5R4 um `Async=1` no `dlssnr_on_amd.ini` roda como mesmo quadro
(`[DlssNr] AmdDanielAsync=true` o deixa passar; veja "O que há nos arquivos").

**danielblnc: o NR parou depois de alguns "timeouts", ou a tela disse que o driver de vídeo reiniciou a GPU?**
Corrigido no 0.3.5.6 (testado no Silent Hill 2). A causa foi encontrada: com os arquivos do Model L na pasta do jogo,
o runtime do danielblnc recebia a cópia do HIP do próprio Model L, e a espera dele nunca via a tarefa terminar. O
AMDNR agora entrega o HIP do driver a todo chamador fora do Model L. Uma espera que desiste é relatada como tal e não
desliga mais o Neural Rendering; não houve reinício nenhum do driver, e esse texto sumiu; e o aviso "runtime failed"
que o Doctor do launcher lê só é gravado depois de uma falha real. O runtime do danielblnc faz par com o Model E; use
o lmxxf para o Model L.

**Você escolheu FSR 3 / FSR 4 e o jogo rodou o FSR 2.1.2?** Desde o 0.3.5.6 uma opção de upscaler que falha ou não
está pronta (Model E, Model L ou outra) deve manter a sua própria escolha de FSR (FSR 4 / FSR 3.1) em vez de cair para
o FSR 2.1.2 (D3D12); o FSR 2.1.2 só roda quando você o escolhe, ou quando nenhuma outra biblioteca FSR consegue rodar,
e o log diz isso. Em alguns jogos D3D11 e Vulkan um upscaler que falha ainda pode cair para o FSR 2.2. Se você ainda
vir isso, envie o zip do relatório.

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
Visto no Starfield num Ryzen de desktop. Desde o 0.3.5 a linha da aba Neural diz qual dos três casos é - ainda sem
runtime, NR rodando **na GPU integrada** (uma APU que um runtime aceita, como uma Radeon 780M ao lado de uma placa
Radeon: o NR roda ali, bem mais devagar que na placa) ou um runtime em outro adaptador - e o `amd_bridge.log` lista
cada adaptador uma vez.

**A linha de status do lmxxf mostra `c32w=off:nofile` numa RX 9070 / 9070 XT?** Uma pasta antiga
`DLSS5-AMD\native-game-tiled-assets` ao lado do `.exe` do jogo (sobra de uma instalação anterior do lmxxf) é
usada no lugar do `LmxxfNrRuntime.pak`. Ela não tem os kernels c32w, então o lmxxf roda na velocidade antiga.
Apague ou renomeie a pasta `DLSS5-AMD`: o pak tem tudo de que o lmxxf precisa. Um `LmxxfNrRuntime.pak`
anterior ao 0.3.3.2 mostra o mesmo status; troque-o pelo desta versão.
`fk=fff-` na mesma linha significa o mesmo (um pak antigo ou uma pasta solta): o lmxxf continua rodando,
na velocidade antiga.

**danielblnc: o estilo do NR ainda muda quando a NR resolution sai de 100%?** Continua aberto do 0.3.4 ao 0.3.5,
e o padrão não mudou. Em 100%, Residual strength 0.99 dá 99% de 1.00 (corrigido no 0.3.3.2);
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
O 0.3.5 acrescenta duas coisas para o Assetto Corsa: o AMDNR sai do caminho de um `nvngx.dll` estranho na pasta do
jogo e protege a criação do device D3D11On12, e um jogo DX11 que nunca apresenta pelo D3D12 recebe uma fila D3D12 de
bootstrap para o passe neural (`[DlssNr] AmdBootstrapQueue`, auto). Ainda não confirmado por um jogador do Assetto
Corsa.

**Uncharted: Legacy of Thieves Collection fechava alguns segundos depois de iniciar numa RX 9000 com o Neural
Rendering ligado?** Corrigido no 0.3.5: o jogo executa seu trabalho em fibras pequenas de 192 KiB, e a primeira
inicialização do HIP (o driver compila seus kernels auxiliares dentro do jogo) estourava essa pilha no primeiro quadro
de NR. O primeiro uso do HIP pela ponte neural agora roda numa pilha grande própria (`[DlssNr] BigStackCall`, auto).
Se você definiu `[DlssNr] Enabled=false` ali para o 0.3.4.2, ligue-o de novo. Ainda não confirmado por um jogador com
RX 9000: se continuar acontecendo, envie o zip do relatório.

**Um jogo com resolução dinâmica (The Last of Us Part II) pisca com o Neural Rendering ligado?** Corrigido no 0.3.5:
o jogo mudava o tamanho de render centenas de vezes por minuto, e cada passo reconstruía a rede e zerava o histórico
dela. Um passo que cabe na alocação agora é mantido (sem acomodação, sem aquecimento, sem zerar o histórico, nos dois
runtimes); um quadro maior ou uma queda real ainda realoca. A linha de estatísticas do relatório conta os passos
mantidos. Ainda não confirmado nesse jogo.

**Geração de quadros: sem ganho de fps, ou "restart the game" para sempre?** Em quase todos os relatos, a geração de
quadros simplesmente não estava ligada - desligada, não quebrada. São cinco passos, e desde o 0.3.5 a aba Frame Gen
e o log nomeiam o que está faltando:

1. Aba Frame Gen: escolha **os dois**, FG Input e FG Output. Um jogo DX12 com geração de quadros própria: o DLSS FG
   ou o FSR 3.1 FG dele é a entrada (jogos com FSR 3.1 FG: "FSR 3.1 FG", não "FSR 3.0 FG"); nenhum FG no jogo: FG
   Input = OptiFG (Upscaler), HUD fix ligado. DX11: só OptiFG. Vulkan: sem saída FSR FG / XeFG (use a do próprio
   jogo).
2. **Save Settings**.
3. **Feche o jogo por completo e abra de novo** - o FG não consegue ligar no meio de uma sessão de jogo.
4. Nas opções gráficas do próprio jogo, ligue a geração de quadros **dele**: DLSS Frame Generation (com DLSS como
   upscaler) para a entrada DLSSG, geração de quadros FSR (com FSR) para a entrada FSR 3.1 FG.
5. Abra o menu de novo, aba Frame Gen, marque **Active** em Frame Generation. Nada é gerado até essa caixa estar
   marcada (o XeFG pode pedir mais um reinício).

Depois, desligue **no jogo**: tela cheia exclusiva (o XeFG precisa de janela sem bordas), V-Sync e limites de quadros
(ou limite ao dobro dos seus fps base), e a geração de quadros XeSS do próprio jogo, se ele tiver uma (um gerador de
quadros por janela; um jogo que carrega o próprio XeSS FG recebe um aviso na aba). Sem ganho de fps = o FG está
desligado - o log diz `... Enabled is off ...: no frames are generated. Frame generation off, not broken.`; metade
dos fps = um limite ou o V-Sync está segurando os quadros gerados. Se ainda assim falhar, clique em **Save report**
depois da falha e poste o zip com o jogo, o par que você escolheu, qual opção de geração de quadros do jogo estava
ligada, janela sem bordas ou tela cheia, HDR ligado ou desligado, e o que você viu. A FAQ completa está fixada no
canal de suporte do Discord.

**Outros mods (RED4ext e Cyber Engine Tweaks do Cyberpunk 2077) ou ReShade junto do AMDNR?** Desde o 0.3.5 o
relatório e o log nomeiam os carregadores proxy dos outros mods (`Mod loaders:` no `report.txt`; o RED4ext é o
`winmm.dll`, o Cyber Engine Tweaks é o `version.dll` por um carregador ASI) e avisam quando o AMDNR ocupa o nome de
um carregador conhecido deste jogo sem encadeá-lo. O AMDNR Launcher nunca toma nem move o arquivo de um carregador:
ele escolhe outro nome de proxy (Cyberpunk 2077: `dxgi.dll`), e o Doctor dele nomeia qualquer carregador que um
INSTALL anterior pôs de lado (`AMDNR_backup`). O ReShade junto do AMDNR é detectado pelo recurso de versão do módulo
ou pelas exportações de add-on do ReShade, nunca pelo nome do arquivo, e nomeado no relatório e no log
(`[Game] ReShade detected: <module>`); nada é carregado, interceptado nem bloqueado, e fazer os dois compartilharem
o device D3D12 está projetado, mas ainda não construído.

**`No HIP adapter matches D3D12 LUID` na aba Neural ou no `amd_bridge.log`?** Desde o 0.3.5 a linha cita os dois
lados - o adaptador D3D12 do jogo (nome, LUID), cada dispositivo HIP (ordinal, nome, gfx, LUID) - e a causa provável
com o que fazer: o jogo roda na GPU integrada ou em outra placa (Configurações do Windows > Sistema > Tela >
Elementos gráficos: defina o jogo para a GPU dedicada), um runtime HIP sem identidade (um `amdhip64_7.dll` perdido ao
lado do jogo: remova-o), nenhum dispositivo HIP (instale o driver Adrenalin da própria AMD), a mesma placa sob outra
identidade, ou um adaptador que não é AMD. Os dois runtimes escrevem o mesmo texto.

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

**Você não encontra as configurações do Ray Regeneration?** Desde o 0.3.5 elas ficam na aba **Ray Regeneration**
própria, logo depois de Upscaling, desenhada enquanto o Ray Regeneration roda no título (e mantida pelo resto da
sessão de jogo depois que ele rodou); desde o 0.3.5.3 a aba Neural não tem nenhuma linha de Ray Regeneration. Enquanto ele não está rodando, a aba não é desenhada; se o Ray Regeneration estiver desligado neste título ou não conseguir iniciar nesta GPU, a linha laranja abaixo do runtime na aba Neural diz isso. Na RX 7000 e nas APUs RDNA 3 / 3.5 o Ray Regeneration é oferecido como preview, ligado por padrão, e o AMDNR Ray Denoiser (preview) tira o ruído dele (o AMD FSR Ray Regeneration é feito para RDNA 4); `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (ou a caixa da aba Upscaling **Preview: Ray Regeneration on this card (restart)**, desmarcada) desliga a preview para o jogo ficar com o denoiser dele. Em RX 6000 e anteriores ele não é oferecido por padrão: a caixa da aba Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) o oferece, com o AMDNR Ray Denoiser (preview). Para fazê-lo rodar, nas
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
remove por completo. Esse é um problema conhecido. O 0.3.5 põe um número nele: o bloco Diagnostics da aba Ray
Regeneration mostra o granulado na entrada e na saída e a cintilação com a câmera parada (medidos enquanto a aba está
aberta), e a linha **Denoiser backend** da aba pode ser posta em Off, o que diz ao jogo que o Ray Reconstruction não
é suportado, para que ele fique com o denoiser dele (depois de reiniciar). Desde o 0.3.5.3 a mesma linha também oferece o denoiser próprio do AMDNR, o AMDNR Ray Denoiser (preview), que o Automatic usa em todas as placas, menos na RX 9000.

**O Ray Reconstruction do jogo está ligado, mas a aba Neural diz "Ray Regeneration is off in this title"?**
O jogo não publica o que o FSR Ray Regeneration precisa: o plugin DLSS dele passa matrizes de câmera vazias
(Satisfactory), que o Ray Reconstruction da NVIDIA trata como opcionais e o FSR Ray Regeneration exige. O upscaling
FSR roda no lugar dele e o NR assume sua posição normal antes do SR; a aba Upscaling diz o mesmo. Desde o 0.3.4 ele
fica desligado a sessão inteira num jogo Unreal com essa assinatura. Desligue o Ray Reconstruction no jogo e
restaure as configurações de denoiser da própria engine.

**Um jogo Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) mostra "DX12 Error 0x80070057"?**
Esses jogos trazem a própria geração de quadros XeSS. Desde o 0.3.5 a aba Frame Gen mostra ali um aviso com uma
recomendação (deixe o XeSS FG do próprio jogo desligado, ou dois geradores dividem uma só janela) e a saída XeFG do
AMDNR continua rodando; a rota mais simples é a opção XeSS FG do próprio jogo com a geração de quadros do AMDNR
desligada. Se ainda acontecer, defina `[FrameGen] Enabled=false` e `[fakenvapi] ForceXeLL=false` e relate com o log.

**The Last of Us Part I trava ao iniciar?** É a inicialização do Streamline do próprio jogo, um
problema conhecido do OptiScaler: renomeie o `sl.common.dll` na pasta do jogo para
`sl.common.dll.bak` e escolha **FSR 3.1** nas configurações do jogo em vez de DLSS.

Notas completas de cada versão: `CHANGELOG.md` (no zip e no repositório).

## Roteiro

- **0.3.5.6** (esta build) — o **Model E** (HelixSR, de lonewolf0622) e o **Model L** (d4r, de countervolts) como
  opções de upscaler experimentais e opcionais, baixadas das releases dos próprios autores (Model L: RX 9070 /
  9070 XT, com o lmxxf ou com o NR desligado); o Full Model Look como NR style padrão, depois do upscaling onde a placa
  aguenta o custo; mais detalhe nos rostos; menos ghosting no lmxxf; Fast mode por padrão na RX 7000 e nos portáteis;
  Game exposure (Auto / On / Off) e Anti-halo; NR resolution até 200%; um menu que oculta
  o que um runtime não consegue rodar; Refine passes no danielblnc; a correção dos falsos timeouts do danielblnc; o
  FSR 3 / 4 mantido em vez do FSR 2.1.2; a contagem de memória do XeFG; AMDNR Launcher 0.3.5.25.
- **0.3.5.1 a 0.3.5.5R4** — danielblnc 0.6.0 (também na RX 6000, com o HIP SDK da AMD); lmxxf 0.41 na RX 9000; o
  NVIDIA look; o AMDNR Ray Denoiser (preview) e o AMDNR Screen-space GI (0.3.5.3); uma proteção contra travamentos
  depois de falhas de GPU contadas; correções de crash a partir dos seus relatos; o AMDNR Launcher redesenhado com
  ASK AI. Veja `CHANGELOG.md`.
- **0.3.5** — **AMDNR Anywhere** (preview, RX 9000, pelo launcher); a aba Ray Regeneration com linhas
  de status por placa e por API, a linha Denoiser backend em Off e o número de ruído; o passe neural depois do
  upscaling (`AmdPlacement`); a aba Frame Gen e o log dizem por que nada é gerado; o lmxxf 0.37 do Kien (MIT) na
  RX 9000; o conjunto de módulos 0.3.5 (cerca de 10% mais rápido na RX 7000 e nos portáteis da classe Z1 Extreme,
  mesma imagem); passos de resolução dinâmica
  mantidos sem reconstruir a rede; a correção da edição carregada nos portáteis; a pasta de runtime compartilhada
  (`AmdRuntimePath`); a recusa de adaptador HIP que cita os dois lados; os carregadores de outros mods e o ReShade
  nomeados no relatório; processos de anti-cheat e de relatório de crash deixados em paz; correções para Uncharted
  (RX 9000), Assetto Corsa, F1 25 e Kingdom Come: Deliverance II (Game Pass), Control Resonant com geração de quadros,
  Tainted Grail: The Fall of Avalon e Dead Space, GTA V Enhanced, Half-Life 2 RTX e outros títulos Vulkan, e os
  textos sobre Linux / Proton; AMDNR Launcher 0.3.5.1.
- **0.3.4.2** — hotfix: o menu do Assetto Corsa (a tecla do menu abre ou fecha o menu uma só vez por
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
- **0.3.6** — a RX 6000 (RDNA 2) com os kernels do próprio AMDNR (o runtime 0.6.0 do danielblnc já roda ali): atrás
  de uma verificação de hardware, com o nível 360 e o Model interleave como preview na Navi 21; o AMDNR Anywhere na
  RX 7000 depois de testado ali. (O AMDNR Ray Denoiser, o denoiser próprio do AMDNR atrás da chamada de Ray
  Reconstruction, já é distribuído como preview desde o 0.3.5.3.)
- **0.4.0** — o nível de rede 1440p; Ray Regeneration em títulos Vulkan pela ponte; Neural Rendering entre
  adaptadores (o jogo numa placa, a rede na placa AMD); o Anywhere além do preview (jogos sem upscaler próprio, em
  que o OptiScaler fornece o upscaler e o passo neural juntos).
- **Depois** — o AMDNR em qualquer janela (a área de trabalho).

---

## Créditos

Esta build é um trabalho de ligação sobre o trabalho de outras pessoas. Se ela lhe for útil, os
agradecimentos pertencem ao upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD by Daniel Blanco — https://github.com/danielblnc/DLSS-NR-on-AMD (os arquivos `*Runtime.zip`, sem modificações)
- **lmxxf** (Kien) — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (o port da rede, os kernels e o runtime HIP, MIT)
- **TheAutomatic** — `LmxxfNrRuntime.cpp`, `LmxxfNrApi.h`, `LmxxfProductionOptions.h`: portions contributed to lmxxf by TheAutomatic (MIT)
- **kernels do lmxxf 0.31** no `LmxxfNrRuntime.pak` (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)) — do lmxxf (Kien, MIT), compilados pelo AMDNR a partir das fontes e da receita de compilação do lmxxf; a parte do AMDNR é o carregamento, os pinos SHA-256, a seleção por GPU e os fallbacks
- **kernels do lmxxf 0.37, 0.39 e 0.41** no `LmxxfNrRuntime.pak` (os módulos lmxxf037-*, lmxxf039-* e lmxxf041-* para a RX 9000) e o código de lançamento deles no `LmxxfNrRuntime.dll` — lmxxf 0.37 / 0.39 / 0.41 by Kien (MIT), distribuídos como o lmxxf os compilou; a parte do AMDNR é o carregamento como um único grupo fixado, os pinos SHA-256, os padrões por GPU, as chaves para desligar e os fallbacks
- **kernels c32w** (0.3.3.2) — kernels RDNA 4 de uma wave do próprio AMDNR para a rede do lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); ideias da documentação pública de WMMA do RDNA 4 da AMD (GPUOpen, ROCm matrix instruction calculator)
- **O backend RDNA 3 do AMDNR** (0.3.3; as builds para portáteis gfx1103 / gfx1150 no 0.3.4), a política de níveis de tamanho da rede e os tamanhos de rede pequenos (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, sem modificações)
- **HelixSR** — lonewolf0622 — https://github.com/lonewolf0622/HelixSR (Model E, 0.3.5.6: baixado pelo AMDNR Launcher da release do próprio projeto e usado sob a licença própria dele; não distribuído pelo AMDNR; ele monta a rede no seu PC a partir do arquivo DLSS da NVIDIA, que o launcher baixa do GitHub da própria NVIDIA ou pega do seu jogo)
- **d4r** — countervolts — https://github.com/countervolts/d4r (Model L, 0.3.5.6: o runtime DLSS da NVIDIA sob a EULA da NVIDIA, rodado por meio do **ZLUDA** (vosen, https://github.com/vosen/ZLUDA) e do HIP; baixado pelo AMDNR Launcher da release do próprio d4r, sob a licença própria do d4r; não distribuído pelo AMDNR, não testado pelo autor dele no Windows)
- **OptiPatcher** — das releases do projeto OptiScaler, https://github.com/optiscaler/OptiPatcher (baixado pelo GAME SETTINGS do AMDNR Launcher quando você o pede; não distribuído pelo AMDNR)
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — o host de captura de janela dentro do qual o AMDNR Anywhere roda, numa build própria do AMDNR publicada nas releases do AMDNR no GitHub (`Magpie.exe` recompilado com as mudanças do AMDNR; a oferta de código-fonte, um diff a partir da tag do fork e as opções de build ao lado, em `Anywhere-host-source/`; nenhum arquivo da NVIDIA) — https://github.com/Blinue/Magpie (o fork: https://github.com/SAOG0721/Magpie)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (a matemática da composição de cor, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), a base do desbloqueio multiquadro do XeFG integrado e do seu ritmo
- **Zach Hembree (DarkHelmet)** — o FSR Ray Regeneration para o OptiScaler, a origem do caminho de Ray Regeneration do AMDNR, continuado por **burak113**, de cujo branch o AMDNR fez o port (branch ffx-denoise-experimental do OptiScaler, GPL-3.0)
- **Screen-space GI** (o efeito herdado; retirado do menu no 0.3.4, `[AmdRtgi] Enabled` no ini) — um efeito que o AMDNR herdou da linhagem OptiScaler-AMD-PreSR; o crédito é dos autores originais. Ele precisa da pasta `experimental_lighting` do pacote do danielblnc, que o AMDNR não distribui.
- **AMDNR Screen-space GI by 3zwr1** - trabalho próprio do AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), escrito a partir de pesquisas publicadas (listadas em `CHANGELOG.md` e `Licenses/AMDNR_NOTICE.txt`).
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (novo no 0.3.4) é um programa para Windows 10 / 11 que também pode rodar sob Proton no Linux
(experimental, ainda não testado por nós: veja "Linux / Proton"). Ele instala e atualiza o AMDNR jogo a jogo:
encontra os seus jogos (Steam, Epic, o app Xbox, Ubisoft Connect, o EA app, GOG, Rockstar, Battle.net e Amazon
Games), escolhe o nome da DLL, baixa a build e o runtime do danielblnc que você escolher (e, quando você os escolhe, o
Model E e o Model L das releases dos próprios autores), verifica cada instalação com o Doctor dele, roda o AMDNR Anywhere em jogos sem upscaler próprio (PLAY ANYWHERE) e se atualiza sozinho. Baixe o
`AMDNR-Launcher.exe`, o AMDNR Launcher da release mais recente, na página de releases:
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>

**Novo no Launcher 0.3.5.25** (na release Alpha0.3.5.6; ele se atualiza primeiro, depois os seus jogos):

- **SR model** em GAME SETTINGS de um jogo: Auto / Model E / Model L (veja "Model E e Model L"). A primeira escolha de
  um modelo mostra o aviso dele (ACCEPT / Cancel, com Cancel como padrão); depois o launcher baixa os arquivos dele da
  release do próprio autor, conferidos por hash. O Model L faz par com o runtime lmxxf (ou com o Neural Rendering
  desligado); com o runtime do danielblnc o launcher o recusa e diz por quê. O Auto tira os arquivos do Model L de
  novo.
- Linhas **Game exposure** (Auto / On / Off) e **Anti-halo** em GAME SETTINGS, gravadas só numa build do AMDNR que as
  lê.
- **Configurações do Anywhere** na página ANYWHERE do launcher: runtime de NR, passes, tamanho neural, FSR (Auto /
  FSR 4 / FSR 3 / Off - Off mantém o NR), geração de quadros (experimental), movimento para o upscaling, nitidez e SR
  model; elas valem no próximo PLAY ANYWHERE.
- Visual **Most FPS**: roda a rede um tamanho menor (lmxxf), com um tamanho neural menor; nenhum quadro neural é
  pulado (Refine passes no danielblnc quando mais de um passe roda).
- **Public Chat** no Community Lab: as mensagens são públicas e verificadas por IA, e podem ser traduzidas
  automaticamente para o idioma do seu launcher; a equipe pode remover mensagens.
- Um runtime que você escolhe para um jogo é mantido para esse jogo.

**Novo no Launcher 0.3.5.1** (na release Alpha0.3.5; ele se atualiza primeiro, depois os seus jogos):

- **PLAY ANYWHERE** num jogo sem upscaler próprio (veja "AMDNR Anywhere"): o launcher baixa o host de captura da
  release do AMDNR no GitHub, inicia o jogo e roda o Neural Rendering na janela dele, com um resumo somente leitura das
  configurações do host ao lado do botão (as configurações em si ficam na aba Anywhere do menu). RX 9000 nesta
  release. Jogos em que o mod não consegue carregar (32 bits, DirectX 9 / OpenGL sem upscaler) são recusados no
  INSTALL com uma linha clara e recebem a oferta do Anywhere.
- **UPDATE ALL** e atualização ao iniciar; espelhos de pacotes; RETRY / OPEN DOWNLOAD / IMPORT PACKAGE quando um
  download falha.
- **COLLECT LOGS** liga o tratamento de crash por uma execução e leva junto o `amdnr_crash.log`, o dump mais novo e o
  `dlssnr_on_amd.ini` do danielblnc.
- **PLAY** inicia jogos do Xbox / Microsoft Store pelo id de app deles.
- **Um único PLAY** com seletor de rota (o upscaler próprio do jogo ou o AMDNR Anywhere, lembrado por jogo) e a API
  gráfica (DX9 / 10 / 11 / 12 / Vulkan / OpenGL) na página de cada jogo.
- **Doze idiomas**: turco, coreano e húngaro se juntam aos nove abaixo (treze agora: uzbeque também).
- **Jogos da Rockstar iniciam pela loja deles** (Steam, Epic ou o Rockstar Games Launcher), nunca pelo exe.
- **AMDNR Anywhere**: o jogo roda com prioridade de GPU abaixo do normal para que o host e o Neural Rendering venham
  primeiro; o ícone da bandeja do host de captura fica oculto.
- **Resident Evil 2 / 3 / 4 (2023) / Village**: um aviso de configuração (eles precisam do plugin de upscaler do
  PureDark com o REFramework). Mais jogos sem DLSS, XeSS ou FSR 2+ são marcados como sem suporte, com o motivo,
  antes de qualquer download.
- **FSR 4 (INT8)** como opção experimental que você liga manualmente na página do jogo nos portáteis RDNA 3.
- **Os carregadores de outros mods nunca são tomados nem movidos** (RED4ext, Cyber Engine Tweaks e afins): o launcher
  escolhe outro nome de proxy e o Doctor dele nomeia qualquer carregador que um INSTALL anterior pôs de lado.
- **GTA V Enhanced**: a página do jogo traz a linha de rota (o FSR 3.1 escolhido no jogo é a entrada; o Neural
  Rendering roda antes dele) e um aviso sobre o `settings.xml`.

**Novo no Launcher 0.3.4.1: vocês pediram, a gente fez** (a partir do primeiro feedback no Discord):

- Nove idiomas (doze desde o 0.3.5.1, treze agora): inglês, árabe, chinês (simplificado), francês, espanhol,
  português, italiano, russo e polonês, mais turco, coreano, húngaro e uzbeque. O launcher segue o idioma do seu Windows (ou inglês, se ele não for um
  desses); escolha outro em LANGUAGE (IDIOMA)
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

O `nvngx_dlssnr.dll` da NVIDIA não está nestes arquivos, nem nenhum outro arquivo da NVIDIA. O pacote d4r do Model L
(o runtime DLSS da NVIDIA sob a EULA da NVIDIA) e o arquivo DLSS da NVIDIA a partir do qual o Model E monta a rede são
baixados pelo AMDNR Launcher só quando você os escolhe, da release do próprio d4r e do GitHub da própria NVIDIA (ou do
seu jogo); o AMDNR não os hospeda nem distribui, e o Model E e o Model L são trabalho dos seus autores, não do AMDNR. O
AMDNR não é afiliado à AMD nem à NVIDIA, nem endossado por elas. Nada disto é endossado, afiliado ou
suportado pela NVIDIA, pela AMD ou por qualquer distribuidora de jogos. Ele aciona diretamente um
recurso não documentado. Use por sua conta e risco.
