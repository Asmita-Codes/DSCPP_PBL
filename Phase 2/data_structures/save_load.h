#ifndef SAVE_LOAD_H
#define SAVE_LOAD_H
#ifdef __cplusplus
extern "C" {
#endif
int save_load_save(const char*path,int score,int health,int kills,int seconds,int rifle,int pistol,int medkits);
int save_load_load(const char*path,int*score,int*health,int*kills,int*seconds,int*rifle,int*pistol,int*medkits);
#ifdef __cplusplus
}
#endif
#endif
