// Terreno real da Madeira à volta do aeroporto: elevação SRTM (PGM de 16 bits) e imagem de satélite
// Sentinel-2, rodadas para o eixo -Z do mundo apontar no rumo da pista. A pista fica plana em y = 0.
#include <cmath>

#include <glm/glm.hpp>
using namespace glm;

#include <common/terrain.hpp>
#include <common/texture.hpp>

const float MAP_ROTATION = radians(44.1f);  // rumo verdadeiro da pista 05
const float DEM_M_PER_PX = 32.1653f, SAT_M_PER_PX = 16.0826f;  // metros por píxel da elevação e do satélite
const vec2 DEM_THRESHOLD_PX(1799.13f, 1300.43f), SAT_THRESHOLD_PX(3086.265f, 1576.855f);  // píxel da soleira

static std::vector<float> heights;  // metros, já com o aeroporto em y = 0
static int demWidth = 0, demHeight = 0;

// Altura do chão num ponto (x, z) do mundo: o relevo real, aplanado à volta do aeroporto.
float groundHeight(float x, float z) {
    if (heights.empty()) return 0.0f;
    // mundo -> píxel da imagem (colunas para leste, linhas para sul), com leitura bilinear
    float c = cos(MAP_ROTATION), s = sin(MAP_ROTATION);
    vec2 p = clamp(DEM_THRESHOLD_PX + vec2(x * c - z * s, x * s + z * c) / DEM_M_PER_PX, vec2(0.0f),
                   vec2(demWidth - 1.01f, demHeight - 1.01f));
    int i = int(p.x), j = int(p.y);
    vec2 f = fract(p);
    const float* q = &heights[j * demWidth + i];
    float h = mix(mix(q[0], q[1], f.x), mix(q[demWidth], q[demWidth + 1], f.x), f.y);
    // a zona da pista e do aeroporto fica plana, e o relevo volta ao longo de 1,3 km
    float dx = max(max(-560.0f - x, x - 110.0f), 0.0f), dz = max(max(-3150.0f - z, z - 150.0f), 0.0f);
    return h * smoothstep(300.0f, 1600.0f, sqrt(dx * dx + dz * dz));
}

// Lê a elevação e constrói as malhas do terreno (grelha de 513 x 513 vértices, normais pelas diferenças
// das alturas à volta) e do mar.
void buildTerrain(Mesh& terrain, Mesh& sea) {
    Image dem = readPNM("assets/terrain_madeira/heightmap.pgm");
    demWidth = dem.width;
    demHeight = dem.height;
    for (size_t i = 0; i + 1 < dem.pixels.size(); i += 2) {  // metros = valor / 10 - 1000
        float m = (dem.pixels[i] << 8 | dem.pixels[i + 1]) * 0.1f - 1000.0f;
        heights.push_back((m <= 0.5f ? min(m, -15.0f) : m) + SEA_LEVEL);  // o mar desce 15 m: costa nítida
    }

    // grelha graduada (sinh) de 80 km: células de ~40 m na aproximação, ~500 m nas bordas
    const int n = 512;
    std::vector<float> xs(n + 1), zs(n + 1);
    for (int k = 0; k <= n; k++) {
        float u = 2.0f * k / n - 1.0f, s = sinh(3.0f * u) / sinh(3.0f);
        xs[k] = 40000.0f * s;
        zs[k] = 3000.0f + s * (u < 0 ? 43000.0f : 37000.0f);
    }
    std::vector<Vertex> grid;
    for (int j = 0; j <= n; j++)
        for (int i = 0; i <= n; i++) {
            float x = xs[i], z = zs[j], e = 0.25f * (xs[min(i + 1, n)] - xs[max(i - 1, 0)]);
            vec3 normal = normalize(vec3(groundHeight(x - e, z) - groundHeight(x + e, z), 2.0f * e,
                                         groundHeight(x, z - e) - groundHeight(x, z + e)));
            grid.push_back({vec3(x, groundHeight(x, z), z), normal, vec3(0.3f, 0.45f, 0.2f), vec2(0.0f)});
        }
    // cada célula da grelha são dois triângulos
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++)
            for (int k : {0, n + 1, 1, 1, n + 1, n + 2}) terrain.vertices.push_back(grid[j * (n + 1) + i + k]);

    // o mar: um só quadrado; onde fica por baixo da terra, o terreno esconde-o
    for (vec2 c : {vec2(-1, -1), vec2(1, 1), vec2(1, -1), vec2(-1, -1), vec2(-1, 1), vec2(1, 1)})
        sea.vertices.push_back({vec3(c.x * 40000.0f, SEA_LEVEL, 3000.0f + c.y * 40000.0f), vec3(0, 1, 0),
                                vec3(0.1f, 0.22f, 0.3f), vec2(0.0f)});
    sea.parts.push_back({0, 6, 0, vec2(1.0f, 250.0f)});
}

// Liga a imagem de satélite à unidade 2 e envia ao shader a transformação mundo -> coordenadas da imagem.
void bindSatellite(GLuint programID) {
    GLuint texture = loadTexture("assets/terrain_madeira/satellite.ppm");  // criada na unidade 0
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture);
    GLint w, h;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    glActiveTexture(GL_TEXTURE0);
    // transformação afim mundo (x, z, 1) -> coordenadas (u, v) da imagem
    float c = cos(MAP_ROTATION) / SAT_M_PER_PX, s = sin(MAP_ROTATION) / SAT_M_PER_PX;
    glUniform3f(glGetUniformLocation(programID, "satU"), c / w, -s / w, SAT_THRESHOLD_PX.x / w);
    glUniform3f(glGetUniformLocation(programID, "satV"), s / h, c / h, SAT_THRESHOLD_PX.y / h);
}
