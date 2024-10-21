#include "raylib.h"
#include <stdio.h>

void Game_init()
{
    printf("Game_init\n");
}

void Game_deinit()
{
    printf("Game_deinit\n");
}

void Game_update()
{
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("Congrats! Change the text and type 'R' in the app!", 190, 200, 20, LIGHTGRAY);
    EndDrawing();
}