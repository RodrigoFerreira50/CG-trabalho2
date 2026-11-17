#ifndef LANDING_HPP
#define LANDING_HPP

#include <glm/glm.hpp>

// Procedimento de aterragem do F-16C na pista 05 (referencial: Y para cima, soleira em z = 0,
// aterragem para -Z, eixo da pista em x = 0): ILS -> flare -> toque -> travagem -> parado.
enum Phase { APPROACH, FLARE, ROLLOUT, STOPPED };

const float KT = 0.5144f, FT = 0.3048f;  // m/s por nó, metros por pé
const float AIM_Z = -291.0f;             // ponto de mira da rampa de 3° (onde ficam as luzes PAPI)

struct Flight {
    glm::vec3 pos;                 // rodas do trem principal (m)
    float vx, groundSpeed, vs;     // velocidade lateral, no solo e vertical (m/s)
    float yaw, pitch, roll, time;  // atitude (graus) e tempo de voo (s)
    int phase;
    float touchdownVs, touchdownZ;
};
extern Flight flight;

void printTOLD();          // dados da aterragem calculados (escritos no terminal)
void restartLanding();     // o avião volta a 9 km da soleira
void printFlight();        // uma linha com os dados do voo no terminal
void updateLanding(float dt);
glm::mat4 aircraftModel(); // matriz de modelo do avião

#endif
