#pragma once
#include <dpp/dpp.h>
#include <memory>
#include <thread>
#include <atomic>
#include <windows.h>
#include "ConfigManager.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "EventHandler.h"
#include "ReportGenerator.h"
#include "TicketManager.h"
#include "AIHandler.h"

class BotCore {
private:
    ConfigManager configManager_;
    DatabaseManager databaseManager_;
    TicketManager ticketManager_;
    dpp::cluster bot_;
    AIHandler aiHandler_;
    ReportGenerator reportGenerator_;
    std::unique_ptr<EventHandler> eventHandlerPtr_;
    std::unique_ptr<CommandHandler> commandHandlerPtr_;
    std::thread inputThread_;
    std::atomic<bool> running_{ true };

    void setupEventHandlers();
    void setupReminderTimer();
    void enviarLembretes();
    static dpp::cluster* static_bot_ptr;
    static void consoleInputLoop(dpp::cluster* bot_cluster_ptr, std::atomic<bool>& running_flag);
    static BOOL WINAPI ConsoleHandler(DWORD fdwCtrlType);

public:
    BotCore();
    ~BotCore();
    void run();
    static void signalHandler(int signum);
};