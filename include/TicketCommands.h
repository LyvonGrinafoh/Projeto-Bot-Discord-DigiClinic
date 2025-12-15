#pragma once
#include <dpp/dpp.h>
#include <vector>
#include "DataTypes.h"

class TicketManager;
class CommandHandler;

class TicketCommands {
private:
    dpp::cluster& bot_;
    TicketManager& ticketManager_;
    const BotConfig& config_;
    CommandHandler& cmdHandler_;

public:
    TicketCommands(dpp::cluster& bot, TicketManager& tm, const BotConfig& config, CommandHandler& handler);

    void handle_chamar(const dpp::slashcommand_t& event);
    void handle_finalizar_ticket(const dpp::slashcommand_t& event);
    void handle_ver_log(const dpp::slashcommand_t& event);

    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id, const BotConfig& config);
};