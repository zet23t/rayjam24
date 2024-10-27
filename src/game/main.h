#ifndef __GAME_MAIN_H__
#define __GAME_MAIN_H__

#include "raylib.h"
#include "rlgl.h"
#include "util.h"

typedef struct Script Script;
typedef struct ScriptAction ScriptAction;

typedef struct ScriptAction {
    int actionIdStart, actionIdEnd;
    void (*action)(Script *script, ScriptAction *action);
    union {
        int actionInt;
        void *actionData;
    };
} ScriptAction;

#define SCRIPT_MAX_ACTIONS 128

typedef struct Script {
    int actionCount;
    ScriptAction actions[SCRIPT_MAX_ACTIONS];
    int currentActionId;
    int nextActionId;
} Script;

extern Script _script;
extern Font _fntMono;
extern Font _fntMedium;

void* pool_alloc(int size, void *data);
void SetSceneDrawingFunction(void (*fn)(void*), void* drawSceneData);

#endif