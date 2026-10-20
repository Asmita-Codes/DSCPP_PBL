#ifndef MISSION_H
#define MISSION_H

#include <string>
#include "GameTypes.h"

enum class MissionType { ReachZone, CollectFuel, SurviveThenReach };

struct MissionDef {
    const char* title;        
    const char* brief;        
    const char* zoneName;     
    MissionType type;
    Vec2i goalTile;          
    int fuelNeeded;           
    float surviveSeconds;     
    float waveScale;          
    int bossWave;             
};

class MissionManager {
public:
    static int count();
    static const MissionDef& defOf(int index);

    void start(int index);                             
    void restore(int index, int fuel, float surviveLeft);  
    void update(float dt);                              
    void addFuel() { ++fuel_; }

    int index() const { return index_; }
    const MissionDef& def() const { return defOf(index_); }
    bool isLast() const { return index_ >= count() - 1; }
    bool goalUnlocked() const;                          
    int fuel() const { return fuel_; }
    float surviveLeft() const { return surviveLeft_; }
    Vec2i goalTile() const { return def().goalTile; }
    Vec2 goalPos() const { return Vec2(def().goalTile.x * (float)TILE, def().goalTile.y * (float)TILE); }
    std::string progressText() const;

private:
    int index_ = 0;
    int fuel_ = 0;
    float surviveLeft_ = 0.f;
};

#endif
