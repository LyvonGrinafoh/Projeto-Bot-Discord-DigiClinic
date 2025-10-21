#pragma once
#include <dpp/dpp.h>
#include "DataTypes.h"

class ConfigManager;

class EventHandler {
private:
    dpp::cluster& bot_;
    ConfigManager& configManager_;

public:
    EventHandler(dpp::cluster& bot, ConfigManager& configMgr);
    void onMessageCreate(const dpp::message_create_t& event);
    void onMessageUpdate(const dpp::message_update_t& event);
    void onMessageDelete(const dpp::message_delete_t& event);
    void onMessageReactionAdd(const dpp::message_reaction_add_t& event);
};