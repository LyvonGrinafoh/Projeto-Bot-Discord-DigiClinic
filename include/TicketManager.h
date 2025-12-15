#pragma once
#include <dpp/dpp.h>
#include <string>
#include <map>
#include <optional>
#include <mutex>
#include <random>
#include "DataTypes.h"

class TicketManager {
private:
    std::map<uint64_t, Ticket> tickets_;
    std::map<uint64_t, std::string> ticket_logs_;
    std::mutex ticket_mutex_;
    std::mt19937 random_engine_;

    uint64_t generateTicketID();
    bool saveTickets();

public:
    TicketManager();

    bool loadTickets();

    Ticket createTicket(dpp::snowflake user_a, dpp::snowflake user_b, dpp::snowflake channel_id, const std::string& assunto);
    bool arquivarTicket(uint64_t ticket_id, const std::string& log_filename);

    std::optional<Ticket> findTicketByChannel(dpp::snowflake channel_id);
    std::optional<Ticket> findTicketById(uint64_t ticket_id);

    void appendToLog(const dpp::message& msg);
    std::string getAndRemoveLog(dpp::snowflake channel_id);
};