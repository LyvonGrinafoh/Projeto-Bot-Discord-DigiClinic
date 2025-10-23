#include "CommandHandler.h"
#include "DatabaseManager.h"
#include "ConfigManager.h"
#include "ReportGenerator.h"
#include "EventHandler.h"
#include "TicketManager.h"
#include "Utils.h"
#include <string>
#include <vector>

CommandHandler::CommandHandler(
    dpp::cluster& bot,
    DatabaseManager& db,
    ConfigManager& configMgr,
    ReportGenerator& rg,
    EventHandler& eventHandler,
    TicketManager& tm
) :
    bot_(bot),
    db_(db),
    configManager_(configMgr),
    reportGenerator_(rg),
    eventHandler_(eventHandler),
    ticketManager_(tm),
    visitasCmds_(bot, db, configManager_.getConfig(), *this, eventHandler),
    leadCmds_(bot, db, configManager_.getConfig(), *this, eventHandler),
    solicitacaoCmds_(bot, db, configManager_.getConfig(), *this, eventHandler),
    placaCmds_(bot, configManager_.getConfig(), *this),
    compraCmds_(bot, db, configManager_.getConfig(), *this),
    gerarPlanilhaCmd_(reportGenerator_),
    ticketCmds_(bot, tm, configManager_.getConfig(), *this)
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
        TicketCommands::addCommandDefinitions(command_list, bot_.me.id);
        bot_.guild_bulk_command_create(command_list, config_.server_id);
        Utils::log_to_file("Comandos slash registrados/atualizados para o servidor ID: " + std::to_string(config_.server_id));
    }
}

void CommandHandler::handleInteraction(const dpp::slashcommand_t& event) {
    std::string command_name = event.command.get_command_name();
    try {
        if (command_name == "visitas") { visitasCmds_.handle_visitas(event); }
        else if (command_name == "cancelar_visita") { visitasCmds_.handle_cancelar_visita(event); }
        else if (command_name == "lista_visitas") { visitasCmds_.handle_lista_visitas(event); }
        else if (command_name == "modificar_visita") { visitasCmds_.handle_modificar_visita(event); }
        else if (command_name == "adicionar_lead") { leadCmds_.handle_adicionar_lead(event); }
        else if (command_name == "modificar_lead") { leadCmds_.handle_modificar_lead(event); }
        else if (command_name == "listar_leads") { leadCmds_.handle_listar_leads(event); }
        else if (command_name == "ver_lead") { leadCmds_.handle_ver_lead(event); }
        else if (command_name == "limpar_leads") { leadCmds_.handle_limpar_leads(event); }
        else if (command_name == "deletar_lead") { leadCmds_.handle_deletar_lead(event); }
        else if (command_name == "demanda" || command_name == "pedido") { solicitacaoCmds_.handle_demanda_pedido(event); }
        else if (command_name == "finalizar_demanda" || command_name == "finalizar_pedido" || command_name == "cancelar_pedido" || command_name == "finalizar_lembrete") { solicitacaoCmds_.handle_finalizar_solicitacao(event); }
        else if (command_name == "limpar_demandas") { solicitacaoCmds_.handle_limpar_demandas(event); }
        else if (command_name == "lista_demandas") { solicitacaoCmds_.handle_lista_demandas(event); }
        else if (command_name == "lembrete") { solicitacaoCmds_.handle_lembrete(event); }
        else if (command_name == "placa") { placaCmds_.handle_placa(event); }
        else if (command_name == "finalizar_placa") { placaCmds_.handle_finalizar_placa(event); }
        else if (command_name == "adicionar_compra") { compraCmds_.handle_adicionar_compra(event); }
        else if (command_name == "gerar_planilha") { gerarPlanilhaCmd_.handle_gerar_planilha(event); }
        else if (command_name == "chamar") { ticketCmds_.handle_chamar(event); }
        else if (command_name == "finalizar_ticket") { ticketCmds_.handle_finalizar_ticket(event); }
        else { event.reply(dpp::message("Erro interno: O comando '/" + command_name + "' não foi reconhecido pelo handler.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("AVISO: Comando nao roteado recebido: /" + command_name); }
    }
    catch (const std::exception& e) { try { event.reply(dpp::message("❌ Ocorreu um erro inesperado ao processar o comando: " + std::string(e.what())).set_flags(dpp::m_ephemeral)); } catch (...) {} Utils::log_to_file("ERRO CRITICO no handleInteraction para /" + command_name + ": " + e.what()); }
    catch (...) { try { event.reply(dpp::message("❌ Ocorreu um erro desconhecido e grave ao processar o comando.").set_flags(dpp::m_ephemeral)); } catch (...) {} Utils::log_to_file("ERRO CRITICO no handleInteraction para /" + command_name + ": Erro desconhecido."); }
}

void CommandHandler::replyAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds) {
    if (msg.flags & dpp::m_ephemeral) { event.reply(msg); return; }
    std::string interaction_token = event.command.token; dpp::slashcommand_t event_copy = event;
    event.reply(msg, [this, event_copy, delay_seconds](const dpp::confirmation_callback_t& cb) {
        if (!cb.is_error()) {
            bot_.start_timer([this, event_copy](dpp::timer timer_handle) mutable {
                event_copy.delete_original_response([](const dpp::confirmation_callback_t& delete_cb) { if (delete_cb.is_error() && delete_cb.get_error().code != 10062) {} });
                bot_.stop_timer(timer_handle);
                }, delay_seconds);
        }
        else { Utils::log_to_file("Falha ao enviar reply (deleção não agendada): " + cb.get_error().message); }
        });
}

void CommandHandler::editAndDelete(const dpp::slashcommand_t& event, const dpp::message& msg, int delay_seconds) {
    if (msg.flags & dpp::m_ephemeral) { event.edit_original_response(msg); return; }
    std::string interaction_token = event.command.token; dpp::slashcommand_t event_copy = event;
    event.edit_original_response(msg, [this, event_copy, delay_seconds](const dpp::confirmation_callback_t& cb) {
        if (!cb.is_error()) {
            bot_.start_timer([this, event_copy](dpp::timer timer_handle) mutable {
                event_copy.delete_original_response([](const dpp::confirmation_callback_t& delete_cb) { if (delete_cb.is_error() && delete_cb.get_error().code != 10062) {} });
                bot_.stop_timer(timer_handle);
                }, delay_seconds);
        }
        else { Utils::log_to_file("Falha ao editar resposta original (deleção não agendada): " + cb.get_error().message); }
        });
}