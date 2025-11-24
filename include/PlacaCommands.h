#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class CommandHandler;
class DatabaseManager;
class EventHandler;

class PlacaCommands {
private:
    dpp::cluster& bot_;
    DatabaseManager& db_;
    const BotConfig& config_;
    CommandHandler& cmdHandler_;
    EventHandler& eventHandler_;

public:
    PlacaCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler, EventHandler& eventHandler);

    void handle_placa(const dpp::slashcommand_t& event);
    void handle_finalizar_placa(const dpp::slashcommand_t& event);
    void handle_lista_placas(const dpp::slashcommand_t& event);

    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};