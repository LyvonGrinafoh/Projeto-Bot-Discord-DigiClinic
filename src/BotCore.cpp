#include "BotCore.h"
#include "Utils.h"
#include <iostream>
#include <stdexcept>
#include <csignal>
#include <ctime>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <string>

dpp::cluster* BotCore::static_bot_ptr = nullptr;

BOOL WINAPI BotCore::ConsoleHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    {
        std::string msg = "Tentativa de fechar console (sinal " + std::to_string(fdwCtrlType) + ") bloqueada. Use 'stop' para sair.";
        std::cout << "\n" << msg << "\n> " << std::flush;
        Utils::log_to_file(msg);
        return TRUE;
    }
    default:
        return FALSE;
    }
}

BotCore::BotCore() :
    configManager_(),
    databaseManager_(),
    ticketManager_(),
    bot_("", dpp::i_default_intents | dpp::i_guild_members | dpp::i_message_content),
    reportGenerator_(databaseManager_, bot_),
    running_(true)
{
    Utils::log_to_file("--- Inicializando BotCore ---");
    if (!configManager_.load()) { throw std::runtime_error("Falha ao carregar configuracao. Encerrando."); }
    bot_.token = configManager_.getConfig().token;
    if (!databaseManager_.loadAll()) { Utils::log_to_file("AVISO: Um ou mais arquivos de banco de dados podem estar corrompidos ou ausentes."); }
    if (!ticketManager_.loadTickets()) { Utils::log_to_file("AVISO: O arquivo de tickets pode estar corrompido."); }

    eventHandlerPtr_ = std::make_unique<EventHandler>(bot_, configManager_, ticketManager_);
    commandHandlerPtr_ = std::make_unique<CommandHandler>(bot_, databaseManager_, configManager_, reportGenerator_, *eventHandlerPtr_, ticketManager_);

    static_bot_ptr = &bot_;
    if (!SetConsoleCtrlHandler(BotCore::ConsoleHandler, TRUE)) {
        std::cerr << "ERRO FATAL: Nao foi possivel registrar o Console Control Handler." << std::endl;
        Utils::log_to_file("ERRO FATAL: Nao foi possivel registrar o Console Control Handler.");
        throw std::runtime_error("Falha ao registrar SetConsoleCtrlHandler");
    }
    else {
        Utils::log_to_file("Console Control Handler registrado com sucesso.");
    }

    std::signal(SIGINT, BotCore::signalHandler);
    std::signal(SIGTERM, BotCore::signalHandler);
    setupEventHandlers();
    setupReminderTimer();
}

BotCore::~BotCore() {
    running_ = false;
    if (inputThread_.joinable()) {
        inputThread_.detach();
    }
    Utils::log_to_file("--- BotCore finalizado ---");
}

void BotCore::setupEventHandlers() {
    bot_.on_log(dpp::utility::cout_logger());
    bot_.on_ready([this](const dpp::ready_t& event) {
        Utils::log_to_file("Bot conectado como: " + bot_.me.username);
        if (commandHandlerPtr_) commandHandlerPtr_->registerCommands();
        });
    bot_.on_slashcommand([this](const dpp::slashcommand_t& event) {
        if (!commandHandlerPtr_) return;
        try {
            dpp::user author = event.command.get_issuing_user(); dpp::channel* c = dpp::find_channel(event.command.channel_id); std::string channel_name = c ? c->name : std::to_string(event.command.channel_id); dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data); std::string options_str; for (const auto& opt : cmd_data.options) { options_str += " `" + opt.name + "`"; } std::string log_message = "`[" + Utils::format_timestamp(event.command.id.get_creation_time()) + "]` `[COMANDO]` `(#" + channel_name + ")` `" + author.username + "` `(" + std::to_string(author.id) + ")`: /" + event.command.get_command_name() + options_str; bot_.message_create(dpp::message(configManager_.getConfig().canal_logs, log_message));
            commandHandlerPtr_->handleInteraction(event);
        }
        catch (const dpp::exception& e) { Utils::log_to_file("ERRO DPP (on_slashcommand): " + std::string(e.what())); try { event.reply(dpp::message("Ocorreu um erro interno da biblioteca Discord++ ao processar este comando.").set_flags(dpp::m_ephemeral)); } catch (...) {} }
        catch (const std::exception& e) { Utils::log_to_file("ERRO STD (on_slashcommand): " + std::string(e.what())); try { event.reply(dpp::message("Ocorreu um erro padrão C++ ao processar este comando.").set_flags(dpp::m_ephemeral)); } catch (...) {} }
        catch (...) { Utils::log_to_file("ERRO DESCONHECIDO (on_slashcommand)"); try { event.reply(dpp::message("Ocorreu um erro desconhecido ao processar este comando.").set_flags(dpp::m_ephemeral)); } catch (...) {} }
        });
    bot_.on_message_create([this](const dpp::message_create_t& event) { if (eventHandlerPtr_) eventHandlerPtr_->onMessageCreate(event); });
    bot_.on_message_update([this](const dpp::message_update_t& event) { if (eventHandlerPtr_) eventHandlerPtr_->onMessageUpdate(event); });
    bot_.on_message_delete([this](const dpp::message_delete_t& event) { if (eventHandlerPtr_) eventHandlerPtr_->onMessageDelete(event); });
    bot_.on_message_reaction_add([this](const dpp::message_reaction_add_t& event) { if (eventHandlerPtr_) eventHandlerPtr_->onMessageReactionAdd(event); });
}

