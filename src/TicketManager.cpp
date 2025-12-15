#include "TicketManager.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <chrono>
#include <string>
#include "DataTypes.h" 
#include "Utils.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static void safe_get_snowflake(const json& j, const std::string& key, uint64_t& target) {
    if (j.contains(key)) {
        const auto& value = j.at(key);
        if (value.is_number()) { value.get_to(target); }
        else if (value.is_string()) {
            try { target = std::stoull(value.get<std::string>()); }
            catch (...) { target = 0; }
        }
        else { target = 0; }
    }
    else { target = 0; }
}

TicketManager::TicketManager() : random_engine_(std::random_device{}()) {
    loadTickets();
}

bool TicketManager::loadTickets() {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    std::ifstream file(TICKETS_DATABASE_FILE);
    if (!file.is_open()) {
        tickets_.clear();
        return true;
    }
    try {
        json j;
        file >> j;
        if (!j.is_null() && !j.empty()) {
            tickets_.clear();
            for (auto& element : j.items()) {
                uint64_t id = std::stoull(element.key());
                json val = element.value();
                Ticket t;
                safe_get_snowflake(val, "ticket_id", t.ticket_id);
                safe_get_snowflake(val, "channel_id", t.channel_id);
                safe_get_snowflake(val, "user_a_id", t.user_a_id);
                safe_get_snowflake(val, "user_b_id", t.user_b_id);
                if (val.contains("status")) val.at("status").get_to(t.status); else t.status = "aberto";
                if (val.contains("log_filename")) val.at("log_filename").get_to(t.log_filename); else t.log_filename = "";
                if (val.contains("nome_ticket")) val.at("nome_ticket").get_to(t.nome_ticket); else t.nome_ticket = "Sem Assunto";

                tickets_[id] = t;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Erro TicketManager: " << e.what() << std::endl;
        tickets_.clear();
        return false;
    }
    return true;
}

bool TicketManager::saveTickets() {
    try {
        std::ofstream file(TICKETS_DATABASE_FILE);
        if (!file.is_open()) return false;

        json j_map = json::object();
        for (const auto& [id, t] : tickets_) {
            json j_t;
            j_t["ticket_id"] = t.ticket_id;
            j_t["channel_id"] = t.channel_id;
            j_t["user_a_id"] = t.user_a_id;
            j_t["user_b_id"] = t.user_b_id;
            j_t["status"] = t.status;
            j_t["log_filename"] = t.log_filename;
            j_t["nome_ticket"] = t.nome_ticket;
            j_map[std::to_string(id)] = j_t;
        }
        file << j_map.dump(4);
    }
    catch (...) { return false; }
    return true;
}

uint64_t TicketManager::generateTicketID() {
    std::uniform_int_distribution<uint64_t> dist(1000, 999999);
    uint64_t id = 0;
    do { id = dist(random_engine_); } while (tickets_.count(id) > 0 || id == 0);
    return id;
}

Ticket TicketManager::createTicket(dpp::snowflake user_a, dpp::snowflake user_b, dpp::snowflake channel_id, const std::string& assunto) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    Ticket new_ticket;
    new_ticket.ticket_id = generateTicketID();
    new_ticket.channel_id = channel_id;
    new_ticket.user_a_id = user_a;
    new_ticket.user_b_id = user_b;
    new_ticket.status = "aberto";
    new_ticket.log_filename = "";
    new_ticket.nome_ticket = assunto;

    tickets_[new_ticket.ticket_id] = new_ticket;
    saveTickets();
    return new_ticket;
}

bool TicketManager::arquivarTicket(uint64_t ticket_id, const std::string& log_filename) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    auto it = tickets_.find(ticket_id);
    if (it != tickets_.end()) {
        it->second.status = "fechado";
        it->second.log_filename = log_filename;
        return saveTickets();
    }
    return false;
}

std::optional<Ticket> TicketManager::findTicketByChannel(dpp::snowflake channel_id) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    for (const auto& [id, ticket] : tickets_) {
        if (ticket.channel_id == channel_id && ticket.status == "aberto") return ticket;
    }
    return std::nullopt;
}

std::optional<Ticket> TicketManager::findTicketById(uint64_t ticket_id) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    auto it = tickets_.find(ticket_id);
    if (it != tickets_.end()) return it->second;
    return std::nullopt;
}

void TicketManager::appendToLog(const dpp::message& msg) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    if (ticket_logs_.find(msg.channel_id) == ticket_logs_.end()) ticket_logs_[msg.channel_id] = "";

    std::string log_line = "[" + Utils::format_timestamp(msg.sent) + "] " + msg.author.username + ": " + msg.content + "\n";
    ticket_logs_[msg.channel_id] += log_line;
}

std::string TicketManager::getAndRemoveLog(dpp::snowflake channel_id) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    std::string log = ticket_logs_[channel_id];
    ticket_logs_.erase(channel_id);
    return log;
}