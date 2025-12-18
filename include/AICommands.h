#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class AIHandler;
class DatabaseManager;
class CommandHandler;

class AICommands {
private:
    dpp::cluster& bot_;
    AIHandler& ai_;
    DatabaseManager& db_;
    CommandHandler& cmdHandler_;

public:
    AICommands(dpp::cluster& bot, AIHandler& ai, DatabaseManager& db, CommandHandler& handler);

    void handle_analise_leads(const dpp::slashcommand_t& event);
    void handle_melhor_funcionario(const dpp::slashcommand_t& event);

    void handle_ensinar(const dpp::slashcommand_t& event);

    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};