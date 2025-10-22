// ========================================================================
// PrizBik - Cubo de Rubik para Casio Prizm (fx-CG50)
// Versión 1.0
// ========================================================================

#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/rtc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <PrizBik/controls.h> // c array con los controles del programa

// --- Estados del Programa ---
enum GameState {
    STATE_IDLE,
    STATE_SCRAMBLED,
    STATE_TIMING,
    STATE_SOLVED,
    STATE_SHOWING_CONTROLS
};

// --- Constantes para las Caras ---
#define UP      0
#define DOWN    1
#define FRONT   2
#define BACK    3
#define LEFT    4
#define RIGHT   5

// --- Variables Globales ---
#define SCRAMBLE_LENGTH 20
char scramble_sequence[SCRAMBLE_LENGTH][4];
int cube[6][3][3];
enum GameState current_state = STATE_IDLE;
unsigned int solve_start_ticks;
unsigned int solve_end_ticks;

// --- Funciones de Lógica del Cubo ---
int is_cube_solved() {
    for (int face = 0; face < 6; face++) {
        int first_color = cube[face][0][0];
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                if (cube[face][row][col] != first_color) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// --- Funciones Gráficas ---
void draw_sticker(int x, int y, int color) {
    int y_offset = 20;
    int size = 19;
    for (int py = y + 1 + y_offset; py < y + size + y_offset; ++py) {
        for (int px = x + 1; px < x + size; ++px) {
            Bdisp_SetPoint_VRAM(px, py, color);
        }
    }
    for (int i = 0; i <= size; i++) {
        Bdisp_SetPoint_VRAM(x + i, y + y_offset, COLOR_BLACK);
        Bdisp_SetPoint_VRAM(x + i, y + size + y_offset, COLOR_BLACK);
        Bdisp_SetPoint_VRAM(x, y + i + y_offset, COLOR_BLACK);
        Bdisp_SetPoint_VRAM(x + size, y + i + y_offset, COLOR_BLACK);
    }
}

void display_scramble_sequence() { //mostrar el scramble actual
    int start_x = 140;
    int y = 5;
    int x = start_x;
    int right_margin = 380;
    for (int i = 0; i < SCRAMBLE_LENGTH; i++) {
        PrintMini(&x, &y, (const char*)scramble_sequence[i], 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
        x += 5;
        if (x + 25 > right_margin) {
            x = start_x;
            y += 15;
        }
    }
}

void display_timer() { //mostrar el tiempo
    char time_buffer[16];
    unsigned int total_ticks = 0;
    if (current_state == STATE_TIMING) {
        total_ticks = RTC_GetTicks() - solve_start_ticks;
    } else if (current_state == STATE_SOLVED) {
        total_ticks = solve_end_ticks - solve_start_ticks;
    } else {
        return;
    }
    unsigned int total_hundredths = ((unsigned long long)total_ticks * 100) / 128;
    int minutes = total_hundredths / 6000;
    int remaining_hundredths = total_hundredths % 6000;
    int seconds = remaining_hundredths / 100;
    int hundredths = remaining_hundredths % 100;
    if (minutes > 0) {
        sprintf(time_buffer, "%d:%02d.%02d", minutes, seconds, hundredths);
    } else {
        sprintf(time_buffer, "%d.%02d", seconds, hundredths);
    }
    int x = 300;
    int y = 5;
    PrintMini(&x, &y, (const char*)time_buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
}

void draw_controls_screen() { // mostrar la imagen de los controles
    Bdisp_AllClr_VRAM();
    int i = 0;
    for (int y = 0; y < CONTROLS_IMG_HEIGHT; y++) {
        for (int x = 0; x < CONTROLS_IMG_WIDTH; x++) {
            int screen_x = x + 6; //marco blanco de la graficadora
            int screen_y = y + 0;
            if (screen_x < 384 && screen_y < 216) {
                 Bdisp_SetPoint_VRAM(screen_x, screen_y, controls_img_map[i]);
            }
            i++;
        }
    }
    Bdisp_PutDisp_DD();
}

void draw_scene() {
    Bdisp_AllClr_VRAM();
    int sticker_size = 22;
    int face_x[6] = {3*sticker_size, 3*sticker_size, 3*sticker_size, 9*sticker_size, 0*sticker_size, 6*sticker_size};
    int face_y[6] = {0*sticker_size, 6*sticker_size, 3*sticker_size, 3*sticker_size, 3*sticker_size, 3*sticker_size};
    for (int face = 0; face < 6; ++face) {
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                int x = face_x[face] + col * sticker_size;
                int y = face_y[face] + row * sticker_size - 4; // Ajuste vertical
                draw_sticker(x, y, cube[face][row][col]);
            }
        }
    }
    if(current_state == STATE_SCRAMBLED) {
        display_scramble_sequence();
    } else if (current_state == STATE_TIMING || current_state == STATE_SOLVED) {
        display_timer();
    }
    Bdisp_PutDisp_DD();
}

// --- Lógica de Inicialización ---
void init_cube() {
    memset(scramble_sequence, 0, sizeof(scramble_sequence));
    current_state = STATE_IDLE;
    int colors[] = {COLOR_WHITE, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_RED};
    for (int face = 0; face < 6; ++face) { for (int row = 0; row < 3; ++row) { for (int col = 0; col < 3; ++col) { cube[face][row][col] = colors[face]; } } }
}

// --- Lógica de Movimientos ---
void rotate_face_clockwise(int face) { int temp = cube[face][0][0]; cube[face][0][0] = cube[face][2][0]; cube[face][2][0] = cube[face][2][2]; cube[face][2][2] = cube[face][0][2]; cube[face][0][2] = temp; temp = cube[face][0][1]; cube[face][0][1] = cube[face][1][0]; cube[face][1][0] = cube[face][2][1]; cube[face][2][1] = cube[face][1][2]; cube[face][1][2] = temp; }
void rotate_face_counter_clockwise(int face) { rotate_face_clockwise(face); rotate_face_clockwise(face); rotate_face_clockwise(face); }

void move_U() { rotate_face_clockwise(UP); int temp[3]; for(int i=0; i<3; i++) temp[i] = cube[FRONT][0][i]; for(int i=0; i<3; i++) { cube[FRONT][0][i] = cube[RIGHT][0][i]; cube[RIGHT][0][i] = cube[BACK][0][i]; cube[BACK][0][i] = cube[LEFT][0][i]; cube[LEFT][0][i] = temp[i]; } }
void move_D() { rotate_face_clockwise(DOWN); int temp[3]; for(int i=0; i<3; i++) temp[i] = cube[FRONT][2][i]; for(int i=0; i<3; i++) { cube[FRONT][2][i] = cube[LEFT][2][i]; cube[LEFT][2][i] = cube[BACK][2][i]; cube[BACK][2][i] = cube[RIGHT][2][i]; cube[RIGHT][2][i] = temp[i]; } }
void move_F() { rotate_face_clockwise(FRONT); int temp[3]; for(int i=0; i<3; i++) temp[i] = cube[UP][2][i]; for(int i=0; i<3; i++) { cube[UP][2][i] = cube[LEFT][2-i][2]; cube[LEFT][2-i][2] = cube[DOWN][0][2-i]; cube[DOWN][0][2-i] = cube[RIGHT][i][0]; cube[RIGHT][i][0] = temp[i]; } }
void move_B() { rotate_face_clockwise(BACK); int temp[3]; for(int i=0; i<3; i++) temp[i] = cube[UP][0][i]; for(int i=0; i<3; i++) { cube[UP][0][i] = cube[RIGHT][i][2]; cube[RIGHT][i][2] = cube[DOWN][2][2-i]; cube[DOWN][2][2-i] = cube[LEFT][2-i][0]; cube[LEFT][2-i][0] = temp[i]; } }
void move_L() { rotate_face_clockwise(LEFT); int temp[3]; for(int i=0; i<3; i++) temp[i] = cube[UP][i][0]; for(int i=0; i<3; i++) { cube[UP][i][0] = cube[BACK][2-i][2]; cube[BACK][2-i][2] = cube[DOWN][i][0]; cube[DOWN][i][0] = cube[FRONT][i][0]; cube[FRONT][i][0] = temp[i]; } }
void move_R() { rotate_face_clockwise(RIGHT); int temp[3]; for(int i=0; i<3; i++) temp[i] = cube[UP][i][2]; for(int i=0; i<3; i++) { cube[UP][i][2] = cube[FRONT][i][2]; cube[FRONT][i][2] = cube[DOWN][i][2]; cube[DOWN][i][2] = cube[BACK][2-i][0]; cube[BACK][2-i][0] = temp[i]; } }

void move_U_prime(){ move_U(); move_U(); move_U(); }
void move_D_prime(){ move_D(); move_D(); move_D(); }
void move_L_prime(){ move_L(); move_L(); move_L(); }
void move_R_prime(){ move_R(); move_R(); move_R(); }
void move_F_prime(){ move_F(); move_F(); move_F(); }
void move_B_prime(){ move_B(); move_B(); move_B(); }

void move_M_prime() { int temp[3]; for(int i=0; i<3; i++) temp[i] = cube[FRONT][i][1]; for(int i=0; i<3; i++) cube[FRONT][i][1] = cube[DOWN][i][1]; for(int i=0; i<3; i++) cube[DOWN][i][1] = cube[BACK][2-i][1]; for(int i=0; i<3; i++) cube[BACK][i][1] = cube[UP][2-i][1]; for(int i=0; i<3; i++) cube[UP][i][1] = temp[i]; }
void move_M() { move_M_prime(); move_M_prime(); move_M_prime(); }

void move_E() { int temp[3]; for (int i = 0; i < 3; i++) { temp[i] = cube[FRONT][1][i]; } for (int i = 0; i < 3; i++) { cube[FRONT][1][i] = cube[RIGHT][1][i]; } for (int i = 0; i < 3; i++) { cube[RIGHT][1][i] = cube[BACK][1][i]; } for (int i = 0; i < 3; i++) { cube[BACK][1][i] = cube[LEFT][1][i]; } for (int i = 0; i < 3; i++) { cube[LEFT][1][i] = temp[i]; } }
void move_E_prime() { move_E(); move_E(); move_E(); }

void move_r() { move_R(); move_M_prime(); }
void move_r_prime() { move_R_prime(); move_M(); }

void move_y() { rotate_face_clockwise(UP); rotate_face_counter_clockwise(DOWN); int temp_face[3][3]; memcpy(temp_face, cube[FRONT], sizeof(temp_face)); memcpy(cube[FRONT], cube[RIGHT], sizeof(cube[FRONT])); memcpy(cube[RIGHT], cube[BACK], sizeof(cube[RIGHT])); memcpy(cube[BACK], cube[LEFT], sizeof(cube[BACK])); memcpy(cube[LEFT], temp_face, sizeof(cube[LEFT])); }
void move_y_prime() { move_y(); move_y(); move_y(); }

void move_x() { rotate_face_clockwise(RIGHT); rotate_face_counter_clockwise(LEFT); int temp_face[3][3]; memcpy(temp_face, cube[UP], sizeof(temp_face)); memcpy(cube[UP], cube[FRONT], sizeof(cube[UP])); memcpy(cube[FRONT], cube[DOWN], sizeof(cube[FRONT])); for (int i = 0; i < 3; i++) { for (int j = 0; j < 3; j++) { cube[DOWN][i][j] = cube[BACK][2 - i][2 - j]; } } for (int i = 0; i < 3; i++) { for (int j = 0; j < 3; j++) { cube[BACK][i][j] = temp_face[2 - i][2 - j]; } } }
void move_x_prime() { move_x(); move_x(); move_x(); }

void macro_1() { move_R(); move_U(); move_R_prime(); move_F_prime(); move_R(); move_U(); move_R_prime(); move_U_prime(); move_R_prime(); move_F(); move_R(); move_R(); move_U_prime(); move_R_prime(); move_U_prime(); }
void macro_2() { move_F(); move_R(); move_U_prime(); move_R_prime(); move_U_prime(); move_R(); move_U(); move_R_prime(); move_F_prime(); move_R(); move_U(); move_R_prime(); move_U_prime(); move_R_prime(); move_F(); move_R(); move_F_prime(); }

// --- Función de Scramble ---
void scramble_cube() {
    memset(scramble_sequence, 0, sizeof(scramble_sequence));
    const char* face_names[] = {"U", "D", "L", "R", "F", "B"};
    const char* modifier_names[] = {" ", "' ", "2 "};
    int face_axes[] = {0, 0, 1, 1, 2, 2};
    int last_axis = -1;
    for (int i = 0; i < SCRAMBLE_LENGTH; i++) {
        int random_face_idx;
        int current_axis;
        do {
            random_face_idx = rand() % 6;
            current_axis = face_axes[random_face_idx];
        } while (current_axis == last_axis);
        last_axis = current_axis;
        int random_modifier_idx = rand() % 3;
        strcpy(scramble_sequence[i], face_names[random_face_idx]);
        strcat(scramble_sequence[i], modifier_names[random_modifier_idx]);
        void (*move_functions[6][2])() = {
            {move_U, move_U_prime}, {move_D, move_D_prime}, {move_L, move_L_prime},
            {move_R, move_R_prime}, {move_F, move_F_prime}, {move_B, move_B_prime}
        };
        if (random_modifier_idx == 0) {
            move_functions[random_face_idx][0]();
        } else if (random_modifier_idx == 1) {
            move_functions[random_face_idx][1]();
        } else {
            move_functions[random_face_idx][0]();
            move_functions[random_face_idx][0]();
        }
    }
}

// --- Función Principal ---
int main(void) {
    int key;
    int move_made = 0;
    
    EnableStatusArea(0);
    Bdisp_EnableColor(1);
    
    srand(RTC_GetTicks());
    
    init_cube();
    
    while (1) {
        if (current_state == STATE_SHOWING_CONTROLS) {
            draw_controls_screen();
        } else {
            draw_scene();
        }
        
        GetKey(&key);
        move_made = 0;

        switch (key) {
            case KEY_CTRL_F1: // Tecla para mostrar/ocultar los controles
                if (current_state == STATE_SHOWING_CONTROLS) {
                    // Decide a qué estado volver (IDLE si no estaba mezclado, SCRAMBLED si sí)
                    if (strlen(scramble_sequence[0]) > 0) { 
                        current_state = STATE_SCRAMBLED; 
                    } else {
                        current_state = STATE_IDLE;
                    }
                } else {
                    current_state = STATE_SHOWING_CONTROLS;
                }
                break;

            case KEY_CTRL_EXE: 
                init_cube(); 
                scramble_cube(); 
                current_state = STATE_SCRAMBLED;
                break;
            
            case KEY_CTRL_EXIT: return 0;

            default:
                 // Si estamos mostrando controles, cualquier otra tecla nos devuelve al estado anterior
                 if (current_state == STATE_SHOWING_CONTROLS) {
                     if (strlen(scramble_sequence[0]) > 0) { 
                        current_state = STATE_SCRAMBLED; 
                    } else {
                        current_state = STATE_IDLE;
                    }
                    break; // Salir del switch para no procesar la tecla como movimiento
                 }
                 
                // Lógica de movimientos solo si estamos en SCRAMBLED o TIMING
                if (current_state == STATE_SCRAMBLED || current_state == STATE_TIMING) {
                    if (current_state == STATE_SCRAMBLED) {
                        current_state = STATE_TIMING;
                        solve_start_ticks = RTC_GetTicks();
                    }
                    
                    switch (key) {
                        // Movimientos Horarios
                        case KEY_CHAR_6: move_U(); move_made=1; break;
                        case KEY_CHAR_0: move_D(); move_made=1; break;
                        case KEY_CHAR_1: move_L(); move_made=1; break;
                        case KEY_CHAR_MULT: move_R(); move_made=1; break;
                        case KEY_CHAR_3: move_F(); move_made=1; break;
                        case KEY_CHAR_9: move_B(); move_made=1; break;

                        // Movimientos Antihorarios
                        case KEY_CHAR_5: move_U_prime(); move_made=1; break;
                        case KEY_CHAR_PMINUS: move_D_prime(); move_made=1; break;
                        case KEY_CHAR_4: move_L_prime(); move_made=1; break;
                        case KEY_CHAR_PLUS: move_R_prime(); move_made=1; break;
                        case KEY_CHAR_2: move_F_prime(); move_made=1; break;
                        case KEY_CHAR_8: move_B_prime(); move_made=1; break;

                        // Movimientos de Capa Media
                        case KEY_CTRL_FD: move_M_prime(); move_made=1; break;
                        case KEY_CHAR_LOG: move_M(); move_made=1; break;
                        case KEY_CHAR_RPAR: move_E(); move_made=1; break;
                        case KEY_CHAR_LPAR: move_E_prime(); move_made=1; break;

                        // Rotaciones del Cubo Completo
                        case KEY_CHAR_7: move_x(); move_made=1; break;
                        case KEY_CTRL_AC: move_y(); move_made=1; break;
                        case KEY_CTRL_DEL: move_y_prime(); move_made=1; break;
                        case KEY_CHAR_FRAC: move_x_prime(); move_made=1; break;
                        
                        // Wide Moves
                        case KEY_CHAR_DIV: move_r(); move_made=1; break;
                        case KEY_CHAR_MINUS: move_r_prime(); move_made=1; break;

                        // PLL's
                        case KEY_CHAR_STORE: macro_1(); move_made=1; break;
                        case KEY_CHAR_COMMA: macro_2(); move_made=1; break;
                    }
                }
                break;
        }

        if (move_made && current_state == STATE_TIMING) {
            if (is_cube_solved()) {
                current_state = STATE_SOLVED;
                solve_end_ticks = RTC_GetTicks();
            }
        }
    }
    return 0;
}
