#ifndef PREADER_H
#define PREADER_H

#include <pd_api.h>

void preader_set_pd_ptr(PlaydateAPI* pd);

void preader_start(void);

void preader_update(void);

void preader_end(void);

#endif