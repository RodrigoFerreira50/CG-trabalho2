#ifndef TERRAIN_HPP
#define TERRAIN_HPP

#include <common/objloader.hpp>

// Relevo real da Madeira à volta do aeroporto (elevação SRTM em PGM de 16 bits e imagem de satélite),
// rodado para o eixo -Z do mundo apontar no rumo da pista 05. A pista fica plana em y = 0.
const float SEA_LEVEL = -46.3f;  // o aeroporto está a 46 m acima do mar

float groundHeight(float x, float z);       // altura do chão (m)
void buildTerrain(Mesh& terrain, Mesh& sea);  // malhas do terreno e do mar
void bindSatellite(GLuint programID);         // imagem de satélite na unidade 2 e a sua georreferência

#endif
