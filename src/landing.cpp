// Procedimento de aterragem do F-16C, calculado a partir dos dados TOLD: aproximação ILS (localizer e
// rampa de 3°), flare, toque, travagem e paragem.
#include <cmath>
#include <cstdio>

#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/landing.hpp>

// --- TOLD (dados de descolagem e aterragem) do F-16C e do aeroporto da Madeira
const float G = 9.81f;  // gravidade (m/s²)
// VREF (velocidade de referência), rampa de 3°, altura do flare (20 ft) e desaceleração dos travões
const float VREF = 141 * KT, GLIDE = radians(3.0f), FLARE_HEIGHT = 20 * FT, BRAKING = 2.5f;
const float WIND = 12 * KT, GUST = 20 * KT, WIND_ANGLE = radians(20.0f - 44.1f);  // vento 020/12G20, pista 044
const float HEADWIND = WIND * cos(WIND_ANGLE), CROSSWIND = WIND * sin(WIND_ANGLE);  // < 0: vem da esquerda
const float VAPP = VREF + clamp(0.5f * HEADWIND + GUST - WIND, 5 * KT, 20 * KT);  // VREF + ½ vento + rajada
const float CRAB = -asin(CROSSWIND / VAPP);  // o nariz vira-se contra o vento
const float START_Z = 9000.0f, MAIN_GEAR_Z = 2.0f;  // início da aproximação; trem principal atrás da origem

Flight flight;  // o estado do avião

// Escreve no terminal os dados da aterragem calculados.
void printTOLD() {
    printf("TOLD F-16C: VREF %.0f kt, vento de frente %.0f kt, lateral %.0f kt -> VAPP %.0f kt, deriva %.1f graus\n",
           VREF / KT, HEADWIND / KT, CROSSWIND / KT, VAPP / KT, degrees(CRAB));
}

void restartLanding() {  // 9 km antes da soleira, 300 m fora do eixo, na rampa de 3° à VAPP
    flight = {vec3(300.0f, (START_Z - AIM_Z) * tan(GLIDE), START_Z), 0.0f, VAPP - HEADWIND, 0.0f,
              degrees(CRAB), 10.0f, 0.0f, 0.0f, APPROACH, 0.0f, 0.0f};
    printf("\n>>> Aproximação ILS à pista 05: 9 km da soleira, 300 m fora do eixo, rampa de 3 graus\n");
    printf("  tempo  fase         soleira   eixo  altitude  vel. ar  vel. solo   razão desc.  picada  rolam.  rumo\n");
}

// Uma linha com os dados do voo (os instrumentos), escrita no terminal uma vez por segundo.
// Soleira: distância até ao início da pista (negativa depois de a passar). Eixo: desvio lateral do eixo da pista.
// Velocidade do ar = velocidade no solo + vento de frente (é a que o piloto vê no instrumento).
void printFlight() {
    // nomes já com espaços até 12 colunas (o printf conta os acentos como 2 letras e desalinhava a tabela)
    const char* phases[] = {"aproximação ", "flare       ", "travagem    ", "parado      "};
    const Flight& f = flight;
    printf("%6.0f s  %s %6.0f m %5.0f m %6.0f ft %5.0f kt %6.0f kt %8.0f ft/min %6.1f° %6.1f° %5.1f°\n", f.time,
           phases[f.phase], f.pos.z, f.pos.x, f.pos.y / FT, (f.groundSpeed + HEADWIND) / KT, f.groundSpeed / KT,
           f.vs / FT * 60, f.pitch, f.roll, f.yaw);
}

// Avança o procedimento dt segundos, conforme a fase em que o avião está.
void updateLanding(float dt) {
    Flight& f = flight;
    f.time += dt;
    if (f.phase == APPROACH) {
        // localizer: aceleração lateral que leva o avião ao eixo sem oscilar; o pranchamento é o dessa volta
        float ax = -0.0064f * f.pos.x - 0.16f * f.vx;
        f.vx += ax * dt;
        f.pos.x += f.vx * dt;
        f.roll = degrees(atan(-ax / G)) + 1.5f * sin(f.time * 1.7f);  // + turbulência ligeira
        f.yaw = degrees(CRAB - atan(f.vx / f.groundSpeed));
        f.vs = -f.groundSpeed * tan(GLIDE);  // rampa de descida
        f.pitch = 10.0f + 0.8f * sin(f.time * 1.1f);
        if (f.pos.y < FLARE_HEIGHT) {
            f.phase = FLARE;
            printf(">>> Flare a 20 ft, %.0f m depois da soleira: potência a zero, nariz para cima, tirar a deriva\n", -f.pos.z);
        }
    } else if (f.phase == FLARE) {  // potência a zero, nariz para cima, tira a deriva
        f.vs += (-0.5f * f.pos.y - 0.3f - f.vs) * 2.0f * dt;
        f.groundSpeed -= 1.0f * dt;
        f.pitch += (13.0f - f.pitch) * dt;
        f.yaw *= 1.0f - 1.5f * dt;
        f.roll *= 1.0f - 2.0f * dt;
        if (f.pos.y <= 0.0f) {  // toque
            f.phase = ROLLOUT;
            f.touchdownVs = f.vs;
            f.touchdownZ = f.pos.z;
            printf(">>> Toque %.0f m depois da soleira, a %.0f ft/min e %.0f kt: baixar o nariz e travar\n", -f.touchdownZ,
                   f.touchdownVs / FT * 60, (f.groundSpeed + HEADWIND) / KT);
            f.pos.y = f.vs = f.yaw = f.roll = 0.0f;
        }
    } else if (f.phase == ROLLOUT) {  // baixa o nariz e depois trava
        f.pitch = max(f.pitch - 3.0f * dt, 0.0f);
        f.groundSpeed -= (f.pitch > 0.0f ? 0.5f : BRAKING) * dt;
        if (f.groundSpeed <= 0.0f) {
            f.groundSpeed = 0.0f;
            f.phase = STOPPED;
            printFlight();
            printf(">>> Parado %.0f m depois da soleira (%.0f m depois do toque). R recomeça.\n", -f.pos.z,
                   f.touchdownZ - f.pos.z);
        }
    }
    f.pos.y = max(f.pos.y + f.vs * dt, 0.0f);
    f.pos.z -= f.groundSpeed * dt;
}

mat4 aircraftModel() {  // translação até às rodas, guinada, picada e rolamento à volta do trem principal
    mat4 m = translate(mat4(1.0f), flight.pos);
    m = rotate(m, radians(flight.yaw), vec3(0, 1, 0));
    m = rotate(m, radians(flight.pitch), vec3(1, 0, 0));
    m = rotate(m, radians(flight.roll), vec3(0, 0, 1));
    return translate(m, vec3(0.0f, 0.0f, -MAIN_GEAR_Z));
}
