// =====================================================================================================
// Computação Gráfica, trabalho 2 (exercício 2): simulação do procedimento de aterragem
// Aluno: Rodrigo Ferreira, n.º 53584
//
// Um F-16 aterra na pista 05 do aeroporto da Madeira, de dia ou de noite. O programa usa OpenGL 3.3
// core (VAO/VBO e shaders), GLEW, GLFW e GLM. Este ficheiro tem a janela, o teclado e o rato, as
// câmaras, as luzes da pista, a transferência dos dados para a placa gráfica, o desenho de cada frame e
// o ciclo principal a 60 fps.
// =====================================================================================================

// bibliotecas padrão do C++
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>

// GLEW: dá acesso às funções do OpenGL moderno
#include <GL/glew.h>

// GLFW: janela, contexto OpenGL, teclado e rato
#include <GLFW/glfw3.h>
GLFWwindow* window;

// GLM: vetores, matrizes e transformações (translação, rotação, perspetiva, lookAt)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

// ficheiros do projeto
#include <common/shader.hpp>     // LoadShaders: compila e liga os shaders
#include <common/texture.hpp>    // imagens PPM / PGM e texturas
#include <common/objloader.hpp>  // leitor de .obj / .mtl e malhas na placa gráfica
#include <common/terrain.hpp>    // relevo da Madeira e mar
#include <common/landing.hpp>    // dados TOLD e procedimento de aterragem

// programa GLSL feito com os dois shaders (vertex e fragment)
GLuint programID;

// malhas: avião, pista, aeroporto, terreno, mar, luzes da pista e luzes de navegação do avião (pontos)
Mesh aircraft, runway, airport, terrain, sea, runwayLights, navigationLights;
GLuint emptyVAO;  // o céu desenha-se sem vértices (o vertex shader gera-os), mas o OpenGL exige um VAO

// tamanho da imagem em píxeis (num ecrã Retina é o dobro do tamanho da janela)
GLint WindowWidth = 1280;
GLint WindowHeight = 720;

const vec3 SUN(4000, 6000, 3000);  // posição da luz pontual (o sol; de noite é a lua)
const vec3 TOWER_EYE(-322, 44, -560), SPOTTER_EYE(70, 2.5f, -380);  // câmaras fixas: torre e ao lado da pista
const vec3 PILOT_EYE(0, 3.2f, -3.2f);  // olhos do piloto, no referencial do avião
const float AIRCRAFT_LENGTH = 15.06f;  // comprimento real do F-16 (m)

// câmaras: a tecla C troca de câmara (com uma transição suave), o rato roda a vista e W A S D Q E deslocam-na
// órbita, perseguição (atrás do avião), asa, cockpit, torre, ao lado da pista e flyby
enum { ORBIT, CHASE, WING, COCKPIT, TOWER, SPOTTER, FLYBY, CAMERA_COUNT };
int camera = ORBIT;
bool turbo = false, paused = false, night = false;  // acelerar 4x, pausa, modo noite
float camYaw = 25, camPitch = 10, orbitDistance = 40;  // órbita: ângulos (graus) e distância ao avião (m)
vec2 look(0, -9);           // cockpit: para onde o piloto olha (graus para o lado e para cima)
vec2 aims[CAMERA_COUNT];    // câmaras paradas: desvio do olhar em relação ao avião (graus), um por câmara
float fieldOfView = 50;     // campo de visão atual (graus)
vec3 eye, flybyEye;         // posição atual da câmara; onde fica a câmara do flyby
vec3 offsets[CAMERA_COUNT];  // translação feita com o teclado, uma por câmara: cada câmara lembra-se de
                             // onde ficou quando se troca com o C
vec3 viewForward(0.0f, 0.0f, -1.0f);  // para onde a câmara olha (o W anda nesta direção)
vec3 viewUp(0.0f, 1.0f, 0.0f);        // o "cima" da câmara atual
double mouseX, mouseY;                // última posição do rato
float chaseYaw = 0, chaseRoll = 0;    // perseguição: rumo e pranchamento que seguem os do avião com atraso
// transição ao trocar de câmara: de onde partiu (posição, direção, cima, campo de visão) e quanto já andou (0 a 1)
vec3 fromEye, fromDir, fromUp;
float fromFov = 50, transition = 1;
float shake = 0;                      // tremor da câmara no toque das rodas (1 no toque, depois vai a 0)
const float FRAME = 1.0f / 60.0f;     // duração de um frame (s): o programa corre fixo a 60 fps

