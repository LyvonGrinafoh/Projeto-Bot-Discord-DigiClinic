#include "DatabaseManager.h"
#include "Utils.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <optional>
#include <algorithm>
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

void to_json(json& j, const Solicitacao& s) {
    j = json{
        {"id", s.id},
        {"id_usuario_responsavel", s.id_usuario_responsavel},
        {"texto", s.texto},
        {"prazo", s.prazo},
        {"nome_usuario_responsavel", s.nome_usuario_responsavel},
        {"tipo", s.tipo},
        {"status", s.status},
        {"anexo_path", s.anexo_path},
        {"prioridade", s.prioridade},
        {"data_criacao", s.data_criacao},
        {"data_finalizacao", s.data_finalizacao},
        {"observacao_finalizacao", s.observacao_finalizacao}
    };
}
void from_json(const json& j, Solicitacao& s) {
    safe_get_snowflake(j, "id", s.id);
    safe_get_snowflake(j, "id_usuario_responsavel", s.id_usuario_responsavel);
    j.at("texto").get_to(s.texto);
    j.at("prazo").get_to(s.prazo);
    j.at("nome_usuario_responsavel").get_to(s.nome_usuario_responsavel);

    if (j.contains("tipo")) { j.at("tipo").get_to(s.tipo); }
    else if (j.contains("is_pedido") && j.at("is_pedido").get<bool>()) { s.tipo = PEDIDO; }
    else { s.tipo = DEMANDA; }

    if (j.contains("status")) { j.at("status").get_to(s.status); }
    else { s.status = "pendente"; }

    if (j.contains("anexo_path")) { j.at("anexo_path").get_to(s.anexo_path); }
    else { s.anexo_path = ""; }

    if (j.contains("prioridade")) { j.at("prioridade").get_to(s.prioridade); }
    else { s.prioridade = 1; }

    if (j.contains("data_criacao")) { j.at("data_criacao").get_to(s.data_criacao); }
    else { s.data_criacao = ""; }

    if (j.contains("data_finalizacao")) { j.at("data_finalizacao").get_to(s.data_finalizacao); }
    else { s.data_finalizacao = ""; }

    if (j.contains("observacao_finalizacao")) { j.at("observacao_finalizacao").get_to(s.observacao_finalizacao); }
    else { s.observacao_finalizacao = ""; }
}

void to_json(json& j, const Lead& l) {
    j = json{
        {"id", l.id},
        {"origem", l.origem},
        {"data_contato", l.data_contato},
        {"hora_contato", l.hora_contato},
        {"nome", l.nome},
        {"contato", l.contato},
        {"area_atuacao", l.area_atuacao},
        {"conhece_formato", l.conhece_formato},
        {"veio_campanha", l.veio_campanha},
        {"respondido", l.respondido},
        {"data_hora_resposta", l.data_hora_resposta},
        {"problema_contato", l.problema_contato},
        {"observacoes", l.observacoes},
        {"status_conversa", l.status_conversa},
        {"status_followup", l.status_followup},
        {"unidade", l.unidade},
        {"convidou_visita", l.convidou_visita},
        {"agendou_visita", l.agendou_visita},
        {"tipo_fechamento", l.tipo_fechamento},
        {"teve_adicionais", l.teve_adicionais},
        {"valor_fechamento", l.valor_fechamento},
        {"print_final_conversa", l.print_final_conversa},
        {"criado_por", l.criado_por}
    };
}
void from_json(const json& j, Lead& l) {
    safe_get_snowflake(j, "id", l.id);
    j.at("origem").get_to(l.origem);
    j.at("data_contato").get_to(l.data_contato);
    j.at("hora_contato").get_to(l.hora_contato);
    j.at("nome").get_to(l.nome);
    j.at("contato").get_to(l.contato);
    j.at("area_atuacao").get_to(l.area_atuacao);
    j.at("conhece_formato").get_to(l.conhece_formato);
    j.at("veio_campanha").get_to(l.veio_campanha);
    j.at("respondido").get_to(l.respondido);
    j.at("data_hora_resposta").get_to(l.data_hora_resposta);
    j.at("problema_contato").get_to(l.problema_contato);
    j.at("observacoes").get_to(l.observacoes);
    j.at("status_conversa").get_to(l.status_conversa);
    j.at("status_followup").get_to(l.status_followup);
    j.at("unidade").get_to(l.unidade);
    j.at("convidou_visita").get_to(l.convidou_visita);
    j.at("agendou_visita").get_to(l.agendou_visita);
    j.at("tipo_fechamento").get_to(l.tipo_fechamento);
    j.at("teve_adicionais").get_to(l.teve_adicionais);
    j.at("valor_fechamento").get_to(l.valor_fechamento);
    j.at("print_final_conversa").get_to(l.print_final_conversa);
    if (j.contains("criado_por")) { j.at("criado_por").get_to(l.criado_por); }
}

