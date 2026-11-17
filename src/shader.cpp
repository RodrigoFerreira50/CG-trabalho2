// Shaders: lê os ficheiros GLSL, compila o vertex shader e o fragment shader e liga-os num programa
// (como a função LoadShaders dos exemplos das aulas).
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include <GL/glew.h>

#include <common/shader.hpp>

// Lê um ficheiro de shader, compila-o e mostra os erros de compilação (se houver).
static GLuint compileShader(GLenum type, const char* file_path) {
    std::ifstream stream(file_path);
    if (!stream) printf("Não foi possível abrir %s. O programa está a correr na pasta certa?\n", file_path);
    std::stringstream sstr;
    sstr << stream.rdbuf();  // o ficheiro todo para uma string
    std::string code = sstr.str();
    const char* source = code.c_str();

    printf("A compilar o shader : %s\n", file_path);
    GLuint shaderID = glCreateShader(type);
    glShaderSource(shaderID, 1, &source, NULL);
    glCompileShader(shaderID);
    char log[2048] = "";  // mensagens do compilador de GLSL
    glGetShaderInfoLog(shaderID, sizeof log, NULL, log);
    if (*log) printf("%s\n", log);
    return shaderID;
}

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {
    GLuint VertexShaderID = compileShader(GL_VERTEX_SHADER, vertex_file_path);
    GLuint FragmentShaderID = compileShader(GL_FRAGMENT_SHADER, fragment_file_path);

    // ligar os dois shaders num programa
    printf("A ligar o programa\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);
    char log[2048] = "";  // mensagens da ligação
    glGetProgramInfoLog(ProgramID, sizeof log, NULL, log);
    if (*log) printf("%s\n", log);

    // depois de ligados, os shaders já não são precisos (o programa fica com o código)
    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);
    return ProgramID;
}