// matrizes de vista e de projeção da câmara atual
mat4 View = mat4(1.0f);
mat4 Projection = mat4(1.0f);

//--------------------------------------------------------------------------------
// Luzes da pista, desenhadas como pontos: bordas a cada 60 m (amarelas nos últimos 600 m), soleira
// verde, fim vermelho, luzes de aproximação com barra transversal a 300 m e as quatro luzes PAPI ao lado
// do ponto de mira. Nas PAPI a normal guarda o ângulo da luz (o vertex shader escolhe branco ou vermelho).
void buildRunwayLights(void) {
    auto light = [](float x, float z, vec3 c, float papiAngle = 0.0f) {
        runwayLights.vertices.push_back({vec3(x, 0.4f, z), vec3(papiAngle, 0, 0), c, vec2(0.0f)});
    };
    const vec3 white(1.0f, 0.95f, 0.8f), yellow(1.0f, 0.75f, 0.2f);
    for (float z = 0; z >= -3000; z -= 60) {  // bordas da pista
        light(-23, z, z < -2400 ? yellow : white);
        light(23, z, z < -2400 ? yellow : white);
    }
    for (float x = -22; x <= 22; x += 4) {
        light(x, 1, vec3(0.2f, 1.0f, 0.3f));      // soleira (início da pista)
        light(x, -3001, vec3(1.0f, 0.1f, 0.1f));  // fim da pista
    }
    for (float z = 30; z <= 900; z += 30)  // luzes de aproximação, antes da pista
        for (float x = -2; x <= 2; x += 2) light(x, z, white);
    for (float x = -15; x <= 15; x += 1.5f) light(x, 300, white);  // barra transversal a 300 m
    const float papi[] = {3.5f, 3.17f, 2.83f, 2.5f};  // a luz mais perto da pista tem o ângulo maior
    for (int i = 0; i < 4; i++) light(-40.0f - 9.0f * i, AIM_Z, white, papi[i]);
}

//--------------------------------------------------------------------------------
// Prepara tudo antes do primeiro frame: shaders, malhas .obj, terreno, luzes e texturas, e envia os
// vértices para a placa gráfica (VAO / VBO).
void transferDataToGPUMemory(void) {
    // compilar e ligar os shaders, e dizer a cada sampler a unidade de textura que lê
    programID = LoadShaders("PhongVertexShader.vertexshader", "PhongFragmentShader.fragmentshader");
    glUseProgram(programID);
    glUniform1i(glGetUniformLocation(programID, "textureSampler"), 0);
    glUniform1i(glGetUniformLocation(programID, "skybox"), 1);
    glUniform1i(glGetUniformLocation(programID, "satellite"), 2);
    glUniform3fv(glGetUniformLocation(programID, "lightPos"), 1, value_ptr(SUN));

    // malhas .obj: pista e edifícios; o F-16 descarregado não tem trem, por isso junta-se o gear.obj
    loadOBJ("assets/runway.obj", runway);
    loadOBJ("assets/airport.obj", airport);
    loadOBJ("assets/f16/falcon.obj", aircraft);
    loadOBJ("assets/f16/gear.obj", aircraft);
    // F-16: roda 90° à volta de Y (nariz para -Z) e calcula a caixa envolvente (cantos lo e hi)
    vec3 lo(1e9f), hi(-1e9f);
    for (Vertex& v : aircraft.vertices) {
        v.pos = vec3(v.pos.z, v.pos.y, -v.pos.x);
        v.normal = vec3(v.normal.z, v.normal.y, -v.normal.x);
        lo = min(lo, v.pos);
        hi = max(hi, v.pos);
    }
    // origem no centro da base (rodas em y = 0) e escala para o comprimento real
    for (Vertex& v : aircraft.vertices)
        v.pos = (v.pos - vec3((lo.x + hi.x) / 2, lo.y, (lo.z + hi.z) / 2)) * AIRCRAFT_LENGTH / (hi.z - lo.z);
    // luzes de navegação nos vértices extremos da malha: vermelha na ponta da asa esquerda, verde na
    // direita, flash branco no topo da deriva (a normal.y = 1 faz piscar) e o farol no trem do nariz
    Vertex left = aircraft.vertices[0], right = left, top = left;
    for (const Vertex& v : aircraft.vertices) {
        if (v.pos.x < left.pos.x) left = v;
        if (v.pos.x > right.pos.x) right = v;
        if (v.pos.y > top.pos.y) top = v;
    }
    navigationLights.vertices = {{left.pos, vec3(0), vec3(1, 0.1f, 0.1f), vec2(0)},
                                 {right.pos, vec3(0), vec3(0.1f, 1, 0.2f), vec2(0)},
                                 {top.pos, vec3(0, 1, 0), vec3(1), vec2(0)},
                                 {vec3(0, 1.2f, -4.5f), vec3(0), vec3(1), vec2(0)}};
    buildTerrain(terrain, sea);
    buildRunwayLights();
    // enviar todas as malhas para a placa gráfica
    for (Mesh* m : {&aircraft, &runway, &airport, &terrain, &sea, &runwayLights, &navigationLights}) transferMeshToGPU(*m);

    // texturas em unidades fixas: 0 = textura de cada material, 1 = skybox, 2 = imagem de satélite
    bindSatellite(programID);
    GLuint skyboxTexture = loadCubemap("assets/skybox/");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glBindTexture(GL_TEXTURE_2D, loadTexture("assets/textures/asphalt.ppm"));  // o macOS quer uma 2D válida em cada unidade
    glActiveTexture(GL_TEXTURE0);
    glGenVertexArrays(1, &emptyVAO);
}

