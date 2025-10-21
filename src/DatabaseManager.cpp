#include "DatabaseManager.h"
#include "Utils.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <optional>


void to_json(json& j, const Solicitacao& s) { j = json{ {"id", s.id}, {"id_usuario_responsavel", s.id_usuario_responsavel}, {"texto", s.texto}, {"prazo", s.prazo}, {"nome_usuario_responsavel", s.nome_usuario_responsavel}, {"tipo", s.tipo} }; }
void from_json(const json& j, Solicitacao& s) { j.at("id").get_to(s.id); j.at("id_usuario_responsavel").get_to(s.id_usuario_responsavel); j.at("texto").get_to(s.texto); j.at("prazo").get_to(s.prazo); j.at("nome_usuario_responsavel").get_to(s.nome_usuario_responsavel); if (j.contains("tipo")) { j.at("tipo").get_to(s.tipo); } else if (j.contains("is_pedido") && j.at("is_pedido").get<bool>()) { s.tipo = PEDIDO; } else { s.tipo = DEMANDA; } }
void to_json(json& j, const Lead& l) { j = json{ {"id", l.id}, {"origem", l.origem}, {"data_contato", l.data_contato}, {"hora_contato", l.hora_contato}, {"nome", l.nome}, {"contato", l.contato}, {"area_atuacao", l.area_atuacao}, {"conhece_formato", l.conhece_formato}, {"veio_campanha", l.veio_campanha}, {"respondido", l.respondido}, {"data_hora_resposta", l.data_hora_resposta}, {"problema_contato", l.problema_contato}, {"observacoes", l.observacoes}, {"status_conversa", l.status_conversa}, {"status_followup", l.status_followup}, {"unidade", l.unidade}, {"convidou_visita", l.convidou_visita}, {"agendou_visita", l.agendou_visita}, {"tipo_fechamento", l.tipo_fechamento}, {"teve_adicionais", l.teve_adicionais}, {"valor_fechamento", l.valor_fechamento}, {"print_final_conversa", l.print_final_conversa}, {"criado_por", l.criado_por} }; }
void from_json(const json& j, Lead& l) { j.at("id").get_to(l.id); j.at("origem").get_to(l.origem); j.at("data_contato").get_to(l.data_contato); j.at("hora_contato").get_to(l.hora_contato); j.at("nome").get_to(l.nome); j.at("contato").get_to(l.contato); j.at("area_atuacao").get_to(l.area_atuacao); j.at("conhece_formato").get_to(l.conhece_formato); j.at("veio_campanha").get_to(l.veio_campanha); j.at("respondido").get_to(l.respondido); j.at("data_hora_resposta").get_to(l.data_hora_resposta); j.at("problema_contato").get_to(l.problema_contato); j.at("observacoes").get_to(l.observacoes); j.at("status_conversa").get_to(l.status_conversa); j.at("status_followup").get_to(l.status_followup); j.at("unidade").get_to(l.unidade); j.at("convidou_visita").get_to(l.convidou_visita); j.at("agendou_visita").get_to(l.agendou_visita); j.at("tipo_fechamento").get_to(l.tipo_fechamento); j.at("teve_adicionais").get_to(l.teve_adicionais); j.at("valor_fechamento").get_to(l.valor_fechamento); j.at("print_final_conversa").get_to(l.print_final_conversa); if (j.contains("criado_por")) { j.at("criado_por").get_to(l.criado_por); } }
void to_json(json& j, const Compra& c) {
    j = json{{"id", c.id}, {"item", c.item}, {"local_compra", c.local_compra}, {"quantidade", c.quantidade}, {"observacao", c.observacao}, {"solicitado_por", c.solicitado_por}, {"data_solicitacao", c.data_solicitacao}};}
