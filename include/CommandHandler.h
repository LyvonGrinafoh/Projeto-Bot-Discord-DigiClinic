#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <dpp/dpp.h>
#include "DataTypes.h"
#include "VisitasCommands.h"
#include "LeadCommands.h"
#include "SolicitacaoCommands.h"
#include "PlacaCommands.h"
#include "CompraCommands.h"
#include "GerarPlanilhaCommand.h"

class DatabaseManager;
class ConfigManager;
class ReportGenerator;

class CommandHandler {
private:
    dpp::cluster& bot_;
    DatabaseManager& db_;
    ConfigManager& configManager_;
    ReportGenerator& reportGenerator_;

    VisitasCommands visitasCmds_;
    LeadCommands leadCmds_;
    SolicitacaoCommands solicitacaoCmds_;
    PlacaCommands placaCmds_;
    CompraCommands compraCmds_;
    GerarPlanilhaCommand gerarPlanilhaCmd_;

    void replyAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds = 10);
    void editAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds = 10);

public:
    CommandHandler(
        dpp::cluster& bot,
        DatabaseManager& db,
        ConfigManager& configMgr,
        ReportGenerator& rg
    );

    void registerCommands();
    void handleInteraction(const dpp::slashcommand_t& event);

    friend class VisitasCommands;
    friend class LeadCommands;
    friend class SolicitacaoCommands;
    friend class PlacaCommands;
    friend class CompraCommands;
    // GerarPlanilhaCommand uses ReportGenerator now
};