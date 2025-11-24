#include "TicketManager.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <chrono>
#include <string>

static void safe_get_snowflake(const json& j, const std::string& key, uint64_t& target) {
    if (j.contains(key)) {
        const auto& value = j.at(key);
        if (value.is_number()) {
            value.get_to(target);
        }
        else if (value.is_string()) {
            try {
                target = std::stoull(value.get<std::string>());
            }
            catch (...) {
                target = 0;
            }
        }
        else {
            target = 0;
        }
    }
    else {
        target = 0;
    }
}

void to_json(json& j, const Ticket& t) {
    j = json{
        {"ticket_id", t.ticket_id},
        {"channel_id", t.channel_id},
        {"user_a_id", t.user_a_id},
        {"user_b_id", t.user_b_id},
        {"status", t.status},
        {"log_filename", t.log_filename}
    };
}
void from_json(const json& j, Ticket& t) {
    safe_get_snowflake(j, "ticket_id", t.ticket_id); // Protegido
    safe_get_snowflake(j, "channel_id", t.channel_id); // Protegido
    safe_get_snowflake(j, "user_a_id", t.user_a_id); // Protegido
    safe_get_snowflake(j, "user_b_id", t.user_b_id); // Protegido

    if (j.contains("status")) {
        j.at("status").get_to(t.status);
    }
    else {
        t.status = "aberto";
    }

    if (j.contains("log_filename")) {
        j.at("log_filename").get_to(t.log_filename);
    }
    else {
        t.log_filename = "";
    }
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
    new_ticket.status = "aberto";
    new_ticket.log_filename = "";

    tickets_[new_ticket.ticket_id] = new_ticket;

    saveTickets();

    ticket_logs_[channel_id] = "Log de conversa do Ticket #" + std::to_string(new_ticket.ticket_id) + " - Criado em " + Utils::format_timestamp(std::time(nullptr)) + "\n";
    ticket_logs_[channel_id] += "Participantes: " + std::to_string(user_a) + " e " + std::to_string(user_b) + "\n\n";

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
        if (ticket.channel_id == channel_id && ticket.status == "aberto") {
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

void TicketManager::appendToLog(const dpp::message& msg) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);

    auto it = ticket_logs_.find(msg.channel_id);
    if (it != ticket_logs_.end()) {

        std::string log_line = "[" + Utils::format_timestamp(msg.sent) + "] ";
        log_line += "[" + msg.author.username + " | Msg ID: " + std::to_string(msg.id) + "]: ";
        log_line += msg.content;

        if (!msg.attachments.empty()) {
            for (const auto& att : msg.attachments) {
                log_line += " [Anexo: " + att.url + "]";
            }
        }

        it->second += log_line + "\n";
    }
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