void from_json(const json& j, Compra& c) {
    j.at("id").get_to(c.id);
    j.at("item").get_to(c.item);
    if (j.contains("local_compra")) { j.at("local_compra").get_to(c.local_compra); }
    else { c.local_compra = "N/A"; }
    if (j.contains("unidade_destino")) { j.at("unidade_destino").get_to(c.unidade_destino); }
    else { c.unidade_destino = "N/A"; }
    if (j.contains("quantidade")) { j.at("quantidade").get_to(c.quantidade); }
    else { c.quantidade = 0; }
    j.at("observacao").get_to(c.observacao);
    j.at("solicitado_por").get_to(c.solicitado_por);
    j.at("data_solicitacao").get_to(c.data_solicitacao);
}
void to_json(json& j, const Visita& v) { j = json{ {"id", v.id}, {"quem_marcou_id", v.quem_marcou_id}, {"quem_marcou_nome", v.quem_marcou_nome}, {"doutor", v.doutor}, {"area", v.area}, {"data", v.data}, {"horario", v.horario}, {"unidade", v.unidade}, {"telefone", v.telefone}, {"observacoes", v.observacoes} }; }
void from_json(const json& j, Visita& v) { j.at("id").get_to(v.id); j.at("quem_marcou_id").get_to(v.quem_marcou_id); j.at("quem_marcou_nome").get_to(v.quem_marcou_nome); j.at("doutor").get_to(v.doutor); j.at("area").get_to(v.area); j.at("data").get_to(v.data); j.at("horario").get_to(v.horario); j.at("unidade").get_to(v.unidade); j.at("telefone").get_to(v.telefone); if (j.contains("observacoes")) { j.at("observacoes").get_to(v.observacoes); } else { v.observacoes = ""; } }

// --- Implementação DatabaseManager ---

template<typename T>
bool DatabaseManager::loadFromFile(const std::string& filename, std::map<uint64_t, T>& database) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Utils::log_to_file("AVISO: Arquivo " + filename + " nao encontrado. Iniciando com DB vazio para este tipo.");
        database.clear();
        return true;
    }

    try {
        json j;
        file >> j;
        if (!j.is_null() && !j.empty()) {
            database = j.get<std::map<uint64_t, T>>();
        }
        else {
            database.clear();
        }
    }
    catch (const json::exception& e) {
        std::string error_msg = "AVISO: Erro ao ler " + filename + ": " + e.what() + ". O arquivo pode estar corrompido. Renomeando para backup e continuando com DB vazio.";
        std::cerr << error_msg << std::endl;
        Utils::log_to_file(error_msg);
        file.close();
        std::string backup_name = filename + ".bak";
        std::remove(backup_name.c_str());
        std::rename(filename.c_str(), backup_name.c_str());
        database.clear();
        return false;
    }
    return true;
}

template<typename T>
bool DatabaseManager::saveToFile(const std::string& filename, const std::map<uint64_t, T>& database) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            Utils::log_to_file("ERRO: Nao foi possivel abrir " + filename + " para escrita.");
            std::cerr << "ERRO: Nao foi possivel abrir " + filename + " para escrita." << std::endl;
            return false;
        }
        json j = database;
        file << j.dump(4); 
    }
    catch (const json::exception& e) {
        Utils::log_to_file("ERRO: Falha ao serializar dados para " + filename + ": " + e.what());
        std::cerr << "ERRO: Falha ao serializar dados para " + filename + ": " + e.what() << std::endl;
        return false;
    }
    catch (...) {
        Utils::log_to_file("ERRO: Erro desconhecido ao salvar dados para " + filename);
        std::cerr << "ERRO: Erro desconhecido ao salvar dados para " + filename << std::endl;
        return false;
    }
    return true;
}

bool DatabaseManager::loadAll() {
    bool success = true;
    success &= loadFromFile(DATABASE_FILE, solicitacoes_);
    success &= loadFromFile(LEADS_DATABASE_FILE, leads_);
    success &= loadFromFile(COMPRAS_DATABASE_FILE, compras_);
    success &= loadFromFile(VISITAS_DATABASE_FILE, visitas_);
    return success;
}