//--------------------------------------------------------------------------------
// Apaga da placa gráfica as malhas (VAO / VBO), o VAO do céu e o programa GLSL.
void cleanupDataFromGPU() {
    for (Mesh* m : {&aircraft, &runway, &airport, &terrain, &sea, &runwayLights, &navigationLights}) cleanupMesh(*m);
    glDeleteVertexArrays(1, &emptyVAO);
    glDeleteProgram(programID);
}

//--------------------------------------------------------------------------------
// Teclado (teclas carregadas uma vez): C troca de câmara, N dia / noite, Tab acelera 4x, P pausa,
// R recomeça a aterragem, Esc sai.
void keyCallback(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, 1);
    if (key == GLFW_KEY_N) night = !night;
    if (key == GLFW_KEY_TAB) turbo = !turbo;
    if (key == GLFW_KEY_P) paused = !paused;
    if (key == GLFW_KEY_R) restartLanding();
    if (key == GLFW_KEY_C) {  // próxima câmara: guarda a vista atual, de onde parte a transição
        fromEye = eye;
        fromDir = viewForward;
        fromUp = viewUp;
        fromFov = fieldOfView;
        transition = 0;
        camera = (camera + 1) % CAMERA_COUNT;
        if (camera == FLYBY) {  // fica onde o avião vai estar daqui a 6 s, ao lado e acima, e vê-o passar
            flybyEye = flight.pos + vec3(-40, 15, -flight.groundSpeed * 6);
            flybyEye.y = max(flybyEye.y, groundHeight(flybyEye.x, flybyEye.z) + 5);
        }
    }
}

// Rato: arrastar com o botão esquerdo roda a vista (órbita à volta do avião, olhar do piloto, ou olhar
// das câmaras paradas, aqui mais devagar quando há muito zoom).
void cursorCallback(GLFWwindow* w, double x, double y) {
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        float dx = float(x - mouseX) * 0.3f, dy = float(y - mouseY) * 0.3f;  // graus por píxel
        if (camera == ORBIT) {
            camYaw -= dx;
            camPitch = clamp(camPitch + dy, -89.0f, 89.0f);
        } else if (camera == COCKPIT) look = clamp(look - vec2(dx, dy), vec2(-150, -60), vec2(150, 60));
        else aims[camera] = clamp(aims[camera] - vec2(dx, dy) * fieldOfView / 60.0f, vec2(-90, -60), vec2(90, 60));
    }
    mouseX = x;
    mouseY = y;
}

// Roda do rato: aproxima ou afasta a câmara da órbita.
void scrollCallback(GLFWwindow*, double, double y) { orbitDistance = clamp(orbitDistance * pow(0.9f, float(y)), 12.0f, 2000.0f); }

