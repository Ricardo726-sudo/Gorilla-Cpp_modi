#include "raylib.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 450;
const double MY_PI = 3.14159265358979323846;

struct Player { 
    float x, y; 
    int score; 
    std::string name;
    bool isAI; 
};
struct Building { float x, width, height; };

enum GameState { MENU, PLAYING, WAITING_INPUT, CPU_THINKING, SHOOTING, GAME_OVER };
GameState state = MENU;

Player p1 = {0, 0, 0, "Jugador 1", false};
Player p2 = {0, 0, 0, "CPU", true}; 
Building buildings[10];
int wind = 0;
int roundsToWin = 3;
int currentTurn = 1; 

float px, py, vx, vy;
const double gravity_scaled = 30.0; 
double wind_scaled = 0;

float cpuThinkTimer = 0.0f;

int inputAngle = 45;
int inputVelocity = 100;
bool awaitingAngle = false;
bool awaitingVelocity = false;
std::string angleStr = "";
std::string velocityStr = "";

// CAMBIO DE ANGULOS: Aumenté maxChars a 3 para permitir escribir "180"
void HandleNumberInput(std::string &buffer, int maxChars, int maxValue) {
    int key = GetCharPressed();
    if (key > 0) {
        if (key >= '0' && key <= '9' && buffer.length() < maxChars) {
            buffer += (char)key;
        }
    }
    if (IsKeyPressed(KEY_BACKSPACE) && buffer.length() > 0) {
        buffer.pop_back();
    }
}

void DrawGorilla(Player p, bool isActive) {
    Color bodyColor = (p.name == "Jugador 1") ? BROWN : DARKBROWN;
    float dir = (p.name == "Jugador 1") ? 1 : -1;

    DrawCircle(p.x, p.y, 18, bodyColor);
    DrawCircle(p.x, p.y - 25, 12, bodyColor);
    DrawCircle(p.x + 5*dir, p.y - 28, 4, WHITE);
    DrawCircle(p.x + 6*dir, p.y - 28, 2, BLACK);
    DrawLineEx({p.x, p.y - 10}, {p.x + 25*dir, p.y - 35}, 5, bodyColor);
    DrawLineEx({p.x, p.y - 10}, {p.x - 15*dir, p.y + 5}, 5, bodyColor);
    DrawRectangle(p.x - 20, p.y + 18, 40, 5, LIGHTGRAY);

    int textWidth = MeasureText(p.name.c_str(), 16);
    DrawRectangle(p.x - textWidth/2 - 5, p.y - 52, textWidth + 10, 18, Fade(BLACK, 0.8f));
    DrawText(p.name.c_str(), p.x - textWidth/2, p.y - 50, 16, WHITE);

    if (isActive) {
        float radius = 35 + sin(GetTime() * 5) * 5;
        DrawCircleLines(p.x, p.y - 15, radius, RED);
    }
}

// CAMBIO DE ANGULOS: Reescritura de la lógica de la IA para ángulos de 0 a 180
void CalculateAIShot(Player ai, Player target, int &outAngle, int &outVelocity) {
    float dx = target.x - ai.x;
    float dy = target.y - ai.y; 
    
    float T = 4.0f + static_cast<float>(rand() % 300) / 100.0f; 
    
    float calculated_vx = (dx - 0.5 * wind_scaled * T * T) / T;
    float calculated_vy = (dy - 0.5 * gravity_scaled * T * T) / T;

    outVelocity = sqrt(calculated_vx * calculated_vx + calculated_vy * calculated_vy);
    
    // atan2 devuelve radianes. En C++, si el tiro es a la izquierda, puede dar negativo (ej: -45 grados).
    outAngle = atan2(-calculated_vy, calculated_vx) * 180.0 / MY_PI; 

    // CAMBIO DE ANGULOS: Normalización a semicírculo (0 a 180)
    // Si el ángulo es negativo (porque apunta a la izquierda), lo convertimos a su equivalente positivo sumando 180.
    // Ejemplo: -45 grados -> 135 grados (que es exactamente arriba a la izquierda).
    if (outAngle < 0) {
        outAngle += 180.0;
    }

    // MARGEN DE ERROR adaptado a 0-180
    outAngle += (rand() % 15) - 7;    
    outVelocity += (rand() % 40) - 20; 

    // Limitar a los nuevos rangos (0 a 180)
    if (outAngle < 5) outAngle = 5;
    if (outAngle > 175) outAngle = 175; // No dejar que llegue a 180 plano para que siempre tenga un poco de arco
    if (outVelocity < 50) outVelocity = 50;
    if (outVelocity > 200) outVelocity = 200;
}

