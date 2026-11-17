// Leitor de malhas Wavefront .obj (com os materiais do .mtl) e funções para as enviar para a placa
// gráfica (VAO / VBO) e desenhar.
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
using namespace glm;

#include <common/objloader.hpp>
#include <common/texture.hpp>

// Um material do .mtl: cor difusa, brilho especular e textura.
struct Material {
    vec3 kd{0.7f};
    vec2 specular{0.3f, 32.0f};  // (Ks, Ns)
    std::string texture;
};

// Materiais do .mtl: newmtl, Kd, Ks, Ns e map_Kd. O map_Kd pode ser um caminho de outro computador
// (por exemplo do Windows): nesse caso procura-se só o nome do ficheiro ao lado do .mtl.
static void loadMTL(const std::string& path, std::map<std::string, Material>& materials) {
    std::ifstream file(path);
    std::string line, key, current, dir = path.substr(0, path.find_last_of('/') + 1);
    while (getline(file, line)) {
        std::istringstream s(line);
        key.clear();
        s >> key;
        if (key == "newmtl") s >> current;
        else if (key == "Kd") s >> materials[current].kd.r >> materials[current].kd.g >> materials[current].kd.b;
        else if (key == "Ks") s >> materials[current].specular.x;
        else if (key == "Ns") s >> materials[current].specular.y;
        else if (key == "map_Kd") {
            std::string name;
            s >> name;
            materials[current].texture = std::ifstream(dir + name) ? dir + name : dir + name.substr(name.find_last_of("/\\") + 1);
        }
    }
}

bool loadOBJ(const std::string& path, Mesh& mesh) {
    std::ifstream file(path);
    if (!file) {
        printf("Não foi possível abrir %s\n", path.c_str());
        return false;
    }
    std::string line, key, dir = path.substr(0, path.find_last_of('/') + 1);
    std::vector<vec3> positions, normals;  // listas lidas do ficheiro (v, vn, vt)
    std::vector<vec2> uvs;
    std::map<std::string, Material> materials;
    Material material;
    mesh.parts.push_back({GLint(mesh.vertices.size()), 0, 0, material.specular});  // parte sem material

    // o ficheiro lê-se linha a linha; a primeira palavra diz o que a linha tem
    while (getline(file, line)) {
        std::istringstream s(line);
        key.clear();
        s >> key;
        if (key == "v") { vec3 p; s >> p.x >> p.y >> p.z; positions.push_back(p); }
        else if (key == "vn") { vec3 n; s >> n.x >> n.y >> n.z; normals.push_back(normalize(n)); }
        else if (key == "vt") { vec2 t; s >> t.x >> t.y; uvs.push_back(t); }
        else if (key == "mtllib") { s >> key; loadMTL(dir + key, materials); }
        else if (key == "usemtl") {  // as faces seguintes usam este material
            s >> key;
            material = materials[key];
            mesh.parts.push_back({GLint(mesh.vertices.size()), 0, material.texture.empty() ? 0 : loadTexture(material.texture), material.specular});
        } else if (key == "f") {
            // cada canto é "v", "v/vt", "v//vn" ou "v/vt/vn"; os índices começam em 1 (ou são negativos)
            std::vector<ivec3> corners;
            for (std::string t; s >> t;) {
                ivec3 i(0);
                if (t.find("//") != std::string::npos) sscanf(t.c_str(), "%d//%d", &i.x, &i.z);
                else sscanf(t.c_str(), "%d/%d/%d", &i.x, &i.y, &i.z);
                corners.push_back(ivec3(i.x < 0 ? int(positions.size()) + i.x : i.x - 1,
                                        i.y < 0 ? int(uvs.size()) + i.y : i.y - 1,
                                        i.z < 0 ? int(normals.size()) + i.z : i.z - 1));
            }
            for (size_t k = 1; k + 1 < corners.size(); k++) {  // polígono -> triângulos em leque
                ivec3 tri[3] = {corners[0], corners[k], corners[k + 1]};
                vec3 a = positions[tri[0].x];
                // normal da face (produto vetorial de dois lados), para os cantos que não trazem normal
                vec3 faceNormal = normalize(cross(positions[tri[1].x] - a, positions[tri[2].x] - a));
                for (ivec3 i : tri)
                    mesh.vertices.push_back({positions[i.x], i.z >= 0 ? normals[i.z] : faceNormal, material.kd,
                                             i.y >= 0 ? uvs[i.y] : vec2(0.0f)});
                mesh.parts.back().count += 3;
            }
        }
    }
    return true;
}

// Cria o VAO e o VBO, copia os vértices e diz ao OpenGL onde está cada atributo dentro de um Vertex.
void transferMeshToGPU(Mesh& mesh) {
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);
    const int size[] = {3, 3, 3, 2};  // posição, normal, cor, uv
    for (int a = 0; a < 4; a++) {
        glEnableVertexAttribArray(a);
        glVertexAttribPointer(a, size[a], GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(a * 3 * sizeof(float)));
    }
    if (mesh.parts.empty()) mesh.parts.push_back({0, GLsizei(mesh.vertices.size()), 0, vec2(0.05f, 8.0f)});
}

// Envia a matriz de modelo e o modo do shader e desenha a malha parte a parte, cada uma com a sua
// textura e o seu brilho especular.
void drawMesh(GLuint programID, const Mesh& mesh, const mat4& model, int mode, GLenum primitive) {
    glUniformMatrix4fv(glGetUniformLocation(programID, "Model"), 1, GL_FALSE, value_ptr(model));
    glUniform1i(glGetUniformLocation(programID, "mode"), mode);
    glBindVertexArray(mesh.vao);
    for (const Part& p : mesh.parts) {
        if (p.texture) glBindTexture(GL_TEXTURE_2D, p.texture);  // sem textura: fica a anterior (o shader não a lê)
        glUniform1i(glGetUniformLocation(programID, "useTexture"), p.texture != 0);
        glUniform2fv(glGetUniformLocation(programID, "specular"), 1, value_ptr(p.specular));
        glDrawArrays(primitive, p.first, p.count);
    }
}

// Apaga o VBO e o VAO da placa gráfica.
void cleanupMesh(Mesh& mesh) {
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteVertexArrays(1, &mesh.vao);
}