// Teclado (teclas mantidas carregadas): W / S em frente e para trás, A / D para os lados, Q / E descer
// e subir, Shift mais depressa. É a translação em R3 da câmara atual, na direção para onde ela olha.
void moveCamera(float dt) {
    auto key = [](int k) { return float(glfwGetKey(window, k) == GLFW_PRESS); };
    vec3 right = normalize(cross(viewForward, vec3(0, 1, 0)));
    float speed = dt * (key(GLFW_KEY_LEFT_SHIFT) ? 400.0f : 40.0f);  // 40 m/s, ou 400 m/s com Shift
    offsets[camera] += speed * (viewForward * (key(GLFW_KEY_W) - key(GLFW_KEY_S)) + right * (key(GLFW_KEY_D) - key(GLFW_KEY_A)) +
                       vec3(0, key(GLFW_KEY_E) - key(GLFW_KEY_Q), 0));
}

//--------------------------------------------------------------------------------
// Calcula a posição da câmara (eye), o ponto para onde olha (center) e o campo de visão, e daí as
// matrizes de vista (lookAt) e de projeção em perspetiva.
// Ponto para onde olha uma câmara em eye virada na direção d, desviada pelos ângulos do rato (aim, em graus).
vec3 aimAt(const vec3& eye, const vec3& d, vec2 aim) {
    float ay = atan(d.x, d.z) + radians(aim.x), ap = clamp(asin(clamp(d.y, -1.0f, 1.0f)) + radians(aim.y), -1.5f, 1.5f);
    return eye + vec3(sin(ay) * cos(ap), sin(ap), cos(ay) * cos(ap));
}

void setMVP(const mat4& aircraftModel) {
    vec3 target = flight.pos + vec3(0, 3, 0), center = target, up(0, 1, 0);
    mat3 rotation(aircraftModel);  // só a rotação do avião (atitude)
    float fov = 50, y = radians(camYaw), p = radians(camPitch);
    if (camera == ORBIT) {  // numa esfera à volta do avião (coordenadas esféricas), que acompanha o rumo dele
        y += radians(flight.yaw);
        eye = target + orbitDistance * vec3(sin(y) * cos(p), sin(p), cos(y) * cos(p));
        eye.y = max(eye.y, 1.0f);  // nunca abaixo do chão
    } else if (camera == CHASE) {
        // perseguição: atrás e acima do avião. O rumo e o pranchamento da câmara seguem os do avião com
        // atraso (1 - e^(-k dt) em cada frame), por isso ela balança nas curvas como num filme
        chaseYaw += (flight.yaw - chaseYaw) * (1 - exp(-2.0f * FRAME));
        chaseRoll += (flight.roll - chaseRoll) * (1 - exp(-1.5f * FRAME));
        float h = radians(chaseYaw);
        vec3 back(sin(h), 0, cos(h));  // a cauda do avião aponta para aqui (o nariz é -Z rodado pelo rumo)
        eye = target + 30.0f * back + vec3(0, 7, 0);
        eye.y = max(eye.y, 1.5f);
        center = aimAt(eye, normalize(target - 25.0f * back - eye), aims[camera]);  // olha um pouco à frente do avião
        up = vec3(rotate(mat4(1.0f), radians(0.4f * chaseRoll), back) * vec4(0, 1, 0, 0));  // inclina com o avião
        fov = 55;
    } else if (camera == WING) {  // presa ao avião, atrás e por fora da asa esquerda, a olhar para o nariz e a rodar com ele
        eye = vec3(aircraftModel * vec4(-9.0f, 2.5f, 10.0f, 1));
        vec3 nose = vec3(aircraftModel * vec4(0.0f, 1.0f, -4.0f, 1));
        center = aimAt(eye, normalize(nose - eye), aims[camera]);
        up = rotation * vec3(0, 1, 0);
        fov = 60;
    } else if (camera == COCKPIT) {  // o olho do piloto, a rodar com o avião
        vec2 l = radians(look);
        eye = vec3(aircraftModel * vec4(PILOT_EYE, 1));
        center = eye + rotation * vec3(-sin(l.x) * cos(l.y), sin(l.y), -cos(l.x) * cos(l.y));
        up = rotation * vec3(0, 1, 0);
        fov = 65;
    } else {  // câmaras paradas (deslocadas pelo teclado), com zoom para o avião ficar sempre do mesmo tamanho
        eye = (camera == TOWER ? TOWER_EYE : camera == SPOTTER ? SPOTTER_EYE : flybyEye) + offsets[camera];
        fov = clamp(degrees(2 * atan(18 / length(target - eye))), 1.0f, 60.0f);
        center = aimAt(eye, normalize(target - eye), aims[camera]);  // olham para o avião, desviadas pelo rato
    }
    if (camera <= COCKPIT) {  // câmaras que seguem o avião: o teclado move a câmara e o ponto para onde olha
        eye += offsets[camera];
        center += offsets[camera];
    }
    if (shake > 0.001f) {  // tremor no toque das rodas, que se desvanece em cerca de 1 s
        float t = float(glfwGetTime());
        vec3 jolt = shake * vec3(0.12f * sin(t * 53), 0.25f * sin(t * 41), 0.12f * sin(t * 67));
        eye += jolt;
        center += 0.3f * jolt;
        shake *= exp(-2.5f * FRAME);
    }

    // transição suave depois de trocar de câmara (1,5 s): a posição, a direção do olhar, o "cima" e o campo de
    // visão passam da câmara anterior para a nova, mais devagar no início e no fim (smoothstep)
    vec3 dir = normalize(center - eye);
    if (transition < 1) {
        transition = min(transition + FRAME / 1.5f, 1.0f);
        float s = smoothstep(0.0f, 1.0f, transition);
        vec3 d = mix(fromDir, dir, s);
        if (length(d) > 0.01f) dir = normalize(d);
        eye = mix(fromEye, eye, s);
        up = normalize(mix(fromUp, up, s));
        fov = mix(fromFov, fov, s);
    }
    viewForward = dir;
    viewUp = up;
    fieldOfView = fov;
    View = lookAt(eye, eye + dir, up);
    // perto do avião (cockpit, asa) o plano de corte perto é menor; longe vai até 150 km (o mapa tem 80 km)
    Projection = perspective(radians(fov), float(WindowWidth) / max(WindowHeight, 1), length(eye - target) < 20 ? 0.25f : 1.0f, 150000.0f);
}

