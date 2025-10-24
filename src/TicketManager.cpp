#include "TicketManager.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <chrono>

void to_json(json& j, const Ticket& t) {
    j = json{
        {"ticket_id", t.ticket_id},
        {"channel_id", t.channel_id},
        {"user_a_id", t.user_a_id},
        {"user_b_id", t.user_b_id}
    };
}
void from_json(const json& j, Ticket& t) {
    j.at("ticket_id").get_to(t.ticket_id);
    j.at("channel_id").get_to(t.channel_id);
    j.at("user_a_id").get_to(t.user_a_id);
    j.at("user_b_id").get_to(t.user_b_id);
}

TicketManager::TicketManager() : random_engine_(std::random_device{}()) {}

bool TicketManager::loadTickets() {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    std::ifstream file(TICKETS_DATABASE_FILE);
    if (!file.is_open()) {
        Utils::log_to_file("AVISO: " + TICKETS_DATABASE_FILE + " nao encontrado. Iniciando com DB vazio.");
        tickets_.clear();
        return true;
    }
    try {
        json j;
        file >> j;
        if (!j.is_null() && !j.empty()) {
            tickets_ = j.get<std::map<uint64_t, Ticket>>();
        }
        else {
            tickets_.clear();
        }
    }
    catch (const json::exception& e) {
        std::string error_msg = "AVISO: Erro ao ler " + TICKETS_DATABASE_FILE + ": " + e.what() + ". Renomeando para backup.";
        std::cerr << error_msg << std::endl;
        Utils::log_to_file(error_msg);
        file.close();
        std::string backup_name = TICKETS_DATABASE_FILE + ".bak";
        std::remove(backup_name.c_str());
        std::rename(TICKETS_DATABASE_FILE.c_str(), backup_name.c_str());
        tickets_.clear();
        return false;
    }
    return true;
}

bool TicketManager::saveTickets() {

    try {
        std::ofstream file(TICKETS_DATABASE_FILE);
        if (!file.is_open()) {
            Utils::log_to_file("ERRO: Nao foi possivel abrir " + TICKETS_DATABASE_FILE + " para escrita.");
            return false;
        }
        json j = tickets_;
        file << j.dump(4);
    }
    catch (const std::exception& e) {
        Utils::log_to_file("ERRO: Falha ao serializar dados para " + TICKETS_DATABASE_FILE + ": " + e.what());
        return false;
    }
    return true;
}

uint64_t TicketManager::generateTicketID() {
    std::uniform_int_distribution<uint64_t> dist(0, 1000000);
    uint64_t id = 0;
    do {
        id = dist(random_engine_);
    } while (tickets_.count(id) > 0 || id == 0);
    return id;
}

Ticket TicketManager::createTicket(dpp::snowflake user_a, dpp::snowflake user_b, dpp::snowflake channel_id) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    Ticket new_ticket;
    new_ticket.ticket_id = generateTicketID();
    new_ticket.channel_id = channel_id;
    new_ticket.user_a_id = user_a;
    new_ticket.user_b_id = user_b;

    tickets_[new_ticket.ticket_id] = new_ticket;

    saveTickets();

    ticket_logs_[channel_id] = "Log de conversa do Ticket #" + std::to_string(new_ticket.ticket_id) + " - Criado em " + Utils::format_timestamp(std::time(nullptr)) + "\n";
    ticket_logs_[channel_id] += "Participantes: " + std::to_string(user_a) + " e " + std::to_string(user_b) + "\n\n";

    return new_ticket;
}

bool TicketManager::removeTicket(uint64_t ticket_id) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    if (tickets_.count(ticket_id) > 0) {
        tickets_.erase(ticket_id);

        return saveTickets();
    }
    return false;
}

std::optional<Ticket> TicketManager::findTicketByChannel(dpp::snowflake channel_id) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    for (const auto& [id, ticket] : tickets_) {
        if (ticket.channel_id == channel_id) {
            return ticket;
        }
    }
    return std::nullopt;
}

std::optional<Ticket> TicketManager::findTicketById(uint64_t ticket_id) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    auto it = tickets_.find(ticket_id);
    if (it != tickets_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void TicketManager::appendToLog(dpp::snowflake channel_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    ticket_logs_[channel_id] += message + "\n";
}

std::string TicketManager::getAndRemoveLog(dpp::snowflake channel_id) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    std::string log;
    auto it = ticket_logs_.find(channel_id);
    if (it != ticket_logs_.end()) {
        log = it->second;
        ticket_logs_.erase(it);
    }
    return log;
}