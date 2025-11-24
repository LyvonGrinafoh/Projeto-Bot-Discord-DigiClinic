#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class CommandHandler;

class TodoCommands {
private:
    dpp::cluster& bot_;
    const BotConfig& config_;
    CommandHandler& cmdHandler_;
    const dpp::snowflake seu_id_discord = 187910708674035713;

public:
    TodoCommands(dpp::cluster& bot, const BotConfig& config, CommandHandler& handler);

    void handle_todo(const dpp::slashcommand_t& event);
    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};