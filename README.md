# Simulador de Aterragem: Computação Gráfica, Trabalho 2

**Aluno:** Rodrigo Ferreira · **N.º** 53584 · **PL1**

**Exercício 2 (número de aluno par): simulação do procedimento de aterragem.** Um F-16 aterra na pista 05
do aeroporto da Madeira (LPMA), de dia ou de noite. O programa está em C++17 com OpenGL 3.3 core
(VAO/VBO e shaders), GLEW, GLFW e GLM. A organização segue a dos exemplos das aulas: `src/`, `common/`,
shaders `.vertexshader`/`.fragmentshader`, e um Makefile que compila para `obj/` e `bin/`.

## Executar (Windows, terminal MSYS2 UCRT64)

Abrir o terminal **MSYS2 UCRT64**: no menu Iniciar procurar "MSYS2 UCRT64", ou executar `C:\msys64\ucrt64.exe`.

Ir para a pasta do projeto (os discos do Windows ficam em `/c/`, `/d/`, …) e correr:

```bash
cd /c/Users/<utilizador>/Desktop/53584-Rodrigo-Ferreira-PL1
make run
```

Instalar as dependências (só uma vez):

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glew mingw-w64-ucrt-x86_64-glm make
```

macOS: `brew install glfw glew glm` · Linux: `sudo apt install libglfw3-dev libglew-dev libglm-dev` · depois `make run`.

Com CMake (em qualquer sistema; as bibliotecas que faltarem são descarregadas):

```bash
cmake -S . -B build
cmake --build build
./build/aterragem
```

## Makefile

| Comando | Efeito |
| --- | --- |
| `make` | Compila |
| `make run` | Compila e executa |
| `make noite` | Compila e executa já de noite |
| `make clean` | Apaga o que foi compilado (`bin/`, `obj/`, `build/`) |

O executável encontra sozinho a pasta do projeto (shaders e `assets/`), por isso também pode ser lançado de
outra pasta, por exemplo `./bin/aterragem` ou `./build/aterragem`.

## Teclas e rato

| Comando | Ação |
|---|---|
| `C` | trocar de câmara, com uma transição suave de 1,5 s: órbita → perseguição → asa → cockpit → torre → pista → flyby → órbita (cada câmara lembra-se de onde a deixaste) |
| arrastar com o botão esquerdo | órbita: rodar à volta do avião; cockpit: olhar à volta; perseguição, asa, torre, pista e flyby: desviar o olhar |
| roda do rato | aproximar ou afastar a órbita |
| `W A S D`, `Q E`, `Shift` | deslocar a câmara atual (em frente / trás, lados, descer / subir), mais depressa |
| `N` | dia / noite |
| `Tab` / `P` / `R` / `Esc` | acelerar 4× / pausa / recomeçar / sair |

O programa corre fixo a **60 fps**: cada frame avança a simulação exatamente 1/60 s.

O título da janela é "Rodrigo Ferreira 53584 - Exercício 2 (aterragem)".

## Dados do voo no terminal

Ao arrancar, o terminal mostra os dados TOLD calculados (VREF, vento, VAPP, deriva). Depois, **uma vez por
segundo**, aparece uma linha com os instrumentos do avião, e cada mudança de fase tem uma mensagem `>>>`:

```
  tempo  fase         soleira   eixo  altitude  vel. ar  vel. solo   razão desc.  picada  rolam.  rumo
    60 s  aproximação    4570 m    14 m    836 ft   154 kt    144 kt     -762 ft/min   10.0°    1.1°   2.6°
