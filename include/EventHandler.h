#pragma once
#include <dpp/dpp.h>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <mutex>
#include "DataTypes.h"

class ConfigManager;
class TicketManager;

using PaginatedItem = std::variant<Lead, Visita, Solicitacao>;

struct PaginationState {
    dpp::snowflake channel_id;
    std::vector<PaginatedItem> items;
    int currentPage = 1;
    int itemsPerPage = 5;
    dpp::snowflake originalUserID;
    std::string listType;
};

class EventHandler {
private:
    dpp::cluster& bot_;
    ConfigManager& configManager_;
    TicketManager& ticketManager_;
    std::map<dpp::snowflake, PaginationState> activePaginations_;
    std::mutex paginationMutex_;

public:
    EventHandler(dpp::cluster& bot, ConfigManager& configMgr, TicketManager& tm);
    void onMessageCreate(const dpp::message_create_t& event);
    void onMessageUpdate(const dpp::message_update_t& event);
    void onMessageDelete(const dpp::message_delete_t& event);
    void onMessageReactionAdd(const dpp::message_reaction_add_t& event);
    void removePaginationState(dpp::snowflake messageId);
    void addPaginationState(dpp::snowflake messageId, PaginationState state);
};