#include "CommandHandler.h"
#include "DatabaseManager.h"
#include "ConfigManager.h"
#include "ReportGenerator.h"
#include "EventHandler.h"
#include "TicketManager.h"
#include "EstoqueCommands.h"
#include "TodoCommands.h"
#include "RelatorioCommands.h" 
#include "AIHandler.h"
#include "Utils.h"
#include <string>
#include <vector>

CommandHandler::CommandHandler(
    dpp::cluster& bot,
    DatabaseManager& db,
    ConfigManager& configMgr,
    ReportGenerator& rg,
    EventHandler& eventHandler,
    TicketManager& tm,
    AIHandler& ai
) :
    bot_(bot),
    db_(db),
    configManager_(configMgr),
    reportGenerator_(rg),
    eventHandler_(eventHandler),
    ticketManager_(tm),
    aiHandler_(ai),
    visitasCmds_(bot, db_, configManager_.getConfig(), *this, eventHandler),
    leadCmds_(bot, db_, configManager_.getConfig(), *this, eventHandler),
    solicitacaoCmds_(bot, db_, configManager_.getConfig(), *this, eventHandler),
    placaCmds_(bot, db_, configManager_.getConfig(), *this, eventHandler_),
    compraCmds_(bot, db_, configManager_.getConfig(), *this),
    gerarPlanilhaCmd_(reportGenerator_),
    ticketCmds_(bot, tm, configManager_.getConfig(), *this),
    estoqueCmds_(bot, db_, configManager_.getConfig(), *this),
    todoCmds_(bot, configManager_.getConfig(), *this),
    relatorioCmds_(bot, db_, configManager_.getConfig(), *this),
    aiCmds_(bot, ai, db_, *this)
{
}

void CommandHandler::registerCommands() {
    std::vector<dpp::slashcommand> commands;
    dpp::snowflake bot_id = bot_.me.id;

    VisitasCommands::addCommandDefinitions(commands, bot_id);
    LeadCommands::addCommandDefinitions(commands, bot_id);
    SolicitacaoCommands::addCommandDefinitions(commands, bot_id, configManager_.getConfig());
    PlacaCommands::addCommandDefinitions(commands, bot_id);
    CompraCommands::addCommandDefinitions(commands, bot_id);
    GerarPlanilhaCommand::addCommandDefinitions(commands, bot_id);
    TicketCommands::addCommandDefinitions(commands, bot_id, configManager_.getConfig());
    EstoqueCommands::addCommandDefinitions(commands, bot_id);
    TodoCommands::addCommandDefinitions(commands, bot_id);
    RelatorioCommands::addCommandDefinitions(commands, bot_id);
    AICommands::addCommandDefinitions(commands, bot_id);

    bot_.global_bulk_command_create(commands);

    dpp::snowflake guild_id = configManager_.getConfig().server_id;
    if (guild_id != 0) {
        bot_.guild_bulk_command_create({}, guild_id);
    }
}

