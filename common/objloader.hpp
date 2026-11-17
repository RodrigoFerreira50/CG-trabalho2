#ifndef OBJLOADER_HPP
#define OBJLOADER_HPP

#include <string>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

// Um vértice, tal como vai para o VBO: posição (0), normal (1), cor difusa Kd (2), coordenadas de textura (3).
struct Vertex {
    glm::vec3 pos, normal, color;
    glm::vec2 uv;
};
// Triângulos seguidos com o mesmo material: textura (map_Kd) e brilho especular (Ks, Ns).
struct Part {
    GLint first;
    GLsizei count;
    GLuint texture;
    glm::vec2 specular;
};
// Lista de triângulos (sem índices) na memória e na placa gráfica (VAO/VBO).
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Part> parts;
    GLuint vao = 0, vbo = 0;
};

// Lê um .obj (v, vt, vn, f com polígonos divididos em leque, mtllib/usemtl) e junta-o à malha.
bool loadOBJ(const std::string& path, Mesh& mesh);

void transferMeshToGPU(Mesh& mesh);
// Desenha a malha com a sua matriz de modelo; mode escolhe o caminho do shader (objeto, terreno, mar...).
void drawMesh(GLuint programID, const Mesh& mesh, const glm::mat4& model, int mode, GLenum primitive = GL_TRIANGLES);
void cleanupMesh(Mesh& mesh);

#endif
