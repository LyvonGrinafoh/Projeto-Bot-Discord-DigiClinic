#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class ReportGenerator;
// No CommandHandler needed here

class GerarPlanilhaCommand {
private:
    ReportGenerator& reportGenerator_;

public:
    GerarPlanilhaCommand(ReportGenerator& rg); // Constructor without CommandHandler
    void handle_gerar_planilha(const dpp::slashcommand_t& event);
    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};