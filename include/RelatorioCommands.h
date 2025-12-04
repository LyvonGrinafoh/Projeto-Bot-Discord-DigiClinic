#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class DatabaseManager;
class CommandHandler;

class RelatorioCommands {
private:
    dpp::cluster& bot_;
    DatabaseManager& db_;
    const BotConfig& config_;
    CommandHandler& cmdHandler_;

public:
    RelatorioCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler);
    void handle_relatorio_do_dia(const dpp::slashcommand_t& event);
    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};