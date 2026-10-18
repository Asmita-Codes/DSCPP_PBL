#include "Mission.h"
#include <algorithm>

namespace {
const MissionDef kMissions[3] = {
    {"SAFE ZONE", "Reach the Safe Zone", "SAFE ZONE", MissionType::ReachZone, {49, 31}, 0, 0.f, 1.0f, 0},
    {"FUEL RUN", "Collect 3 fuel cans, then reach the Evacuation Point", "EVACUATION POINT", MissionType::CollectFuel, {49, 1}, 3, 0.f, 1.2f, 0},
    {"LAST STAND", "Survive 75 seconds, then reach the Helipad", "HELIPAD", MissionType::SurviveThenReach, {25, 21}, 0, 75.f, 1.4f, 3},
};
}

int MissionManager::count() { return 3; }

const MissionDef& MissionManager::defOf(int index) { return kMissions[std::clamp(index, 0, count() - 1)]; }

void MissionManager::start(int index) {
    index_ = std::clamp(index, 0, count() - 1);
    fuel_ = 0;
    surviveLeft_ = def().type == MissionType::SurviveThenReach ? def().surviveSeconds : 0.f;
}

void MissionManager::restore(int index, int fuel, float surviveLeft) {
    index_ = std::clamp(index, 0, count() - 1);
    fuel_ = std::max(0, fuel);
    surviveLeft_ = std::max(0.f, surviveLeft);
}

void MissionManager::update(float dt) {
    if (def().type == MissionType::SurviveThenReach && surviveLeft_ > 0.f) surviveLeft_ = std::max(0.f, surviveLeft_ - dt);
}

bool MissionManager::goalUnlocked() const {
    switch (def().type) {
    case MissionType::ReachZone: return true;
    case MissionType::CollectFuel: return fuel_ >= def().fuelNeeded;
    case MissionType::SurviveThenReach: return surviveLeft_ <= 0.f;
    }
    return false;
}

std::string MissionManager::progressText() const {
    switch (def().type) {
    case MissionType::ReachZone: return "Reach the safe zone";
    case MissionType::CollectFuel:
        return fuel_ < def().fuelNeeded ? "Fuel cans: " + std::to_string(fuel_) + "/" + std::to_string(def().fuelNeeded)
                                        : "Go to the evacuation point";
    case MissionType::SurviveThenReach:
        return surviveLeft_ > 0.f ? "Survive: " + std::to_string((int)std::ceil(surviveLeft_)) + "s" : "Go to the helipad";
    }
    return "";
}
