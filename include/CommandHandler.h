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
#include "TicketCommands.h"
#include "EstoqueCommands.h"
#include "TodoCommands.h"
#include "RelatorioCommands.h"
#include "AICommands.h"

class DatabaseManager;
class ConfigManager;
class ReportGenerator;
class EventHandler;
class TicketManager;
class AIHandler;

class CommandHandler {
private:
    dpp::cluster& bot_;
    DatabaseManager& db_;
    ConfigManager& configManager_;
    ReportGenerator& reportGenerator_;
    EventHandler& eventHandler_;
    TicketManager& ticketManager_;
    AIHandler& aiHandler_;

    VisitasCommands visitasCmds_;
    LeadCommands leadCmds_;
    SolicitacaoCommands solicitacaoCmds_;
    PlacaCommands placaCmds_;
    CompraCommands compraCmds_;
    GerarPlanilhaCommand gerarPlanilhaCmd_;
    TicketCommands ticketCmds_;
    EstoqueCommands estoqueCmds_;
    TodoCommands todoCmds_;
    RelatorioCommands relatorioCmds_;
    AICommands aiCmds_;

public:
    CommandHandler(
        dpp::cluster& bot,
        DatabaseManager& db,
        ConfigManager& configMgr,
        ReportGenerator& rg,
        EventHandler& eventHandler,
        TicketManager& tm,
        AIHandler& ai
    );

    void registerCommands();
    void handleInteraction(const dpp::slashcommand_t& event);
    void replyAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds = 10);
    void editAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds = 10);
};