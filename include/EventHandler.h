#pragma once
#include <dpp/dpp.h>
#include <map>
#include <deque>
#include <mutex>
#include "DataTypes.h"
#include "AIHandler.h"

class ConfigManager;
class TicketManager;
class DatabaseManager;

class EventHandler {
private:
    dpp::cluster& bot_;
    ConfigManager& configManager_;
    TicketManager& ticketManager_;
    DatabaseManager& db_;
    AIHandler& ai_;

    std::map<dpp::snowflake, PaginationState> active_paginations_;
    std::mutex pagination_mutex_;

    std::map<dpp::snowflake, std::deque<std::string>> memory_buffer_;
    void updateMemory(dpp::snowflake channel_id, const std::string& user, const std::string& content);
    std::string getMemoryContext(dpp::snowflake channel_id);

public:
    EventHandler(dpp::cluster& bot, ConfigManager& configMgr, TicketManager& ticketMgr, DatabaseManager& db, AIHandler& ai);

    void onMessageCreate(const dpp::message_create_t& event);
    void onMessageUpdate(const dpp::message_update_t& event);
    void onMessageDelete(const dpp::message_delete_t& event);
    void onMessageReactionAdd(const dpp::message_reaction_add_t& event);
    void onButtonClickListener(const dpp::button_click_t& event);
    void onFormSubmitListener(const dpp::form_submit_t& event);

    void addPaginationState(dpp::snowflake message_id, PaginationState state);
};