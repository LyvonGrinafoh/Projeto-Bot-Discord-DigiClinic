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
#include <variant>

// Inicialização do ponteiro estático
dpp::cluster* BotCore::static_bot_ptr = nullptr;

// Handler para impedir fechamento acidental do console
BOOL WINAPI BotCore::ConsoleHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    {
        std::string msg = "Tentativa de fechar console bloqueada. Use 'stop' para sair.";
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
    aiHandler_(bot_),
    reportGenerator_(databaseManager_, bot_),
    running_(true)
{
    // Configura ícone da janela do console (opcional, visual)
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow != NULL) {
        HINSTANCE hInstance = GetModuleHandle(NULL);
        HANDLE hIconSmall = LoadImage(hInstance, MAKEINTRESOURCE(101), IMAGE_ICON, 16, 16, 0);
        if (hIconSmall) SendMessage(consoleWindow, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
        HANDLE hIconBig = LoadImage(hInstance, MAKEINTRESOURCE(101), IMAGE_ICON, 32, 32, 0);
        if (hIconBig) SendMessage(consoleWindow, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
    }

    Utils::log_to_file("--- Inicializando BotCore ---");

    // 1. Carregar Configuração
    if (!configManager_.load()) {
        throw std::runtime_error("Falha ao carregar configuracao. Encerrando.");
    }
    bot_.token = configManager_.getConfig().token;

    // 2. Carregar Banco de Dados e Conhecimento (Cérebro)
    if (!databaseManager_.loadAll()) {
        Utils::log_to_file("AVISO: Database incompleto ou iniciado do zero.");
    }

    // 3. Carregar Tickets
    if (!ticketManager_.loadTickets()) {
        Utils::log_to_file("AVISO: Tickets corrompido ou vazio.");
    }

    // 4. Inicializar Handlers
    // ATENÇÃO: Passando aiHandler_ para o EventHandler (para o bot ouvir menções)
    eventHandlerPtr_ = std::make_unique<EventHandler>(
        bot_,
        configManager_,
        ticketManager_,
        databaseManager_,
        aiHandler_
    );

    commandHandlerPtr_ = std::make_unique<CommandHandler>(
        bot_,
        databaseManager_,
        configManager_,
        reportGenerator_,
        *eventHandlerPtr_,
        ticketManager_,
        aiHandler_
    );

    // Configura Ctrl+C handler
    static_bot_ptr = &bot_;
    if (!SetConsoleCtrlHandler(BotCore::ConsoleHandler, TRUE)) {
        throw std::runtime_error("Falha ao registrar ConsoleHandler");
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

        if (commandHandlerPtr_) {
            commandHandlerPtr_->registerCommands();
        }
        });

    bot_.on_slashcommand([this](const dpp::slashcommand_t& event) {
        if (!commandHandlerPtr_) return;

        try {
            dpp::user author = event.command.get_issuing_user();
            dpp::channel* c = dpp::find_channel(event.command.channel_id);
            std::string channel_name = c ? c->name : std::to_string(event.command.channel_id);

            dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);
            std::string options_str;

            const dpp::attachment* anexo_log = nullptr;

            for (const auto& opt : cmd_data.options) {
                options_str += " `" + opt.name + "`";
                if (opt.type == dpp::co_attachment) {
                    dpp::snowflake att_id = std::get<dpp::snowflake>(opt.value);
                    anexo_log = &event.command.get_resolved_attachment(att_id);
                }
            }

            std::string log_message = "`[" + Utils::format_timestamp(event.command.id.get_creation_time()) + "]` `[COMANDO]` `(#" + channel_name + ")` `" + author.username + "`: /" + event.command.get_command_name() + options_str;

            if (anexo_log) {
                dpp::snowflake log_channel_id = configManager_.getConfig().canal_logs;
                std::string filename = anexo_log->filename;
                Utils::BaixarAnexo(&bot_, anexo_log->url, "", [this, log_channel_id, log_message, filename](bool sucesso, std::string conteudo) {
                    dpp::message msg_to_log(log_channel_id, log_message);
                    if (sucesso && !conteudo.empty()) {
                        msg_to_log.add_file(filename, conteudo);
                    }
                    bot_.message_create(msg_to_log);
                    });
            }
            else {
                bot_.message_create(dpp::message(configManager_.getConfig().canal_logs, log_message));
            }

            commandHandlerPtr_->handleInteraction(event);
        }
        catch (const std::exception& e) {
            Utils::log_to_file("ERRO DESCONHECIDO NO HANDLER: " + std::string(e.what()));
        }
        catch (...) {
            Utils::log_to_file("ERRO DESCONHECIDO NO HANDLER");
        }
        });

    bot_.on_message_create([this](const dpp::message_create_t& event) {
        if (eventHandlerPtr_) eventHandlerPtr_->onMessageCreate(event);
        });

    bot_.on_message_update([this](const dpp::message_update_t& event) {
        if (eventHandlerPtr_) eventHandlerPtr_->onMessageUpdate(event);
        });

    bot_.on_message_delete([this](const dpp::message_delete_t& event) {
        if (eventHandlerPtr_) eventHandlerPtr_->onMessageDelete(event);
        });

    bot_.on_message_reaction_add([this](const dpp::message_reaction_add_t& event) {
        if (eventHandlerPtr_) eventHandlerPtr_->onMessageReactionAdd(event);
        });

    bot_.on_button_click([this](const dpp::button_click_t& event) {
        if (eventHandlerPtr_) eventHandlerPtr_->onButtonClickListener(event);
        });

    bot_.on_form_submit([this](const dpp::form_submit_t& event) {
        if (eventHandlerPtr_) eventHandlerPtr_->onFormSubmitListener(event);
        });
}

