#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class CommandHandler;

class PlacaCommands {
private:
    dpp::cluster& bot_;
    const BotConfig& config_;
    CommandHandler& cmdHandler_;

public:
    PlacaCommands(dpp::cluster& bot, const BotConfig& config, CommandHandler& handler);
    void handle_placa(const dpp::slashcommand_t& event);
    void handle_finalizar_placa(const dpp::slashcommand_t& event);
    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};