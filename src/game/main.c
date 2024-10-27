#include "main.h"
#include <math.h>
#include <stdio.h>
#include <external/glad.h>
#include <memory.h>
#include <raymath.h>
#include "scriptactions.h"

typedef struct ConextData {
    int step;
} ContextData;

static ContextData *_contextData;
static Model _model;
static Model _sampleObjects;
static Model _flatVectorScene;
static Model _flatVectorSceneOutlines;

static Shader _defaultShader;
static Shader _shader;
static Shader _outlineShader;
static Shader *_postProcessorShader;
static RenderTexture2D _target = {0};

static int _allocationPoolIndex;
static char _allocationPool[2048];

Font _fntMono = {0};
Font _fntMedium = {0};
Script _script;

void DrawDitheredScene(void* data);
void *_drawSceneData = NULL;
void (*_DrawSceneFn)(void *data) = DrawDitheredScene;

void SetSceneDrawingFunction(void (*fn)(void*), void *drawSceneData)
{
    _drawSceneData = drawSceneData;
    _DrawSceneFn = fn;
}

void* pool_alloc(int size, void *data)
{
    void *ptr = &_allocationPool[_allocationPoolIndex];
    //ensure alignment
    size = (size + 0xf) & ~0xf;
    _allocationPoolIndex += size;
    memcpy(ptr, data, size);
    return ptr;
}


void Script_init()
{
    _script.actionCount = 0;
    _script.currentActionId = 0;
}

void Script_addAction(ScriptAction action)
{
    if (SCRIPT_MAX_ACTIONS == _script.actionCount)
    {
        TraceLog(LOG_ERROR, "Script_addAction: script action count exceeded");
        return;
    }

    // default init, if no end, assume length 1
    if (action.actionIdEnd < action.actionIdStart)
    {
        action.actionIdEnd = action.actionIdStart;
    }

    _script.actions[_script.actionCount++] = action;
}

// actions



void Script_update()
{
    _script.nextActionId = _script.currentActionId;
    for (int i = 0; i < _script.actionCount; i++) 
    {
        ScriptAction *action = &_script.actions[i];
        if (_script.currentActionId >= action->actionIdStart && _script.currentActionId <= action->actionIdEnd)
        {
            action->action(&_script, action);
        }
    }
    _script.currentActionId = _script.nextActionId;
    _contextData->step = _script.currentActionId;
}

void UpdateRenderTexture()
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    if ((screenWidth >> 1) != _target.texture.width || (screenHeight >> 1) != _target.texture.height)
    {
        UnloadRenderTexture(_target);
        _target = LoadRenderTexture(screenWidth>>1, screenHeight>>1);
        SetTextureFilter(_target.texture, TEXTURE_FILTER_POINT);
        SetTextureWrap(_target.texture, TEXTURE_WRAP_CLAMP);
    }
}


