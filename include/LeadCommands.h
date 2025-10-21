#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class DatabaseManager;
class CommandHandler;

class LeadCommands {
private:
    dpp::cluster& bot_;
    DatabaseManager& db_;
    const BotConfig& config_;
    CommandHandler& cmdHandler_;

public:
    LeadCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler);
    void handle_adicionar_lead(const dpp::slashcommand_t& event);
    void handle_modificar_lead(const dpp::slashcommand_t& event);
    void handle_listar_leads(const dpp::slashcommand_t& event);
    void handle_ver_lead(const dpp::slashcommand_t& event);
    void handle_limpar_leads(const dpp::slashcommand_t& event);
    void handle_deletar_lead(const dpp::slashcommand_t& event);
    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};