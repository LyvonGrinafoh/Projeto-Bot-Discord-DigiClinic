#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class DatabaseManager;
class CommandHandler;

class EstoqueCommands {
private:
    dpp::cluster& bot_;
    DatabaseManager& db_;
    const BotConfig& config_;
    CommandHandler& cmdHandler_;
    std::string formatarItemEstoque(const EstoqueItem& item);

public:
    EstoqueCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler);
    void handle_estoque_criar(const dpp::slashcommand_t& event);
    void handle_estoque_add(const dpp::slashcommand_t& event);
    void handle_estoque_remove(const dpp::slashcommand_t& event);
    void handle_estoque_lista(const dpp::slashcommand_t& event);
    void handle_estoque_delete(const dpp::slashcommand_t& event);
    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};