#pragma once
#include "token_model.h"
enum {PET_OFFLINE,PET_ONLINE,PET_THINKING,PET_LIMITED,PET_LISTENING,PET_COMPUTER,PET_COUNT};
unsigned pet_resolve(bool live,int signal,unsigned explicit_state,uint64_t explicit_age);
unsigned pet_frame_at(unsigned state,uint64_t elapsed);
bool pet_parse(const char *text,unsigned *sequence,unsigned *state);
void pet_decode_row(unsigned frame,unsigned y,unsigned x,unsigned width,uint16_t background,uint16_t *out);