void to_json(json& j, const Compra& c) {
    j = json{
        {"id", c.id},
        {"descricao", c.descricao},
        {"local_compra", c.local_compra},
        {"valor", c.valor},
        {"observacao", c.observacao},
        {"registrado_por", c.registrado_por},
        {"data_registro", c.data_registro},
        {"categoria", c.categoria},
        {"unidade", c.unidade}
    };
}
void from_json(const json& j, Compra& c) {
    safe_get_snowflake(j, "id", c.id);

    if (j.contains("descricao")) { j.at("descricao").get_to(c.descricao); }
    else if (j.contains("item")) { j.at("item").get_to(c.descricao); }
    else { c.descricao = "N/A"; }

    if (j.contains("local_compra")) { j.at("local_compra").get_to(c.local_compra); }
    else { c.local_compra = "N/A"; }

    if (j.contains("valor")) { j.at("valor").get_to(c.valor); }
    else { c.valor = 0.0; }

    if (j.contains("observacao")) { j.at("observacao").get_to(c.observacao); }
    else { c.observacao = ""; }

    if (j.contains("registrado_por")) { j.at("registrado_por").get_to(c.registrado_por); }
    else if (j.contains("solicitado_por")) { j.at("solicitado_por").get_to(c.registrado_por); }
    else { c.registrado_por = "N/A"; }

    if (j.contains("data_registro")) { j.at("data_registro").get_to(c.data_registro); }
    else if (j.contains("data_solicitacao")) { j.at("data_solicitacao").get_to(c.data_registro); }
    else { c.data_registro = "N/A"; }

    if (j.contains("categoria")) { j.at("categoria").get_to(c.categoria); }
    else { c.categoria = "N/A"; }
    if (j.contains("unidade")) { j.at("unidade").get_to(c.unidade); }
    else { c.unidade = "N/A"; }
}

void to_json(json& j, const Visita& v) {
    j = json{
        {"id", v.id},
        {"quem_marcou_id", v.quem_marcou_id},
        {"quem_marcou_nome", v.quem_marcou_nome},
        {"doutor", v.doutor},
        {"area", v.area},
        {"data", v.data},
        {"horario", v.horario},
        {"unidade", v.unidade},
        {"telefone", v.telefone},
        {"observacoes", v.observacoes},
        {"status", v.status},
        {"relatorio_visita", v.relatorio_visita},
        {"ficha_path", v.ficha_path}
    };
}
void from_json(const json& j, Visita& v) {
    safe_get_snowflake(j, "id", v.id);
    safe_get_snowflake(j, "quem_marcou_id", v.quem_marcou_id);
    j.at("quem_marcou_nome").get_to(v.quem_marcou_nome);
    j.at("doutor").get_to(v.doutor);
    j.at("area").get_to(v.area);
    j.at("data").get_to(v.data);
    j.at("horario").get_to(v.horario);
    j.at("unidade").get_to(v.unidade);
    j.at("telefone").get_to(v.telefone);
    if (j.contains("observacoes")) { j.at("observacoes").get_to(v.observacoes); }
    else { v.observacoes = ""; }

    if (j.contains("status")) { j.at("status").get_to(v.status); }
    else {
        if (v.observacoes.find("Visita Cancelada") != std::string::npos) { v.status = "cancelada"; }
        else { v.status = "agendada"; }
    }
    if (j.contains("relatorio_visita")) { j.at("relatorio_visita").get_to(v.relatorio_visita); }
    else { v.relatorio_visita = ""; }

    if (j.contains("ficha_path")) { j.at("ficha_path").get_to(v.ficha_path); }
    else { v.ficha_path = ""; }
}

void to_json(json& j, const Ticket& t) {
    j = json{
        {"ticket_id", t.ticket_id},
        {"channel_id", t.channel_id},
        {"user_a_id", t.user_a_id},
        {"user_b_id", t.user_b_id},
        {"status", t.status},
        {"log_filename", t.log_filename},
        {"nome_ticket", t.nome_ticket}
    };
}
void from_json(const json& j, Ticket& t) {
    safe_get_snowflake(j, "ticket_id", t.ticket_id);
    safe_get_snowflake(j, "channel_id", t.channel_id);
    safe_get_snowflake(j, "user_a_id", t.user_a_id);
    safe_get_snowflake(j, "user_b_id", t.user_b_id);

    if (j.contains("status")) { j.at("status").get_to(t.status); }
    else { t.status = "aberto"; }
    if (j.contains("log_filename")) { j.at("log_filename").get_to(t.log_filename); }
    else { t.log_filename = ""; }
    if (j.contains("nome_ticket")) { j.at("nome_ticket").get_to(t.nome_ticket); }
    else { t.nome_ticket = "Sem Assunto"; }
}