>>> Flare a 20 ft, 179 m depois da soleira: potência a zero, nariz para cima, tirar a deriva
>>> Toque 420 m depois da soleira, a -87 ft/min e 148 kt: baixar o nariz e travar
>>> Parado 1653 m depois da soleira (1233 m depois do toque). R recomeça.
```

| Coluna | Significado |
|---|---|
| soleira | distância ao início da pista (negativa depois de a passar) |
| eixo | desvio lateral em relação ao eixo da pista |
| altitude | altura das rodas acima da pista |
| vel. ar / vel. solo | velocidade em relação ao ar (a do instrumento) e ao chão; a diferença é o vento de frente |
| razão desc. | velocidade vertical (negativa a descer) |
| picada, rolam., rumo | atitude do avião (o rumo inclui a deriva contra o vento lateral) |

Em pausa (`P`) e depois de parar não aparecem linhas novas; com `R` a tabela recomeça.

## O procedimento de aterragem (TOLD)

As velocidades vêm dos dados de aterragem do F-16C: VREF 141 kt. O vento 020/12G20 kt é decomposto na
pista 044 (11 kt de frente e 5 kt da esquerda), e daí VAPP = VREF + ½ vento de frente + rajada = 154 kt,
com 1,8° de deriva. O programa escreve estes valores no terminal ao arrancar.

1. **Aproximação ILS**: o avião começa a 9 km, 300 m fora do eixo. O localizer leva-o ao eixo com um
   pranchamento proporcional à aceleração lateral, e a rampa de 3° passa a soleira a 50 ft. O nariz vai
   10° acima do horizonte e virado contra o vento.
2. **Flare** a 20 ft: a potência vai a zero, o nariz sobe para 13°, a razão de descida diminui e a deriva
   desaparece.
3. **Toque e travagem**: o nariz baixa a 3°/s e depois os travões desaceleram a 2,5 m/s² até parar.

## Modo noite

- **Céu**: estrelas e lua; a luz pontual passa a ser o luar, fraco e azulado, e a neblina escurece.
- **Farol de aterragem**: segunda luz pontual, em cone, no trem do nariz, com Phong difuso e especular,
  que ilumina a pista à frente do avião.
- **Luzes do avião**: vermelha na ponta da asa esquerda, verde na direita e flash branco na deriva,
  colocadas nos vértices extremos da malha.
- **Luzes da pista**: bordas, soleira verde, fim vermelho e luzes de aproximação com barra a 300 m.
  O **PAPI** fica ao lado do ponto de mira e cada luz mostra branco ou vermelho conforme o ângulo da
  câmara (calculado no vertex shader).
- **Luzes das casas**: aparecem nas zonas onde a imagem de satélite mostra telhados.

## Requisitos e extras

- **Obrigatório**: o programa lê malhas `.obj`/`.mtl` (avião e pista) com o leitor `src/objloader.cpp`. O
  avião roda e desloca-se com matrizes de modelo. O teclado troca de câmara (C) e desloca-a (W A S D Q E); o rato roda-a e aproxima-a. A cena tem
  uma luz pontual com Phong shading por fragmento.
- **Extras**: modo noite; relevo real da Madeira com imagem de satélite como textura; F-16 com textura e
  trem; skybox; mar com ondas e reflexo de Fresnel; neblina; edifícios, marcas e luzes da pista com PAPI;
  sete câmaras à escolha com transições suaves (perseguição com atraso, câmara na asa, tremor no toque); aterragem calculada a partir dos dados TOLD.

## Ficheiros

```
Makefile                             src/*.cpp -> obj/*.o -> bin/aterragem (Windows MSYS2, macOS e Linux)
CMakeLists.txt                       alternativa com CMake (descarrega as bibliotecas em falta)
PhongVertexShader.vertexshader       vértices: mundo, ecrã, céu sem vértices, tamanho das luzes, PAPI
PhongFragmentShader.fragmentshader   Phong (sol / lua + farol), satélite, mar, céu, estrelas, neblina
common/shader.hpp   src/shader.cpp      LoadShaders (como nas aulas)
common/texture.hpp  src/texture.cpp     imagens PPM / PGM, texturas 2D e cube map
common/objloader.hpp src/objloader.cpp  leitor .obj / .mtl, VAO / VBO, desenho
common/terrain.hpp  src/terrain.cpp     relevo da Madeira, mar, georreferência do satélite
common/landing.hpp  src/landing.cpp     dados TOLD e procedimento de aterragem
src/main.cpp                            janela, teclado / rato, câmaras, luzes, transferDataToGPUMemory, draw
assets/                                 runway.obj, airport.obj, f16/, skybox/, textures/, terrain_madeira/
```

## Créditos (objetos obtidos online)

- **F-16**: malha e textura obtidas online no repositório
  [gue-ni/OpenGL_Flightsim](https://github.com/gue-ni/OpenGL_Flightsim) (licença MIT, Jakob Maier,
  `assets/LICENSE-OpenGL_Flightsim.txt`): `assets/models/falcon.obj` e `falcon.mtl`,
  `assets/textures/f16_large.jpg` e `colorpalette.png`. O skybox (`assets/textures/skybox/1/`) vem do mesmo
  repositório. As imagens foram convertidas de JPG/PNG para PPM, o formato que o programa lê.
  Este repositório é o indicado no próprio enunciado (Figura 1 e referência [7]).
- **Ilha da Madeira**: obtida online a partir de dados abertos.
  - Relevo (`heightmap.pgm`): [Terrain Tiles](https://registry.opendata.aws/terrain-tiles/) (Mapzen /
    AWS Open Data, a partir de SRTM e ETOPO1).
  - Imagem de satélite (`satellite.ppm`): [Sentinel-2 cloudless](https://s2maps.eu) by EOX IT Services
    GmbH (contém dados Copernicus Sentinel 2020), licença CC BY-NC-SA 4.0.
- **Feitos por mim**: a pista (`runway.obj`), os edifícios do aeroporto (`airport.obj`) e o trem de
  aterragem (`gear.obj`), gerados em metros com as medidas reais do aeroporto da Madeira.