//--------------------------------------------------------------------------------
// Desenha um frame: céu, terreno, mar, pista, edifícios, luzes e avião.
void draw(void) {
    mat4 model = aircraftModel();  // matriz de modelo do avião (posição e atitude do procedimento)
    setMVP(model);
    glViewport(0, 0, WindowWidth, WindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // uniforms comuns a todo o frame: câmara, farol de aterragem (no trem do nariz), noite e tempo
    glUseProgram(programID);
    mat4 invSky = inverse(Projection * mat4(mat3(View)));  // píxel do ecrã -> direção (para o céu)
    vec3 spotPos = vec3(model * vec4(0, 1.2f, -4.5f, 1)), spotDir = normalize(mat3(model) * vec3(0, -0.08f, -1));
    glUniformMatrix4fv(glGetUniformLocation(programID, "View"), 1, GL_FALSE, value_ptr(View));
    glUniformMatrix4fv(glGetUniformLocation(programID, "Projection"), 1, GL_FALSE, value_ptr(Projection));
    glUniformMatrix4fv(glGetUniformLocation(programID, "invSky"), 1, GL_FALSE, value_ptr(invSky));
    glUniform3fv(glGetUniformLocation(programID, "eyePos"), 1, value_ptr(eye));
    glUniform3fv(glGetUniformLocation(programID, "spotPos"), 1, value_ptr(spotPos));
    glUniform3fv(glGetUniformLocation(programID, "spotDir"), 1, value_ptr(spotDir));
    glUniform1i(glGetUniformLocation(programID, "night"), night);
    glUniform1f(glGetUniformLocation(programID, "time"), float(glfwGetTime()));

    // céu (um triângulo que cobre o ecrã, sem escrever a profundidade), terreno e mar
    glDepthMask(GL_FALSE);
    glUniform1i(glGetUniformLocation(programID, "mode"), 3);
    glBindVertexArray(emptyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);
    drawMesh(programID, terrain, mat4(1.0f), 1);
    drawMesh(programID, sea, mat4(1.0f), 2);

    // pista e marcas: camadas no mesmo plano do terreno, pintadas pela ordem do ficheiro e puxadas para
    // a frente (polygon offset); só depois se escreve a profundidade. O aeroporto fica à esquerda (espelho).
    const mat4 mirror = scale(mat4(1.0f), vec3(-1, 1, 1));
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1, -4);
    glDepthMask(GL_FALSE);
    drawMesh(programID, runway, mirror, 0);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    drawMesh(programID, runway, mirror, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // edifícios, luzes da pista e avião (com a câmara dentro do cockpit não se desenha o avião, para não tapar a vista)
    drawMesh(programID, airport, mirror, 0);
    drawMesh(programID, runwayLights, mat4(1.0f), 4, GL_POINTS);
    bool insideCockpit = length(eye - vec3(model * vec4(PILOT_EYE, 1))) < 3.0f;
    if (!insideCockpit) drawMesh(programID, aircraft, model, 0);
    if (night && !insideCockpit) drawMesh(programID, navigationLights, model, 4, GL_POINTS);
}

//--------------------------------------------------------------------------------
int main(int argc, char** argv) {
    // se o programa não correr na pasta do projeto (por exemplo a partir de bin/, build/ ou build/Release/),
    // sobe a partir da pasta do executável até encontrar a pasta com os shaders e os assets
    for (auto dir = std::filesystem::absolute(argv[0]).parent_path(); !std::filesystem::exists("assets") && dir.has_relative_path();
         dir = dir.parent_path())
        if (std::filesystem::exists(dir / "assets")) std::filesystem::current_path(dir);
    night = argc > 1 && strcmp(argv[1], "noite") == 0;  // ./bin/aterragem noite começa de noite

    // iniciar o GLFW e pedir um contexto OpenGL 3.3 core com antialiasing (4 amostras por píxel)
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // necessário no macOS
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // abrir a janela (com o nome do aluno e o exercício no título) e criar o seu contexto OpenGL
    window =glfwCreateWindow(WindowWidth, WindowHeight, "Rodrigo Ferreira 53584 - Exercício 2 (aterragem)", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // sincronizar com o ecrã (sem cortes na imagem)

    // iniciar o GLEW
    glewExperimental = true;  // necessário no perfil core
    glewInit();

    // funções chamadas pelo GLFW quando se carrega numa tecla, se mexe o rato ou se roda a roda do rato
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetScrollCallback(window, scrollCallback);

    printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
    printf("GL Version   : %s\n", glGetString(GL_VERSION));

    // teste de profundidade, antialiasing e tamanho dos pontos dado pelo vertex shader
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // enviar os dados (vértices, texturas e shaders) para a placa gráfica e começar a aterragem
    transferDataToGPUMemory();
    printTOLD();
    restartLanding();

    // ciclo principal, fixo a 60 frames por segundo: cada frame avança a simulação exatamente 1/60 s e
    // depois espera (a tratar o teclado e o rato) até à hora do frame seguinte
    double nextFrame = glfwGetTime();
    int frames = 0;
    do {
        int phase = flight.phase;
        updateLanding(paused ? 0.0f : FRAME * (turbo ? 4 : 1));
        if (phase == FLARE && flight.phase == ROLLOUT) shake = 1;  // as rodas tocaram na pista: a câmara treme
        if (++frames % 60 == 0) {  // uma vez por segundo: os dados do voo no terminal (não em pausa nem parado)
            if (!paused && flight.phase != STOPPED) printFlight();
            fflush(stdout);  // mostrar já (no Windows a saída pode ficar guardada)
        }
        moveCamera(FRAME);  // a câmara mexe-se mesmo em pausa
        glfwGetFramebufferSize(window, &WindowWidth, &WindowHeight);
        draw();

        // mostrar a imagem desenhada
        glfwSwapBuffers(window);

        // esperar pelo frame seguinte, tratando os eventos (teclado, rato) entretanto
        nextFrame += FRAME;
        if (glfwGetTime() > nextFrame + 0.1) nextFrame = glfwGetTime();  // muito atrasado: não tenta recuperar
        for (double now = glfwGetTime(); now < nextFrame; now = glfwGetTime()) glfwWaitEventsTimeout(nextFrame - now);
        glfwPollEvents();
    } while (glfwWindowShouldClose(window) == 0);

    // apagar os VAO, os VBO e os shaders da placa gráfica
    cleanupDataFromGPU();

    // fechar a janela e terminar o GLFW
    glfwTerminate();
    return 0;
}
