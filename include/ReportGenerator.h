#pragma once
#include <string>
#include <dpp/dpp.h>

class DatabaseManager;

class ReportGenerator {
private:
    DatabaseManager& db_;
    dpp::cluster& bot_; // Needs bot_ cluster to schedule timer

public:
    ReportGenerator(DatabaseManager& db, dpp::cluster& bot); // Constructor takes bot_
    void gerarPlanilhaLeads(const dpp::slashcommand_t& event);
    void gerarPlanilhaCompras(const dpp::slashcommand_t& event);
    void gerarPlanilhaVisitas(const dpp::slashcommand_t& event);
};