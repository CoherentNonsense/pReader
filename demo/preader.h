#ifndef PREADER_H
#define PREADER_H

#include <pd_api.h>

extern PlaydateAPI* playdate;

void preader_set_pd_ptr(PlaydateAPI* pd);

void preader_start();

void preader_update();

#endif