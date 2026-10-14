#ifndef GAMESAVE_H
#define GAMESAVE_H

#include "GameTypes.h"

bool saveGameToFile(const SaveData& data, const char* path);
bool loadGameFromFile(SaveData& data, const char* path);   
bool saveFileExists(const char* path);

#endif
