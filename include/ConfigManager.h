#pragma once

#include <string>
#include <vector>
#include "DataTypes.h"

class ConfigManager {
private:
    BotConfig config_;
    bool loaded_ = false;

public:
    ConfigManager() = default;
    bool load(const std::string& filename = "config.json");
    const BotConfig& getConfig() const;
    bool isLoaded() const;
};