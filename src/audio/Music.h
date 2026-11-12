#ifndef MUSIC_H
#define MUSIC_H

#include <vector>

constexpr int MUSIC_TRACKS = 4;
const char* musicTitle(int track);                    

std::vector<float> composeMusic(int sampleRate, int track);

#endif