bool DatabaseManager::saveAll() {
    bool success = true;
    success &= saveToFile(DATABASE_FILE, solicitacoes_);
    success &= saveToFile(LEADS_DATABASE_FILE, leads_);
    success &= saveToFile(COMPRAS_DATABASE_FILE, compras_);
    success &= saveToFile(VISITAS_DATABASE_FILE, visitas_);
    return success;
}

// --- Implementações dos Getters/Setters/Manipuladores ---

// Solicitacoes
const std::map<uint64_t, Solicitacao>& DatabaseManager::getSolicitacoes() const { return solicitacoes_; }
std::optional<Solicitacao> DatabaseManager::getSolicitacao(uint64_t id) const {
    auto it = solicitacoes_.find(id);
    if (it != solicitacoes_.end()) { return it->second; }
    return std::nullopt;
}
bool DatabaseManager::addOrUpdateSolicitacao(const Solicitacao& s) {
    solicitacoes_[s.id] = s;
    return saveSolicitacoes();
}
bool DatabaseManager::removeSolicitacao(uint64_t id) {
    if (solicitacoes_.erase(id) > 0) {
        return saveSolicitacoes();
    }
    return false;
}
void DatabaseManager::clearSolicitacoes() { solicitacoes_.clear(); saveSolicitacoes(); }
bool DatabaseManager::saveSolicitacoes() { return saveToFile(DATABASE_FILE, solicitacoes_); }

// Leads
const std::map<uint64_t, Lead>& DatabaseManager::getLeads() const { return leads_; }
std::optional<Lead> DatabaseManager::getLead(uint64_t id) const {
    auto it = leads_.find(id);
    if (it != leads_.end()) { return it->second; }
    return std::nullopt;
}
Lead* DatabaseManager::getLeadPtr(uint64_t id) {
    auto it = leads_.find(id);
    if (it != leads_.end()) { return &it->second; }
    return nullptr;
}
bool DatabaseManager::addOrUpdateLead(const Lead& l) {
    leads_[l.id] = l;
    return saveLeads();
}
bool DatabaseManager::removeLead(uint64_t id) {
    if (leads_.erase(id) > 0) { return saveLeads(); }
    return false;
}
void DatabaseManager::clearLeads() { leads_.clear(); saveLeads(); }
bool DatabaseManager::saveLeads() { return saveToFile(LEADS_DATABASE_FILE, leads_); }

// Compras
const std::map<uint64_t, Compra>& DatabaseManager::getCompras() const { return compras_; }
std::optional<Compra> DatabaseManager::getCompra(uint64_t id) const {
    auto it = compras_.find(id);
    if (it != compras_.end()) { return it->second; }
    return std::nullopt;
}
bool DatabaseManager::addOrUpdateCompra(const Compra& c) {
    compras_[c.id] = c;
    return saveCompras();
}
bool DatabaseManager::removeCompra(uint64_t id) {
    if (compras_.erase(id) > 0) { return saveCompras(); }
    return false;
}
void DatabaseManager::clearCompras() { compras_.clear(); saveCompras(); }
bool DatabaseManager::saveCompras() { return saveToFile(COMPRAS_DATABASE_FILE, compras_); }

// Visitas
const std::map<uint64_t, Visita>& DatabaseManager::getVisitas() const { return visitas_; }
std::optional<Visita> DatabaseManager::getVisita(uint64_t id) const {
    auto it = visitas_.find(id);
    if (it != visitas_.end()) { return it->second; }
    return std::nullopt;
}
Visita* DatabaseManager::getVisitaPtr(uint64_t id) {
    auto it = visitas_.find(id);
    if (it != visitas_.end()) { return &it->second; }
    return nullptr;
}
bool DatabaseManager::addOrUpdateVisita(const Visita& v) {
    visitas_[v.id] = v;
    return saveVisitas();
}
bool DatabaseManager::removeVisita(uint64_t id) {
    if (visitas_.erase(id) > 0) { return saveVisitas(); }
    return false;
}
void DatabaseManager::clearVisitas() { visitas_.clear(); saveVisitas(); }
bool DatabaseManager::saveVisitas() { return saveToFile(VISITAS_DATABASE_FILE, visitas_); }