void BotCore::setupReminderTimer() {
    bot_.start_timer([this](dpp::timer t) {
        enviarLembretes();
        }, 3600);
}

void BotCore::enviarLembretes() {
    std::time_t now_utc = std::time(nullptr);
    std::time_t now_brt_t = now_utc - 10800;
    std::tm* brt_tm = std::gmtime(&now_brt_t);

    if (!brt_tm || brt_tm->tm_hour != 9) {
        return;
    }

    Utils::log_to_file("Verificando visitas para amanha...");
    const BotConfig& cfg = configManager_.getConfig();
    const auto& visitas = databaseManager_.getVisitas();

    std::string msg_tatuape = "";
    std::string msg_campo_belo = "";

    for (const auto& [id, v] : visitas) {
        if (v.status == "agendada" && Utils::isDateTomorrow(v.data)) {
            std::string linha = "- " + v.horario + " | Dr(a) " + v.doutor + " (Resp: " + v.quem_marcou_nome + ")\n";
            if (v.unidade == "Tatuapé") msg_tatuape += linha;
            else if (v.unidade == "Campo Belo") msg_campo_belo += linha;
        }
    }

    if (!msg_tatuape.empty()) {
        std::string mencao = "";
        if (cfg.cargo_tatuape != 0) mencao += "<@&" + std::to_string(cfg.cargo_tatuape) + "> ";
        dpp::embed embed = dpp::embed().set_color(dpp::colors::gold).set_title("📅 Visitas Tatuapé - Amanhã").set_description(msg_tatuape);
        bot_.message_create(dpp::message(cfg.canal_visitas, mencao).add_embed(embed));
    }

    if (!msg_campo_belo.empty()) {
        std::string mencao = "";
        if (cfg.cargo_campo_belo != 0) mencao += "<@&" + std::to_string(cfg.cargo_campo_belo) + "> ";
        dpp::embed embed = dpp::embed().set_color(dpp::colors::gold).set_title("📅 Visitas Campo Belo - Amanhã").set_description(msg_campo_belo);
        bot_.message_create(dpp::message(cfg.canal_visitas, mencao).add_embed(embed));
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
    std::string msg = "Sinal " + std::to_string(signum) + " recebido.";
    std::cout << "\n" << msg << "\n> " << std::flush;
    Utils::log_to_file(msg);
}

void BotCore::consoleInputLoop(dpp::cluster* bot_cluster_ptr, std::atomic<bool>& running_flag) {
    std::string line;
    while (running_flag) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line) || line == "stop") {
            if (running_flag) {
                if (bot_cluster_ptr) bot_cluster_ptr->shutdown();
                running_flag = false;
            }
            break;
        }
    }
}