void DrawDitheredScene(void *data)
{
    float clockTime = GetTime();
    SetShaderValue(_shader, GetShaderLocation(_shader, "time"), &clockTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(_outlineShader, GetShaderLocation(_outlineShader, "depthOutlineEnabled"), (float[]){1.0f}, SHADER_UNIFORM_FLOAT);
    SetShaderValue(_outlineShader, GetShaderLocation(_outlineShader, "uvOutlineEnabled"), (float[]){1.0f}, SHADER_UNIFORM_FLOAT);

    DrawModel(_model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    _postProcessorShader = &_outlineShader;
}

typedef struct OutlineSceneConfig {
    Model *model;
    int drawDepthOutlineMode;
    int drawUvOutlineMode;
} OutlineSceneConfig;

void DrawOutlinedScene(void *data)
{
    OutlineSceneConfig *config = (OutlineSceneConfig*)data;
    Model *model = config->model;
    float clockTime = GetTime();
    int blink = fmodf(clockTime,1.0f) > 0.5f;
    SetShaderValue(_outlineShader, GetShaderLocation(_outlineShader, "depthOutlineEnabled"), 
        (float[]){blink && config->drawDepthOutlineMode == 1 || config->drawDepthOutlineMode > 1 ? 1.0f : 0.0f}, SHADER_UNIFORM_FLOAT);
    SetShaderValue(_outlineShader, GetShaderLocation(_outlineShader, "uvOutlineEnabled"), 
        (float[]){blink && config->drawUvOutlineMode == 1 || config->drawUvOutlineMode > 1 ? 1.0f : 0.0f}, SHADER_UNIFORM_FLOAT);

    model->materials[1].shader = _shader;
    DrawModel(*model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    _postProcessorShader = &_outlineShader;
}

void DrawSimpleScene(void *data)
{
    Model *model = (Model*)data;
    model->materials[1].shader = _defaultShader;
    DrawModel(*model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    _postProcessorShader = NULL;
}

void Game_init(void** contextData)
{
    if (*contextData == NULL)
    {
        *contextData = MemAlloc(sizeof(ContextData));
    }
    _contextData = *contextData;

    printf("Game_init\n");
    
    
    _model = LoadModel("resources/polyobjects.glb");
    _sampleObjects = LoadModel("resources/sampleObjects.glb");
    _flatVectorScene = LoadModel("resources/flat-vector-scene.glb");
    _flatVectorSceneOutlines = LoadModel("resources/flat-vector-scene-outlines.glb");


    _shader = LoadShader("resources/dither.vs", "resources/dither.fs");
    _outlineShader = LoadShader(0, "resources/outline.fs");
    SetShaderValue(_outlineShader, GetShaderLocation(_outlineShader, "depthOutlineEnabled"), (float[]){1.0f}, SHADER_UNIFORM_FLOAT);
    SetShaderValue(_outlineShader, GetShaderLocation(_outlineShader, "uvOutlineEnabled"), (float[]){1.0f}, SHADER_UNIFORM_FLOAT);
    _defaultShader = _model.materials[0].shader;
    _model.materials[0].shader = _shader;
    _model.materials[1].shader = _shader;

    _fntMedium = LoadFont("resources/fnt_medium.png");
    _fntMono = LoadFont("resources/fnt_mymono.png");

    UpdateRenderTexture();

    Script_init();
    SetTextLineSpacingEx(-6);
    int step = 0;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Dithering & Outlining howto",
            "This is a tutorial on how dithering and outline rendering can be implemented.\n"
            "The goal is to show how the principles works - not to provide a production ready efficient implementation.\n"
            , (Rectangle){20, 20, 280, 200})
    });

    step += 1;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Dithering & Outlining howto",
            "Furthermore, this tutorial describes only ONE way to do this.\n"
            "There are countless other approaches.\n"
            , (Rectangle){20, 20, 280, 200})
    });

    step += 1;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Dithering", 
            "Dithering refers to using a few colors and coloring pixels in a way to emulate more colors.\n"
            , 
            (Rectangle){20, 20, 200, 200})
    });
    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .actionIdEnd = step + 1,
        .action = ScriptAction_drawMagnifiedTexture,
        .actionData = ScriptAction_DrawMagnifiedTextureData_new(
            (Rectangle){180, 140, 16, 16},
            (Rectangle){20, 240, 200, 200},
            &_target.texture, _outlineShader)
    });

    step += 1;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Dithering", 
            "Adapted from print media, it was used when computers were only able to draw images using a limited amount of colors."
            , 
            (Rectangle){20, 20, 200, 200})
    });
    
    step += 1;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Dithering", 
            "In this case, the reproduction of this style was aimed to work with fairly simple "
            "tricks."
            , 
            (Rectangle){20, 20, 200, 200})
    });

    

    Script_addAction((ScriptAction){
        .actionIdStart = 0,
        .actionIdEnd = step,
        .action = ScriptAction_setDrawScene,
        .actionData = ScriptAction_SetDrawSceneData_new(DrawDitheredScene, NULL)
    });

    step += 1;
    int simpleSceneStartStep = step;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Pixel art textures", 
            "Let's start with a simple 3d scene, using a regular textures.\n"
            "When using a perspective camera, texture scales will vary a lot."
            , 
            (Rectangle){20, 20, 200, 220})
    });

    step += 1;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Pixel art textures", 
            "This is causing a lot of aliasing when the object is far away,"
            " and big pixels when the object is closer than it was designed for."
            , 
            (Rectangle){20, 20, 200, 200})
    });

    step += 1;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Pixel art textures", 
            "This could be tackled using an orthographic camera and carefuly,"
            "choosing the right texture sizes, but this limits the freedom of the camera."
            , 
            (Rectangle){20, 20, 200, 220})
    });

    Script_addAction((ScriptAction){
        .actionIdStart = simpleSceneStartStep,
        .actionIdEnd = step,
        .action = ScriptAction_setDrawScene,
        .actionData = ScriptAction_SetDrawSceneData_new(DrawSimpleScene, &_sampleObjects)
    });

    step += 1;

    int vectorSceneChapterStart = step;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Flat colored meshes", 
            "Another approach is to use flat colored meshes.\n"
            "Each triangle is colored with a single color, producing a style "
            "that resembles a vector draw style."
            , 
            (Rectangle){20, 20, 200, 220})
    });

    step += 1;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Flat colored meshes", 
            "This approach uses more is more complex to model.\n"
            "It scales nicely, but there's still aliasing."
            , 
            (Rectangle){20, 20, 200, 220})
    });


    Script_addAction((ScriptAction){
        .actionIdStart = vectorSceneChapterStart,
        .actionIdEnd = step,
        .action = ScriptAction_setDrawScene,
        .actionData = ScriptAction_SetDrawSceneData_new(DrawSimpleScene, &_flatVectorScene)
    });

    step += 1;

    int vectorSceneWithOutlines = step;

    
    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Outlines", 
            "We need proper outlines that are 1-2 pixel wide.\n"
            "For that, we need post processing."
            , 
            (Rectangle){20, 20, 200, 220})
    });

    
    static OutlineSceneConfig noBlinkingOutlineFlatV = {
        0
    };
    noBlinkingOutlineFlatV.model = &_flatVectorScene;
    
    Script_addAction((ScriptAction){
        .actionIdStart = vectorSceneWithOutlines,
        .actionIdEnd = step,
        .action = ScriptAction_setDrawScene,
        .actionData = ScriptAction_SetDrawSceneData_new(DrawOutlinedScene, &noBlinkingOutlineFlatV)
    });

    step += 1;
    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Outlines", 
            "In a first step, we create outlines by depth:\n"
            "When a pixel is sufficiently far distanced from the surrounding pixels, we draw an outline."
            , 
            (Rectangle){20, 20, 200, 220})
    });

    static OutlineSceneConfig blinkingDepthOutlineFlatV = {
        0
    };
    blinkingDepthOutlineFlatV.model = &_flatVectorScene;
    blinkingDepthOutlineFlatV.drawDepthOutlineMode = 1;
    blinkingDepthOutlineFlatV.drawUvOutlineMode = 0;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_setDrawScene,
        .actionData = ScriptAction_SetDrawSceneData_new(DrawOutlinedScene, &blinkingDepthOutlineFlatV)
    });

    step += 1;
    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Outlines", 
            "The object boundaries are now clear. The far distant objects don't have an outline "
            "because our depth buffer range runs out - but at that point, we don't want too many outlines anyway."
            , 
            (Rectangle){20, 20, 200, 290})
    });

    static OutlineSceneConfig enabledDepthOutlineFlatV = {
        0
    };
    enabledDepthOutlineFlatV.model = &_flatVectorScene;
    enabledDepthOutlineFlatV.drawDepthOutlineMode = 2;
    enabledDepthOutlineFlatV.drawUvOutlineMode = 0;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_setDrawScene,
        .actionData = ScriptAction_SetDrawSceneData_new(DrawOutlinedScene, &enabledDepthOutlineFlatV)
    });

    step += 1;
    
    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Outlines", 
            "However, we want more outlines. We can use the UV coordinates as outline indicators as well!"
            , 
            (Rectangle){20, 20, 200, 290})
    });

    static OutlineSceneConfig blinkingUvOutlineFlatV = {
        0
    };
    blinkingUvOutlineFlatV.model = &_flatVectorScene;
    blinkingUvOutlineFlatV.drawDepthOutlineMode = 2;
    blinkingUvOutlineFlatV.drawUvOutlineMode = 1;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_setDrawScene,
        .actionData = ScriptAction_SetDrawSceneData_new(DrawOutlinedScene, &blinkingUvOutlineFlatV)
    });

    step += 1;
    
    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Outlines", 
            "Other outline indicators could be used as well, such as normals or other vertex attributes.\n"
            "The overall result still doesn't look great."
            , 
            (Rectangle){20, 20, 200, 220})
    });

    static OutlineSceneConfig fulloutlinesFlatV = {
        0
    };
    fulloutlinesFlatV.model = &_flatVectorScene;
    fulloutlinesFlatV.drawDepthOutlineMode = 2;
    fulloutlinesFlatV.drawUvOutlineMode = 2;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_setDrawScene,
        .actionData = ScriptAction_SetDrawSceneData_new(DrawOutlinedScene, &fulloutlinesFlatV)
    });

    step += 1;
    
    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_drawTextRect,
        .actionData =  ScriptAction_DrawTextRectData_new(
            "Outlines", 
            "Using the UV coordinates to outline the objects is simple and "
            "the result is looking much more like pixel art than using "
            "pixel art textures directly."
            , 
            (Rectangle){20, 20, 200, 220})
    });

    static OutlineSceneConfig fulloutlines2FlatV = {
        0
    };
    fulloutlines2FlatV.model = &_flatVectorSceneOutlines;
    fulloutlines2FlatV.drawDepthOutlineMode = 2;
    fulloutlines2FlatV.drawUvOutlineMode = 2;

    Script_addAction((ScriptAction){
        .actionIdStart = step,
        .action = ScriptAction_setDrawScene,
        .actionData = ScriptAction_SetDrawSceneData_new(DrawOutlinedScene, &fulloutlines2FlatV)
    });

    step += 1;

    // static Camera3D camera = {
    //     .fovy = 60,
    //     .target = {0.0f, 0.0f, 0.0f},
    //     .up = {0.0f, 1.0f, 0.0f},
    //     .position = {-4.0f, 2.5f, -3.0f},
    //     .projection = CAMERA_PERSPECTIVE,
    // };
    
    // Script_addAction((ScriptAction){
    //     .actionIdStart = step,
    //     .action = ScriptAction_drawMesh,
    //     .actionData = ScriptAction_DrawMeshData_new(
    //         &_sampleObjects.meshes[0],
    //         _shader,
    //         &_sampleObjects.materials[1],
    //         MatrixIdentity(),
    //         &camera
    //     ),
    // });

    _script.currentActionId = _contextData->step;

    Script_addAction((ScriptAction){
        .actionIdStart = 0, .actionIdEnd = step, .action = ScriptAction_jumpStep, .actionData = ScriptAction_JumpStepData_new(-1, 1, 1),
    });
}

