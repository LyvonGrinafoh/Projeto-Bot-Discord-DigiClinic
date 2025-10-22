#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class DatabaseManager;
class ConfigManager;
class CommandHandler;
class EventHandler;

class SolicitacaoCommands {
private:
    dpp::cluster& bot_;
    DatabaseManager& db_;
    const BotConfig& config_;
    CommandHandler& cmdHandler_;
    EventHandler& eventHandler_;

public:
    SolicitacaoCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler, EventHandler& eventHandler);
    void handle_demanda_pedido(const dpp::slashcommand_t& event);
    void handle_finalizar_solicitacao(const dpp::slashcommand_t& event);
    void handle_limpar_demandas(const dpp::slashcommand_t& event);
    void handle_lista_demandas(const dpp::slashcommand_t& event);
    void handle_lembrete(const dpp::slashcommand_t& event);
    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id, const BotConfig& config);
};