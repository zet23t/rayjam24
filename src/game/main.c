#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <external/glad.h>

static Model model;
static Shader shader;
static Shader outlineShader;
static RenderTexture2D target = {0};

RenderTexture2D LoadRenderTexture32(int width, int height)
{
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create color texture (default to RGBA)
        target.texture.id = rlLoadTexture(NULL, width, height, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.texture.mipmaps = 1;

        // Create depth renderbuffer/texture
        target.depth.id = rlLoadTextureDepth(width, height, true);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach color texture and depth renderbuffer/texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

void Game_init()
{
    printf("Game_init\n");
    model = LoadModel("resources/polyobjects.glb");
    shader = LoadShader("resources/dither.vs", "resources/dither.fs");
    outlineShader = LoadShader(0, "resources/outline.fs");
    model.materials[0].shader = shader;
    model.materials[1].shader = shader;
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    target = LoadRenderTexture32(screenWidth>>2, screenHeight>>2);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
}

void Game_deinit()
{
    printf("Game_deinit\n");
    UnloadModel(model);
    UnloadShader(shader);
    UnloadShader(outlineShader);
    UnloadRenderTexture(target);
}

void Game_update()
{
    BeginTextureMode(target);
    ClearBackground(WHITE);
    float time = sinf(GetTime() * 0.5f) * 0.5f + PI * 1.25f;
    float camx = sinf(time) * 5.0f;
    float camz = cosf(time) * 5.0f;
    Camera3D camera = {
        .fovy = 45.0f,
        .target = (Vector3){0.0f, 0.25f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .position = (Vector3){camx, 2.0f, camz},
    };

    rlDisableColorBlend();

    rlSetBlendMode(RL_BLEND_CUSTOM);
    rlSetBlendFactors(GL_ONE, GL_ZERO, GL_FUNC_ADD);
    BeginMode3D(camera);

    DrawModel(model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

    EndMode3D();
    EndTextureMode();

    rlDisableColorBlend();
    BeginDrawing();
    BeginShaderMode(outlineShader);
    SetShaderValue(outlineShader, GetShaderLocation(outlineShader, "resolution"), (float[2]){(float)target.texture.width, (float)target.texture.height}, SHADER_UNIFORM_VEC2);
    DrawTexturePro(target.texture, 
        (Rectangle){0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height}, 
        (Rectangle){0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()}, 
        (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    EndShaderMode();
    EndDrawing();
}