void to_json(json& j, const Placa& p) {
    j = json{
        {"id", p.id},
        {"doutor", p.doutor},
        {"tipo_placa", p.tipo_placa},
        {"solicitado_por_id", p.solicitado_por_id},
        {"solicitado_por_nome", p.solicitado_por_nome},
        {"imagem_referencia_path", p.imagem_referencia_path},
        {"arte_final_path", p.arte_final_path},
        {"status", p.status}
    };
}
void from_json(const json& j, Placa& p) {
    safe_get_snowflake(j, "id", p.id);
    j.at("doutor").get_to(p.doutor);

    if (j.contains("tipo_placa")) { j.at("tipo_placa").get_to(p.tipo_placa); }
    else { p.tipo_placa = "N/A"; }

    safe_get_snowflake(j, "solicitado_por_id", p.solicitado_por_id);
    j.at("solicitado_por_nome").get_to(p.solicitado_por_nome);
    j.at("status").get_to(p.status);

    if (j.contains("imagem_referencia_path")) { j.at("imagem_referencia_path").get_to(p.imagem_referencia_path); }
    else if (j.contains("imagem_referencia_url")) { j.at("imagem_referencia_url").get_to(p.imagem_referencia_path); }
    else { p.imagem_referencia_path = ""; }

    if (j.contains("arte_final_path")) { j.at("arte_final_path").get_to(p.arte_final_path); }
    else if (j.contains("arte_final_url")) { j.at("arte_final_url").get_to(p.arte_final_path); }
    else { p.arte_final_path = ""; }
}

void to_json(json& j, const EstoqueItem& e) {
    j = json{
        {"id", e.id},
        {"nome_item", e.nome_item},
        {"quantidade", e.quantidade},
        {"unidade", e.unidade},
        {"local_estoque", e.local_estoque},
        {"atualizado_por_nome", e.atualizado_por_nome},
        {"data_ultima_att", e.data_ultima_att},
        {"quantidade_minima", e.quantidade_minima},
        {"categoria", e.categoria}
    };
}
void from_json(const json& j, EstoqueItem& e) {
    safe_get_snowflake(j, "id", e.id);

    if (j.contains("nome_item")) { j.at("nome_item").get_to(e.nome_item); }
    else { e.nome_item = "N/A"; }

    if (j.contains("quantidade")) { j.at("quantidade").get_to(e.quantidade); }
    else { e.quantidade = 0; }

    if (j.contains("unidade")) { j.at("unidade").get_to(e.unidade); }
    else { e.unidade = "N/A"; }

    if (j.contains("local_estoque")) { j.at("local_estoque").get_to(e.local_estoque); }
    else { e.local_estoque = "N/A"; }

    if (j.contains("atualizado_por_nome")) { j.at("atualizado_por_nome").get_to(e.atualizado_por_nome); }
    else { e.atualizado_por_nome = "N/A"; }

    if (j.contains("data_ultima_att")) { j.at("data_ultima_att").get_to(e.data_ultima_att); }
    else { e.data_ultima_att = "N/A"; }

    if (j.contains("quantidade_minima")) { j.at("quantidade_minima").get_to(e.quantidade_minima); }
    else { e.quantidade_minima = 0; }

    if (j.contains("categoria")) { j.at("categoria").get_to(e.categoria); }
    else { e.categoria = "N/A"; }
}

void to_json(json& j, const RelatorioDiario& r) {
    j = json{
        {"id", r.id},
        {"user_id", r.user_id},
        {"username", r.username},
        {"data_hora", r.data_hora},
        {"conteudo", r.conteudo}
    };
}
void from_json(const json& j, RelatorioDiario& r) {
    safe_get_snowflake(j, "id", r.id);
    safe_get_snowflake(j, "user_id", r.user_id);
    if (j.contains("username")) j.at("username").get_to(r.username); else r.username = "N/A";
    if (j.contains("data_hora")) j.at("data_hora").get_to(r.data_hora); else r.data_hora = "";
    if (j.contains("conteudo")) j.at("conteudo").get_to(r.conteudo); else r.conteudo = "";
}

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
    success &= loadFromFile(PLACAS_DATABASE_FILE, placas_);
    success &= loadFromFile(ESTOQUE_DATABASE_FILE, estoque_);
    success &= loadFromFile(RELATORIOS_DATABASE_FILE, relatorios_);
    return success;
}

