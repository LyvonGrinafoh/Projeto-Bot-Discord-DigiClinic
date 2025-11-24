#pragma once
#include <dpp/dpp.h>
#include <map>
#include <mutex>
#include "DataTypes.h"

class ConfigManager;
class TicketManager;

class EventHandler {
private:
    dpp::cluster& bot_;
    ConfigManager& configManager_;
    TicketManager& ticketManager_;

    std::map<dpp::snowflake, PaginationState> active_paginations_;
    std::mutex pagination_mutex_;

public:
    EventHandler(dpp::cluster& bot, ConfigManager& configMgr, TicketManager& ticketMgr);

    void onMessageCreate(const dpp::message_create_t& event);
    void onMessageUpdate(const dpp::message_update_t& event);
    void onMessageDelete(const dpp::message_delete_t& event);
    void onMessageReactionAdd(const dpp::message_reaction_add_t& event);

    void addPaginationState(dpp::snowflake message_id, PaginationState state);
};