void BotCore::setupReminderTimer() { bot_.start_timer([this](dpp::timer t) { enviarLembretes(); }, 7200); }

void BotCore::enviarLembretes() {
    std::time_t now_utc = std::time(nullptr); std::time_t now_brt_t = now_utc - 10800; std::tm* brt_tm = std::gmtime(&now_brt_t);
    if (!brt_tm) { Utils::log_to_file("ERRO: gmtime retornou nullptr em enviarLembretes."); return; }
    int dia_semana = brt_tm->tm_wday; int hora = brt_tm->tm_hour; bool horario_comercial = false;
    if (dia_semana >= 1 && dia_semana <= 5) { if (hora >= 9 && hora < 18) { horario_comercial = true; } }
    else if (dia_semana == 6) { if (hora >= 9 && hora < 13) { horario_comercial = true; } }
    if (!horario_comercial) { return; }
    Utils::log_to_file("Dentro do horário comercial. Verificando lembretes...");
    std::map<dpp::snowflake, std::vector<Solicitacao>> solicitacoes_por_usuario; const auto& todas_solicitacoes = databaseManager_.getSolicitacoes();
    for (const auto& [id, solicitacao] : todas_solicitacoes) { if (solicitacao.tipo != PEDIDO) { solicitacoes_por_usuario[solicitacao.id_usuario_responsavel].push_back(solicitacao); } }
    for (const auto& [id_usuario, lista_solicitacoes] : solicitacoes_por_usuario) {
        if (id_usuario == 0) continue;
        if (!lista_solicitacoes.empty()) {
            std::string conteudo_dm = "Olá! 👋 Este é um lembrete das suas pendências ativas:\n\n";
            for (const auto& solicitacao : lista_solicitacoes) {
                std::string tipo_str = (solicitacao.tipo == DEMANDA) ? "Demanda" : "Lembrete";
                conteudo_dm += "**" + tipo_str + ":**\n";
                conteudo_dm += "**Código:** `" + std::to_string(solicitacao.id) + "`\n";
                conteudo_dm += "**Descrição:** " + solicitacao.texto + "\n";
                conteudo_dm += "**Prazo:** " + solicitacao.prazo + "\n\n";
            }
            conteudo_dm += "Para finalizar, use o comando correspondente no servidor (`/finalizar_demanda` ou `/finalizar_lembrete`).";
            dpp::message dm(conteudo_dm);
            if (conteudo_dm.length() < 1950) { bot_.direct_message_create(id_usuario, dm, [id_usuario](const dpp::confirmation_callback_t& callback) { if (callback.is_error()) { Utils::log_to_file("AVISO: Falha ao enviar DM de lembrete para usuario ID " + std::to_string(id_usuario) + ": " + callback.get_error().message); } }); }
            else { Utils::log_to_file("AVISO: DM de lembrete para usuario ID " + std::to_string(id_usuario) + " muito longa (" + std::to_string(conteudo_dm.length()) + " chars). Lembrete nao enviado."); bot_.direct_message_create(id_usuario, dpp::message("Seu resumo de pendências está muito longo para ser enviado via DM. Use `/lista_demandas @voce` no servidor."), [](const auto& callback) {}); }
        }
    }
}

void BotCore::run() {
    Utils::log_to_file("--- Iniciando D++ cluster ---");
    inputThread_ = std::thread(BotCore::consoleInputLoop, &bot_, std::ref(running_));
    bot_.start(dpp::st_wait);
    running_ = false;
    if (inputThread_.joinable()) {
        inputThread_.detach();
    }
}

void BotCore::signalHandler(int signum) {
    std::string msg = "Sinal " + std::to_string(signum) + " recebido (ex: Ctrl+C). Para desligar o bot, digite 'stop' e pressione Enter.";
    std::cout << "\n" << msg << "\n> " << std::flush;
    Utils::log_to_file(msg);
}

void BotCore::consoleInputLoop(dpp::cluster* bot_cluster_ptr, std::atomic<bool>& running_flag) {
    std::string line;
    Utils::log_to_file("Thread de input do console iniciada.");
    while (running_flag) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) {
            if (running_flag) {
                Utils::log_to_file("Console input stream fechado/erro. Desligando...");
                std::cerr << "\nConsole input stream fechado/erro. Desligando..." << std::endl;
                if (bot_cluster_ptr) {
                    bot_cluster_ptr->shutdown();
                }
                running_flag = false;
            }
            break;
        }

        if (line == "stop") {
            Utils::log_to_file("Comando 'stop' recebido via console. Iniciando desligamento...");
            std::cout << "Comando 'stop' recebido. Iniciando desligamento..." << std::endl;
            if (bot_cluster_ptr) {
                bot_cluster_ptr->shutdown();
            }
            running_flag = false;
            break;
        }
        else if (!line.empty()) {
            std::cout << "Comando desconhecido. Digite 'stop' para sair." << std::endl;
        }
    }
    Utils::log_to_file("Thread de input do console finalizada.");
}