bool DatabaseManager::saveAll() {
    bool success = true;
    success &= saveToFile(DATABASE_FILE, solicitacoes_);
    success &= saveToFile(LEADS_DATABASE_FILE, leads_);
    success &= saveToFile(COMPRAS_DATABASE_FILE, compras_);
    success &= saveToFile(VISITAS_DATABASE_FILE, visitas_);
    success &= saveToFile(PLACAS_DATABASE_FILE, placas_);
    success &= saveToFile(ESTOQUE_DATABASE_FILE, estoque_);
    success &= saveToFile(RELATORIOS_DATABASE_FILE, relatorios_);
    return success;
}

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
Lead* DatabaseManager::findLeadByContato(const std::string& contato) {
    if (contato.empty()) {
        return nullptr;
    }
    std::string contato_lower = contato;
    std::transform(contato_lower.begin(), contato_lower.end(), contato_lower.begin(), ::tolower);

    for (auto& [id, lead] : leads_) {
        std::string lead_contato_lower = lead.contato;
        std::transform(lead_contato_lower.begin(), lead_contato_lower.end(), lead_contato_lower.begin(), ::tolower);
        if (lead_contato_lower == contato_lower) {
            return &lead;
        }
    }
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
void DatabaseManager::clearVisitas() {
    std::map<uint64_t, Visita> visitas_filtradas;
    for (auto const& [id, visita] : visitas_) {
        if (visita.status != "agendada") {
            visitas_filtradas[id] = visita;
        }
    }
    visitas_ = visitas_filtradas;
    saveVisitas();
}
bool DatabaseManager::saveVisitas() { return saveToFile(VISITAS_DATABASE_FILE, visitas_); }

const std::map<uint64_t, Placa>& DatabaseManager::getPlacas() const { return placas_; }
std::optional<Placa> DatabaseManager::getPlaca(uint64_t id) const {
    auto it = placas_.find(id);
    if (it != placas_.end()) { return it->second; }
    return std::nullopt;
}
Placa* DatabaseManager::getPlacaPtr(uint64_t id) {
    auto it = placas_.find(id);
    if (it != placas_.end()) { return &it->second; }
    return nullptr;
}
bool DatabaseManager::addOrUpdatePlaca(const Placa& p) {
    placas_[p.id] = p;
    return savePlacas();
}
bool DatabaseManager::removePlaca(uint64_t id) {
    if (placas_.erase(id) > 0) {
        return savePlacas();
    }
    return false;
}
void DatabaseManager::clearPlacas() {
    std::map<uint64_t, Placa> placas_filtradas;
    for (auto const& [id, placa] : placas_) {
        if (placa.status != "pendente") {
            placas_filtradas[id] = placa;
        }
    }
    placas_ = placas_filtradas;
    savePlacas();
}
bool DatabaseManager::savePlacas() { return saveToFile(PLACAS_DATABASE_FILE, placas_); }

const std::map<uint64_t, EstoqueItem>& DatabaseManager::getEstoque() const { return estoque_; }
std::optional<EstoqueItem> DatabaseManager::getEstoqueItem(uint64_t id) const {
    auto it = estoque_.find(id);
    if (it != estoque_.end()) { return it->second; }
    return std::nullopt;
}
EstoqueItem* DatabaseManager::getEstoqueItemPtr(uint64_t id) {
    auto it = estoque_.find(id);
    if (it != estoque_.end()) { return &it->second; }
    return nullptr;
}
EstoqueItem* DatabaseManager::getEstoqueItemPorNome(const std::string& nome) {
    std::string nome_lower = nome;
    std::transform(nome_lower.begin(), nome_lower.end(), nome_lower.begin(), ::tolower);

    for (auto& [id, item] : estoque_) {
        std::string item_nome_lower = item.nome_item;
        std::transform(item_nome_lower.begin(), item_nome_lower.end(), item_nome_lower.begin(), ::tolower);
        if (item_nome_lower == nome_lower) {
            return &item;
        }
    }
    return nullptr;
}
bool DatabaseManager::addOrUpdateEstoqueItem(const EstoqueItem& item) {
    estoque_[item.id] = item;
    return saveEstoque();
}
bool DatabaseManager::removeEstoqueItem(uint64_t id) {
    if (estoque_.erase(id) > 0) {
        return saveEstoque();
    }
    return false;
}
bool DatabaseManager::saveEstoque() { return saveToFile(ESTOQUE_DATABASE_FILE, estoque_); }

const std::map<uint64_t, RelatorioDiario>& DatabaseManager::getRelatorios() const { return relatorios_; }

bool DatabaseManager::addOrUpdateRelatorio(const RelatorioDiario& r) {
    relatorios_[r.id] = r;
    return saveRelatorios();
}

bool DatabaseManager::saveRelatorios() { return saveToFile(RELATORIOS_DATABASE_FILE, relatorios_); }