void SetupRound() {
    float currentX = 0;
    float bWidth = SCREEN_WIDTH / 10.0f;
    for(int i=0; i<10; i++) {
        buildings[i].x = currentX;
        buildings[i].width = bWidth;
        buildings[i].height = (rand() % 150) + 100;
        currentX += bWidth;
    }

    p1.x = buildings[0].x + buildings[0].width / 2;
    p1.y = SCREEN_HEIGHT - buildings[0].height - 20;
    p2.x = buildings[9].x + buildings[9].width / 2;
    p2.y = SCREEN_HEIGHT - buildings[9].height - 20;

    wind = (rand() % 21) - 10;
    wind_scaled = wind * 2.0; 
    state = WAITING_INPUT;
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Gorilla.bas - Angulos 0 a 180");
    SetTargetFPS(60);
    srand(time(NULL));

    while (!WindowShouldClose()) {
        
        if (state == MENU) {
            if (IsKeyPressed(KEY_ONE)) {
                p2.name = "Jugador 2"; p2.isAI = false;
                p1.score = 0; p2.score = 0;
                currentTurn = 1; 
                SetupRound();
            } else if (IsKeyPressed(KEY_TWO)) {
                p2.name = "CPU"; p2.isAI = true;
                p1.score = 0; p2.score = 0;
                currentTurn = 1; 
                SetupRound();
            }
        }
        else if (state == WAITING_INPUT) {
            Player active = (currentTurn == 1) ? p1 : p2;
            
            if (active.isAI) {
                state = CPU_THINKING;
                cpuThinkTimer = GetTime() + 1.5f; 
            } else {
                if (!awaitingAngle && !awaitingVelocity) {
                    awaitingAngle = true;
                    angleStr = "";
                }
                
                if (awaitingAngle) {
                    // CAMBIO DE ANGULOS: Ahora permitimos 3 caracteres para llegar a "180"
                    HandleNumberInput(angleStr, 3, 180);
                    
                    if (IsKeyPressed(KEY_ENTER) && angleStr.length() > 0) {
                        inputAngle = std::stoi(angleStr);
                        // CAMBIO DE ANGULOS: Nuevo límite máximo de 180
                        if (inputAngle > 180) inputAngle = 180;
                        awaitingAngle = false;
                        awaitingVelocity = true;
                        velocityStr = "";
                    }
                    if (IsKeyPressed(KEY_ESCAPE)) {
                        awaitingAngle = false;
                        angleStr = "";
                    }
                }
                
                if (awaitingVelocity) {
                    HandleNumberInput(velocityStr, 3, 200);
                    if (IsKeyPressed(KEY_ENTER) && velocityStr.length() > 0) {
                        try {
                            inputVelocity = std::stoi(velocityStr);
                            if (inputVelocity < 10) inputVelocity = 10;
                            if (inputVelocity > 300) inputVelocity = 300;
                            awaitingVelocity = false;
                            
                            // La magia de C++: Con esta fórmula, no necesitamos ifs para saber si dispara a izquierda o derecha.
                            // Si ingresas 135, cos(135) es negativo, por lo que vx será negativo (va a la izquierda).
                            // Si ingresas 45, cos(45) es positivo, por lo que vx será positivo (va a la derecha).
                            double rad = inputAngle * MY_PI / 180.0;
                            px = active.x; py = active.y;
                            vx = inputVelocity * cos(rad);
                            vy = -inputVelocity * sin(rad); // Negativo porque el eje Y en pantalla está invertido
                            state = SHOOTING;
                        } catch (const std::exception &e) {
                            velocityStr = "";
                        }
                    }
                    if (IsKeyPressed(KEY_ESCAPE)) {
                        awaitingVelocity = false;
                        awaitingAngle = true;
                        velocityStr = "";
                        angleStr = "";
                    }
                }
            }
        }
        else if (state == CPU_THINKING) {
            if (GetTime() >= cpuThinkTimer) {
                Player active = (currentTurn == 1) ? p1 : p2;
                Player target = (currentTurn == 1) ? p2 : p1;
                int aiAngle, aiVelocity;
                CalculateAIShot(active, target, aiAngle, aiVelocity);
                
                // La IA usa la misma fórmula trigonométrica que el humano
                double rad = aiAngle * MY_PI / 180.0;
                px = active.x; py = active.y;
                vx = aiVelocity * cos(rad);
                vy = -aiVelocity * sin(rad);
                state = SHOOTING; 
            }
        }
        else if (state == SHOOTING) {
            float dt = GetFrameTime();
            px += vx * dt;
            py += vy * dt;
            vy += gravity_scaled * dt;
            vx += wind_scaled * dt;

            bool hitSomething = false;
            Player &target = (currentTurn == 1) ? p2 : p1;

            for(int i=0; i<10; i++) {
                if(px >= buildings[i].x && px <= buildings[i].x + buildings[i].width &&
                   py >= SCREEN_HEIGHT - buildings[i].height) {
                    hitSomething = true; break;
                }
            }

            if(CheckCollisionCircles({px, py}, 8, {target.x, target.y}, 18)) {
                if(currentTurn == 1) p1.score++; else p2.score++;
                hitSomething = true;
            }

            if(px < 0 || px > SCREEN_WIDTH || py > SCREEN_HEIGHT) {
                hitSomething = true;
            }

            if (hitSomething) {
                if (p1.score >= roundsToWin || p2.score >= roundsToWin) {
                    state = GAME_OVER;
                } else {
                    currentTurn = (currentTurn == 1) ? 2 : 1;
                    SetupRound(); 
                }
            }
        }
        else if (state == GAME_OVER) {
            if (IsKeyPressed(KEY_R)) {
                state = MENU;
                currentTurn = 1;
            }
        }

        // --- DIBUJO GRÁFICO ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (state == MENU) {
            DrawText("GORILLA.BAS REINTERPRETADO", SCREEN_WIDTH/2 - 180, 100, 30, BLACK);
            DrawText("Presione [1] para Jugador vs Jugador", SCREEN_WIDTH/2 - 180, 200, 20, DARKGRAY);
            DrawText("Presione [2] para Jugador vs Computadora", SCREEN_WIDTH/2 - 200, 240, 20, DARKGRAY);
        }
        else {
            for(int i=0; i<10; i++) {
                DrawRectangle(buildings[i].x, SCREEN_HEIGHT - buildings[i].height, 
                              buildings[i].width, buildings[i].height, DARKGRAY);
                for(int wy = SCREEN_HEIGHT - buildings[i].height + 10; wy < SCREEN_HEIGHT - 10; wy += 20) {
                    for(int wx = buildings[i].x + 10; wx < buildings[i].x + buildings[i].width - 10; wx += 15) {
                        DrawRectangle(wx, wy, 5, 5, YELLOW);
                    }
                }
            }

            bool p1Active = (currentTurn == 1 && (state == WAITING_INPUT || state == CPU_THINKING || state == SHOOTING));
            bool p2Active = (currentTurn == 2 && (state == WAITING_INPUT || state == CPU_THINKING || state == SHOOTING));
            
            DrawGorilla(p1, p1Active);
            DrawGorilla(p2, p2Active);

            if (state == SHOOTING) {
                DrawCircle(px, py, 6, YELLOW);
                DrawCircle(px, py, 3, ORANGE);
            }

            DrawRectangle(0, 0, SCREEN_WIDTH, 30, Fade(LIGHTGRAY, 0.8f));
            DrawText(TextFormat("%s: %d", p1.name.c_str(), p1.score), 10, 5, 20, BLACK);
            DrawText(TextFormat("%s: %d", p2.name.c_str(), p2.score), SCREEN_WIDTH - 100, 5, 20, BLACK);
            
            std::string windStr = "Viento: " + std::to_string(wind);
            DrawText(windStr.c_str(), SCREEN_WIDTH/2 - 40, 5, 20, BLUE);

            if (state == CPU_THINKING) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
                int alpha = (sin(GetTime() * 8) + 1.0f) * 127; 
                DrawText("La CPU esta calculando su tiro...", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 - 10, 24, (Color){255, 80, 80, (unsigned char)alpha});
            } 
            else if (state == WAITING_INPUT) {
                Player activeName = (currentTurn == 1) ? p1 : p2;
                DrawRectangle(50, 50, SCREEN_WIDTH - 100, 350, Fade(BLACK, 0.7f));
                DrawRectangleLines(50, 50, SCREEN_WIDTH - 100, 350, WHITE);
                
                DrawText(TextFormat("=== TURNO DE %s ===", activeName.name.c_str()), 70, 70, 20, YELLOW);
                DrawText(TextFormat("Viento: %d", wind), 70, 110, 16, SKYBLUE);
                
                if (awaitingAngle) {
                    // CAMBIO DE ANGULOS: Se actualizó el texto en pantalla para el jugador
                    DrawText("Ingrese ANGULO (0-180):", 70, 160, 18, WHITE);
                    DrawRectangle(70, 195, 300, 40, DARKGRAY);
                    DrawRectangleLines(70, 195, 300, 40, GREEN);
                    DrawText(angleStr.empty() ? "_" : angleStr.c_str(), 80, 205, 24, GREEN);
                    DrawText("[ENTER] Confirmar  |  [BACKSPACE] Borrar", 70, 250, 12, GRAY);
                } else if (awaitingVelocity) {
                    DrawText(TextFormat("Angulo: %d  [OK]", inputAngle), 70, 160, 16, GREEN);
                    DrawText("Ingrese VELOCIDAD (10-300):", 70, 200, 18, WHITE);
                    DrawRectangle(70, 235, 300, 40, DARKGRAY);
                    DrawRectangleLines(70, 235, 300, 40, GREEN);
                    DrawText(velocityStr.empty() ? "_" : velocityStr.c_str(), 80, 245, 24, GREEN);
                    DrawText("[ENTER] Lanzar  |  [BACKSPACE] Borrar  |  [ESC] Volver", 70, 290, 12, GRAY);
                }
            }

            if (state == GAME_OVER) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
                std::string winner = (p1.score >= roundsToWin) ? p1.name : p2.name;
                DrawText(TextFormat("GANADOR: %s", winner.c_str()), SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 20, 30, GOLD);
                DrawText("Presione [R] para volver al menu", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 30, 20, WHITE);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}