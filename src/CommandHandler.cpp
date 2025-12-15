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
    const BotConfig& config_ = configManager_.getConfig();
    if (dpp::run_once<struct register_bot_commands>()) {
        std::vector<dpp::slashcommand> command_list;
        VisitasCommands::addCommandDefinitions(command_list, bot_.me.id);
        LeadCommands::addCommandDefinitions(command_list, bot_.me.id);
        SolicitacaoCommands::addCommandDefinitions(command_list, bot_.me.id, config_);
        PlacaCommands::addCommandDefinitions(command_list, bot_.me.id);
        CompraCommands::addCommandDefinitions(command_list, bot_.me.id);
        GerarPlanilhaCommand::addCommandDefinitions(command_list, bot_.me.id);
        TicketCommands::addCommandDefinitions(command_list, bot_.me.id, config_);
        EstoqueCommands::addCommandDefinitions(command_list, bot_.me.id);
        TodoCommands::addCommandDefinitions(command_list, bot_.me.id);
        RelatorioCommands::addCommandDefinitions(command_list, bot_.me.id);

        AICommands::addCommandDefinitions(command_list, bot_.me.id);

        bot_.guild_bulk_command_create(command_list, config_.server_id);
        Utils::log_to_file("Comandos slash atualizados.");
    }
}

void CommandHandler::handleInteraction(const dpp::slashcommand_t& event) {
    std::string command_name = event.command.get_command_name();
    try {
        if (command_name == "visitas") { visitasCmds_.handle_visitas(event); }
        else if (command_name == "cancelar_visita") { visitasCmds_.handle_cancelar_visita(event); }
        else if (command_name == "finalizar_visita") { visitasCmds_.handle_finalizar_visita(event); }
        else if (command_name == "lista_visitas") { visitasCmds_.handle_lista_visitas(event); }
        else if (command_name == "modificar_visita") { visitasCmds_.handle_modificar_visita(event); }

        else if (command_name == "adicionar_lead") { leadCmds_.handle_adicionar_lead(event); }
        else if (command_name == "modificar_lead") { leadCmds_.handle_modificar_lead(event); }
        else if (command_name == "listar_leads") { leadCmds_.handle_listar_leads(event); }
        else if (command_name == "ver_lead") { leadCmds_.handle_ver_lead(event); }
        else if (command_name == "limpar_leads") { leadCmds_.handle_limpar_leads(event); }
        else if (command_name == "deletar_lead") { leadCmds_.handle_deletar_lead(event); }

        else if (command_name == "demanda" || command_name == "pedido") { solicitacaoCmds_.handle_demanda_pedido(event); }
        else if (command_name == "finalizar_demanda" || command_name == "finalizar_pedido") { solicitacaoCmds_.handle_finalizar_solicitacao_form(event); }
        else if (command_name == "ver_demanda") { solicitacaoCmds_.handle_ver_demanda(event); }
        else if (command_name == "cancelar_demanda" || command_name == "cancelar_pedido") { solicitacaoCmds_.handle_cancelar_demanda(event); }
        else if (command_name == "limpar_demandas") { solicitacaoCmds_.handle_limpar_demandas(event); }
        else if (command_name == "lista_demandas") { solicitacaoCmds_.handle_lista_demandas(event); }
        else if (command_name == "lembrete") { solicitacaoCmds_.handle_lembrete(event); }
        else if (command_name == "finalizar_lembrete") { solicitacaoCmds_.handle_finalizar_solicitacao_form(event); }

        else if (command_name == "placa") { placaCmds_.handle_placa(event); }
        else if (command_name == "finalizar_placa") { placaCmds_.handle_finalizar_placa(event); }
        else if (command_name == "lista_placas") { placaCmds_.handle_lista_placas(event); }

        else if (command_name == "registrar_gasto") { compraCmds_.handle_adicionar_compra(event); }
        else if (command_name == "gerar_planilha") { gerarPlanilhaCmd_.handle_gerar_planilha(event); }

        else if (command_name == "chamar") { ticketCmds_.handle_chamar(event); }
        else if (command_name == "finalizar_ticket") { ticketCmds_.handle_finalizar_ticket(event); }
        else if (command_name == "ver_log") { ticketCmds_.handle_ver_log(event); }

        else if (command_name == "estoque_criar") { estoqueCmds_.handle_estoque_criar(event); }
        else if (command_name == "estoque_repor") { estoqueCmds_.handle_estoque_add(event); }
        else if (command_name == "estoque_baixa") { estoqueCmds_.handle_estoque_remove(event); }
        else if (command_name == "estoque_lista") { estoqueCmds_.handle_estoque_lista(event); }
        else if (command_name == "estoque_delete") { estoqueCmds_.handle_estoque_delete(event); }
        else if (command_name == "estoque_modificar") { estoqueCmds_.handle_estoque_modificar(event); }

        else if (command_name == "todo") { todoCmds_.handle_todo(event); }
        else if (command_name == "relatorio_do_dia") { relatorioCmds_.handle_relatorio_do_dia(event); }

        else if (command_name == "ia") { aiCmds_.handle_ia_duvida(event); }
        else if (command_name == "analise_leads") { aiCmds_.handle_analise_leads(event); }
        else if (command_name == "melhor_funcionario") { aiCmds_.handle_melhor_funcionario(event); }

        else { event.reply(dpp::message("Comando desconhecido.").set_flags(dpp::m_ephemeral)); }
    }
    catch (const std::exception& e) { Utils::log_to_file("ERRO CRITICO handleInteraction: " + std::string(e.what())); }
    catch (...) { Utils::log_to_file("ERRO CRITICO DESCONHECIDO handleInteraction"); }
}

void CommandHandler::replyAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds) {
    if (msg.flags & dpp::m_ephemeral) { event.reply(msg); return; }

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