void CommandHandler::handleInteraction(const dpp::slashcommand_t& event) {
    std::string name = event.command.get_command_name();

    if (name == "visitas" || name == "cancelar_visita" || name == "finalizar_visita" || name == "lista_visitas" || name == "modificar_visita") {
        if (name == "visitas") visitasCmds_.handle_visitas(event);
        else if (name == "cancelar_visita") visitasCmds_.handle_cancelar_visita(event);
        else if (name == "finalizar_visita") visitasCmds_.handle_finalizar_visita(event);
        else if (name == "lista_visitas") visitasCmds_.handle_lista_visitas(event);
        else if (name == "modificar_visita") visitasCmds_.handle_modificar_visita(event);
    }
    else if (name == "adicionar_lead" || name == "modificar_lead" || name == "listar_leads" || name == "ver_lead" || name == "limpar_leads" || name == "deletar_lead") {
        if (name == "adicionar_lead") leadCmds_.handle_adicionar_lead(event);
        else if (name == "modificar_lead") leadCmds_.handle_modificar_lead(event);
        else if (name == "listar_leads") leadCmds_.handle_listar_leads(event);
        else if (name == "ver_lead") leadCmds_.handle_ver_lead(event);
        else if (name == "limpar_leads") leadCmds_.handle_limpar_leads(event);
        else if (name == "deletar_lead") leadCmds_.handle_deletar_lead(event);
    }
    else if (name == "demanda" || name == "pedido" || name == "finalizar_solicitacao" || name == "cancelar_demanda" || name == "limpar_demandas" || name == "lista_demandas" || name == "cancelar_pedido" || name == "ver_demanda" || name == "lembrete") {
        if (name == "finalizar_solicitacao") solicitacaoCmds_.handle_finalizar_solicitacao_form(event);
        else if (name == "cancelar_demanda" || name == "cancelar_pedido") solicitacaoCmds_.handle_cancelar_demanda(event);
        else if (name == "limpar_demandas") solicitacaoCmds_.handle_limpar_demandas(event);
        else if (name == "lista_demandas") solicitacaoCmds_.handle_lista_demandas(event);
        else if (name == "ver_demanda") solicitacaoCmds_.handle_ver_demanda(event);
        else if (name == "lembrete") solicitacaoCmds_.handle_lembrete(event);
        else solicitacaoCmds_.handle_demanda_pedido(event);
    }
    else if (name == "placa" || name == "finalizar_placa" || name == "lista_placas") {
        if (name == "placa") placaCmds_.handle_placa(event);
        else if (name == "finalizar_placa") placaCmds_.handle_finalizar_placa(event);
        else if (name == "lista_placas") placaCmds_.handle_lista_placas(event);
    }
    else if (name == "registrar_gasto") {
        compraCmds_.handle_adicionar_compra(event);
    }
    else if (name == "gerar_planilha") {
        gerarPlanilhaCmd_.handle_gerar_planilha(event);
    }
    else if (name == "chamar" || name == "finalizar_ticket" || name == "ver_log") {
        if (name == "chamar") ticketCmds_.handle_chamar(event);
        else if (name == "finalizar_ticket") ticketCmds_.handle_finalizar_ticket(event);
        else if (name == "ver_log") ticketCmds_.handle_ver_log(event);
    }
    else if (name.rfind("estoque", 0) == 0) {
        if (name == "estoque_criar") estoqueCmds_.handle_estoque_criar(event);
        else if (name == "estoque_repor") estoqueCmds_.handle_estoque_add(event);
        else if (name == "estoque_baixa") estoqueCmds_.handle_estoque_remove(event);
        else if (name == "estoque_lista") estoqueCmds_.handle_estoque_lista(event);
        else if (name == "estoque_delete") estoqueCmds_.handle_estoque_delete(event);
        else if (name == "estoque_modificar") estoqueCmds_.handle_estoque_modificar(event);
    }
    else if (name == "todo") {
        todoCmds_.handle_todo(event);
    }
    else if (name == "relatorio_do_dia") {
        relatorioCmds_.handle_relatorio_do_dia(event);
    }
    else if (name == "analise_leads") {
        aiCmds_.handle_analise_leads(event);
    }
    else if (name == "melhor_funcionario") {
        aiCmds_.handle_melhor_funcionario(event);
    }
    else if (name == "ensinar") {
        aiCmds_.handle_ensinar(event);
    }
}

void CommandHandler::replyAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds) {
    if (msg.flags & dpp::m_ephemeral) {
        event.reply(msg);
        return;
    }

    int final_delay = (delay_seconds == 10) ? 60 : delay_seconds;

    event.reply(msg, [this, event, final_delay](const dpp::confirmation_callback_t& cb) {
        if (!cb.is_error()) {
            event.get_original_response([this, final_delay, event](const dpp::confirmation_callback_t& msg_cb) {
                if (!msg_cb.is_error()) {
                    auto m = std::get<dpp::message>(msg_cb.value);
                    bot_.message_add_reaction(m.id, m.channel_id, "🗑️");
                    bot_.start_timer([this, event](dpp::timer timer_handle) mutable {
                        event.delete_original_response([](auto) {});
                        bot_.stop_timer(timer_handle);
                        }, final_delay);
                }
                });
        }
        });
}

void CommandHandler::editAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds) {
    if (msg.flags & dpp::m_ephemeral) { event.edit_original_response(msg); return; }

    int final_delay = (delay_seconds == 10) ? 60 : delay_seconds;

    event.edit_original_response(msg, [this, event, final_delay](const dpp::confirmation_callback_t& cb) {
        if (!cb.is_error()) {
            event.get_original_response([this, final_delay, event](const dpp::confirmation_callback_t& msg_cb) {
                if (!msg_cb.is_error()) {
                    auto m = std::get<dpp::message>(msg_cb.value);
                    bot_.message_add_reaction(m.id, m.channel_id, "🗑️");
                    bot_.start_timer([this, event](dpp::timer timer_handle) mutable {
                        event.delete_original_response([](auto) {});
                        bot_.stop_timer(timer_handle);
                        }, final_delay);
                }
                });
        }
        });
}