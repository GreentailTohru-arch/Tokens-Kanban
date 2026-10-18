#pragma once
#include <stdbool.h>
#include <stdint.h>
enum { PET_ACTION_NONE, PET_ACTION_SEND, PET_ACTION_UNDO, PET_ACTION_TOGGLE, PET_ACTION_APPSHOT, PET_ACTION_VOICE, PET_ACTION_NEXT };
#define PET_ACTION_CUSTOM_FIRST 16
#define PET_ACTION_CUSTOM_LAST 23
enum { PET_CONTROL_PRESS, PET_CONTROL_RELEASE, PET_CONTROL_LONG };
#define PET_BINDINGS 9
typedef struct { uint32_t action[PET_BINDINGS]; } pet_bindings_t;
typedef struct { unsigned key, clicks; bool down, held, voice; uint64_t released, pressed; } pet_controls_t;
pet_bindings_t pet_bindings_default(void);
bool pet_bindings_valid(const pet_bindings_t *b);
void pet_controls_cancel(pet_controls_t *s);
unsigned pet_controls_event(pet_controls_t *s,const pet_bindings_t *b,unsigned key,unsigned event,uint64_t now);
unsigned pet_controls_tick(pet_controls_t *s,const pet_bindings_t *b,uint64_t now);
