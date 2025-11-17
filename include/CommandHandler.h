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

class DatabaseManager;
class ConfigManager;
class ReportGenerator;
class EventHandler;
class TicketManager;

class CommandHandler {
private:
    dpp::cluster& bot_;
    DatabaseManager& db_;
    ConfigManager& configManager_;
    ReportGenerator& reportGenerator_;
    EventHandler& eventHandler_;
    TicketManager& ticketManager_;

    VisitasCommands visitasCmds_;
    LeadCommands leadCmds_;
    SolicitacaoCommands solicitacaoCmds_;
    PlacaCommands placaCmds_;
    CompraCommands compraCmds_;
    GerarPlanilhaCommand gerarPlanilhaCmd_;
    TicketCommands ticketCmds_;
    EstoqueCommands estoqueCmds_;
    TodoCommands todoCmds_;

    void replyAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds = 10);
    void editAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds = 10);

public:
    CommandHandler(
        dpp::cluster& bot,
        DatabaseManager& db,
        ConfigManager& configMgr,
        ReportGenerator& rg,
        EventHandler& eventHandler,
        TicketManager& tm
    );

    void registerCommands();
    void handleInteraction(const dpp::slashcommand_t& event);

    friend class VisitasCommands;
    friend class LeadCommands;
    friend class SolicitacaoCommands;
    friend class PlacaCommands;
    friend class CompraCommands;
    friend class TicketCommands;
    friend class EstoqueCommands;
    friend class TodoCommands;
};