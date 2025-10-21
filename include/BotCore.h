#pragma once
#include <dpp/dpp.h>
#include <memory>
#include <thread>   // Add thread include
#include <atomic>   // Add atomic include
#include "ConfigManager.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "EventHandler.h"
#include "ReportGenerator.h"

class BotCore {
private:
    ConfigManager configManager_;
    DatabaseManager databaseManager_;
    dpp::cluster bot_;
    ReportGenerator reportGenerator_;
    std::unique_ptr<CommandHandler> commandHandlerPtr_;
    std::unique_ptr<EventHandler> eventHandlerPtr_;

    std::thread inputThread_; // Add thread member
    std::atomic<bool> running_{ true }; // Add running flag member

    void setupEventHandlers();
    void setupReminderTimer();
    void enviarLembretes();
    static dpp::cluster* static_bot_ptr;

    // Declare the input loop function (can be static or free)
    static void consoleInputLoop(dpp::cluster* bot_cluster_ptr, std::atomic<bool>& running_flag);

public:
    BotCore();
    ~BotCore();
    void run();
    static void signalHandler(int signum);
};