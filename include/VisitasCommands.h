#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class DatabaseManager;
class CommandHandler;
class EventHandler;

class VisitasCommands {
private:
    dpp::cluster& bot_;
    DatabaseManager& db_;
    const BotConfig& config_;
    CommandHandler& cmdHandler_;
    EventHandler& eventHandler_;

public:
    VisitasCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler, EventHandler& eventHandler);
    void handle_visitas(const dpp::slashcommand_t& event);
    void handle_cancelar_visita(const dpp::slashcommand_t& event);
    void handle_finalizar_visita(const dpp::slashcommand_t& event); // <-- FUNÇÃO ADICIONADA
    void handle_lista_visitas(const dpp::slashcommand_t& event);
    void handle_modificar_visita(const dpp::slashcommand_t& event);
    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};