void Game_deinit()
{
    printf("Game_deinit\n");
    UnloadModel(_model);
    UnloadModel(_sampleObjects);
    UnloadModel(_flatVectorScene);
    UnloadModel(_flatVectorSceneOutlines);
    UnloadShader(_shader);
    UnloadShader(_outlineShader);
    UnloadRenderTexture(_target);
    UnloadFont(_fntMedium);
    UnloadFont(_fntMono);
}

void DrawScene()
{
    if (_DrawSceneFn)
    {
        _DrawSceneFn(_drawSceneData);
    }
}

void Game_update()
{
    static int mode = 0;
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    UpdateRenderTexture();

    BeginTextureMode(_target);
    ClearBackground(WHITE);
    // float time = sinf(GetTime() * 0.5f) * 0.0f + PI * 1.25f;
    static Vector2 rotation = {1.0f, -2.5f};
    static Vector2 rotateVelocity = {0.0f, 0.0f};
    static float distance = 5.0f;
    static float distanceVelocity = 0.0f;
    const float MaxDistance = 10.0f;
    const float MinDistance = 2.5f;

    if (rotation.x > 1.6f) rotateVelocity.x -= 20.1f * GetFrameTime();
    if (rotation.x < 0.8f) rotateVelocity.x += 20.1f * GetFrameTime();
    if (distance < MinDistance + 1.0f) distanceVelocity += (MinDistance - distance + 1.0f) * GetFrameTime();
    if (distance > MaxDistance - 1.0f) distanceVelocity -= (distance - MaxDistance + 1.0f) * GetFrameTime();
    distance += distanceVelocity;
    rotation.x += rotateVelocity.x * GetFrameTime();
    rotation.y += rotateVelocity.y * GetFrameTime();
    if (rotation.x > 1.7f) rotation.x = 1.7f;
    if (rotation.x < 0.4f) rotation.x = 0.4f;
    if (distance < MinDistance) distance = MinDistance, distanceVelocity = 0.0f;
    if (distance > MaxDistance) distance = MaxDistance, distanceVelocity = 0.0f;

    float decay = 1.0f - GetFrameTime() * distance;
    rotateVelocity.x *= decay;
    rotateVelocity.y *= decay;
    distanceVelocity *= decay;

    float rad = sinf(rotation.x) * distance;
    float camy = cosf(rotation.x) * distance;
    float camx = sinf(rotation.y) * rad;
    float camz = cosf(rotation.y) * rad;
    static Camera3D camera = {
        .fovy = 45.0f,
        .target = {0.0f, 0.25f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .position = {-4.0f, 2.5f, -3.0f},
    };
    camera.position.x = camx;
    camera.position.y = camy;
    camera.position.z = camz;
    static int wasDown = 0;
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        if (wasDown)
        {
            rotateVelocity.x -= GetMouseDelta().y * 0.015f;
            rotateVelocity.y -= GetMouseDelta().x * 0.015f;
        }
        wasDown = 1;
    }
    else {
        wasDown = 0;
    }
    distanceVelocity -= GetMouseWheelMove() * 0.1f;


    // UpdateCamera(&camera, CAMERA_THIRD_PERSON);
    rlDisableColorBlend();

    // rlSetBlendMode(RL_BLEND_CUSTOM);
    // rlSetBlendFactors(GL_ONE, GL_ZERO, GL_FUNC_ADD);
    BeginMode3D(camera);

    DrawScene();

    EndMode3D();
    EndTextureMode();

    rlEnableColorBlend();
    // rlSetBlendMode(RL_BLEND_ALPHA);
    BeginDrawing();
    if ((mode & 1) == 0 && _postProcessorShader)
    {
        BeginShaderMode(*_postProcessorShader);
   }
    SetShaderValue(_outlineShader, GetShaderLocation(_outlineShader, "resolution"), (float[2]){(float)_target.texture.width, (float)_target.texture.height}, SHADER_UNIFORM_VEC2);
    DrawTexturePro(_target.texture, 
        (Rectangle){0.0f, 0.0f, (float)_target.texture.width, (float)-_target.texture.height}, 
        (Rectangle){0.0f, 0.0f, (float)screenWidth, (float)screenHeight}, 
        (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    if ((mode & 1) == 0 && _postProcessorShader)
        EndShaderMode();


    // int mouseX = GetMouseX();
    // int mouseY = GetMouseY();
    // int hoverA = (mouseX > 0 && mouseX < 20 && mouseY > 5 && mouseY < 25);
    // int hoverB = (mouseX > 30 && mouseX < 50 && mouseY > 5 && mouseY < 25);
    // int hoverC = (mouseX > 55 && mouseX < 75 && mouseY > 5 && mouseY < 25);

    // mode = hoverA ? 1 : hoverB ? 2 : hoverC ? 3 : mode;

    // if (mode & 2)
    // {
    //     _model.materials[0].shader = defaultShader;
    //     _model.materials[1].shader = defaultShader;
    // }
    // else
    // {
    //     _model.materials[0].shader = _shader;
    //     _model.materials[1].shader = _shader;
    // }

    // DrawRectangle(5, 5, 20, 20, hoverA ? RED : WHITE);
    // DrawRectangle(30, 5, 20, 20, hoverB ? RED : WHITE);
    // DrawRectangle(55, 5, 20, 20, hoverC ? RED : WHITE);
    rlEnableColorBlend();

    Script_update();

    // DrawRectangle(20, 20, 200, 200, WHITE);
    // DrawRectangleLines(21, 21, 198, 198, BLACK);
    // DrawRectangleLines(20, 20, 200, 200, BLACK);
    // DrawTextEx(fntMedium, "Dithering & outlining\n", (Vector2){26, 24}, fntMedium.baseSize * 2.0f, -2.0f, WHITE);
    EndDrawing();
}