#pragma once
#include <vector>
#include <dpp/dpp.h>
#include "DataTypes.h"

class ReportGenerator;

class GerarPlanilhaCommand {
private:
    ReportGenerator& reportGenerator_;

public:
    GerarPlanilhaCommand(ReportGenerator& rg);
    void handle_gerar_planilha(const dpp::slashcommand_t& event);
    static void addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id);
};