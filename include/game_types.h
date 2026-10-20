#ifndef GAME_TYPES_H
#define GAME_TYPES_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { int x, y; } DnPoint;
typedef struct { char name[32]; int score; int seconds; } DnScoreEntry;
#ifdef __cplusplus
}
#endif
#endif
