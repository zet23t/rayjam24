#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <external/glad.h>

static Model model;
static Shader defaultShader;
static Shader shader;
static Shader outlineShader;
static RenderTexture2D target = {0};
static Font fntMedium = {0};

void Game_init()
{
    printf("Game_init\n");
    model = LoadModel("resources/polyobjects.glb");
    shader = LoadShader("resources/dither.vs", "resources/dither.fs");
    outlineShader = LoadShader(0, "resources/outline.fs");
    defaultShader = model.materials[0].shader;
    model.materials[0].shader = shader;
    model.materials[1].shader = shader;

    
    fntMedium = LoadFont("resources/fnt_medium.png");
}

void Game_deinit()
{
    printf("Game_deinit\n");
    UnloadModel(model);
    UnloadShader(shader);
    UnloadShader(outlineShader);
    UnloadRenderTexture(target);
    UnloadFont(fntMedium);
}


void Game_update()
{
    static int mode = 0;
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    if ((screenWidth >> 1) != target.texture.width || (screenHeight >> 1) != target.texture.height)
    {
        UnloadRenderTexture(target);
        target = LoadRenderTexture(screenWidth>>1, screenHeight>>1);
        SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    }

    BeginTextureMode(target);
    ClearBackground(WHITE);
    // float time = sinf(GetTime() * 0.5f) * 0.0f + PI * 1.25f;
    // float camx = sinf(time) * 5.0f;
    // float camz = cosf(time) * 5.0f;
    static Camera3D camera = {
        .fovy = 45.0f,
        .target = {0.0f, 0.25f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .position = {-4.0f, 2.5f, -3.0f},
    };

    UpdateCamera(&camera, CAMERA_THIRD_PERSON);
    rlDisableColorBlend();

    // rlSetBlendMode(RL_BLEND_CUSTOM);
    // rlSetBlendFactors(GL_ONE, GL_ZERO, GL_FUNC_ADD);
    BeginMode3D(camera);

    float clockTime = GetTime();
    SetShaderValue(shader, GetShaderLocation(shader, "time"), &clockTime, SHADER_UNIFORM_FLOAT);
    DrawModel(model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

    EndMode3D();
    EndTextureMode();

    rlEnableColorBlend();
    // rlSetBlendMode(RL_BLEND_ALPHA);
    BeginDrawing();
    if ((mode & 1) == 0)
    {
        BeginShaderMode(outlineShader);
    }
    SetShaderValue(outlineShader, GetShaderLocation(outlineShader, "resolution"), (float[2]){(float)target.texture.width, (float)target.texture.height}, SHADER_UNIFORM_VEC2);
    DrawTexturePro(target.texture, 
        (Rectangle){0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height}, 
        (Rectangle){0.0f, 0.0f, (float)screenWidth, (float)screenHeight}, 
        (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    if ((mode & 1) == 0)
        EndShaderMode();


    // int mouseX = GetMouseX();
    // int mouseY = GetMouseY();
    // int hoverA = (mouseX > 0 && mouseX < 20 && mouseY > 5 && mouseY < 25);
    // int hoverB = (mouseX > 30 && mouseX < 50 && mouseY > 5 && mouseY < 25);
    // int hoverC = (mouseX > 55 && mouseX < 75 && mouseY > 5 && mouseY < 25);

    // mode = hoverA ? 1 : hoverB ? 2 : hoverC ? 3 : mode;

    // if (mode & 2)
    // {
    //     model.materials[0].shader = defaultShader;
    //     model.materials[1].shader = defaultShader;
    // }
    // else
    // {
    //     model.materials[0].shader = shader;
    //     model.materials[1].shader = shader;
    // }

    // DrawRectangle(5, 5, 20, 20, hoverA ? RED : WHITE);
    // DrawRectangle(30, 5, 20, 20, hoverB ? RED : WHITE);
    // DrawRectangle(55, 5, 20, 20, hoverC ? RED : WHITE);
    // rlEnableColorBlend();
    // DrawRectangle(20, 20, 200, 200, WHITE);
    // DrawTextEx(fntMedium, "Dithering & outlining:\n- foo", (Vector2){26, 24}, fntMedium.baseSize * 2.0f, -2.0f, WHITE);
    EndDrawing();
}