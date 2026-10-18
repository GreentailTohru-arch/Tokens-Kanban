#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* Caller serializes this bounded journal. New sessions discard old actions. */
typedef struct {unsigned session,transport,seq,action;uint64_t polled,created;bool voice;} pet_remote_t;
void pet_remote_update(pet_remote_t *r,unsigned action,bool voice,uint64_t now);
size_t pet_remote_line(pet_remote_t *r,const char *line,char *out,size_t cap,unsigned transport,uint64_t now);
