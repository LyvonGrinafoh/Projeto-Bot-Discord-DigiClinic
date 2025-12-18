#include "DatabaseManager.h"
#include "Utils.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <optional>
#include <algorithm>
#include <string> 
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// --- MANUAL PADRÃO (Iniciado se o cerebro.json não existir) ---
const std::string MANUAL_PADRAO = R"(
VOCÊ É O SISTEMA CENTRAL DE INTELIGÊNCIA DA 'DIGITAL CLINIC'.
Você conversa com funcionários e auxilia na rotina de marketing, vendas e operações.

--- FUNCIONALIDADES E COMANDOS ---
1. GESTÃO DE LEADS:
   - /adicionar_lead: Registra novo cliente (Nome, Contato, Origem).
   - /listar_leads: Mostra lista paginada.
   - /modificar_lead: Atualiza status da negociação.
   - /analise_leads: Gera insights de vendas.

2. AGENDAMENTOS (VISITAS):
   - /visitas: Marca visita para Tatuapé ou Campo Belo.
   - /finalizar_visita: Anexa a ficha preenchida e encerra.
   - O bot avisa automaticamente às 09h sobre visitas do dia seguinte.

3. OPERAÇÕES E ESTOQUE:
   - /demanda: Cria tarefa interna.
   - /pedido: Solicita compra de material.
   - /estoque_baixa e /estoque_repor: Controla insumos (Café, Papelaria, etc).

4. COMUNICAÇÃO INTERNA:
   - /chamar: Abre um chamado privado (Ticket).
   - /relatorio_do_dia: Registro obrigatório diário das atividades.

DIRETRIZES DE PERSONALIDADE:
- Seja prestativo, ágil e profissional, mas com tom amigável.
- Se não souber a resposta, sugira o comando /ia para suporte avançado ou contatar o Matheus.
)";

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

// --- SERIALIZAÇÃO JSON (Solicitacao) ---
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

    if (j.contains("tipo")) j.at("tipo").get_to(s.tipo);
    else if (j.contains("is_pedido") && j.at("is_pedido").get<bool>()) s.tipo = PEDIDO;
    else s.tipo = DEMANDA;

    if (j.contains("status")) j.at("status").get_to(s.status); else s.status = "pendente";
    if (j.contains("anexo_path")) j.at("anexo_path").get_to(s.anexo_path); else s.anexo_path = "";
    if (j.contains("prioridade")) j.at("prioridade").get_to(s.prioridade); else s.prioridade = 1;
    if (j.contains("data_criacao")) j.at("data_criacao").get_to(s.data_criacao); else s.data_criacao = "";
    if (j.contains("data_finalizacao")) j.at("data_finalizacao").get_to(s.data_finalizacao); else s.data_finalizacao = "";
    if (j.contains("observacao_finalizacao")) j.at("observacao_finalizacao").get_to(s.observacao_finalizacao); else s.observacao_finalizacao = "";
}

// --- SERIALIZAÇÃO JSON (Lead) ---
void to_json(json& j, const Lead& l) {
    j = json{
        {"id", l.id},
        {"nome", l.nome},
        {"origem", l.origem},
        {"data_contato", l.data_contato},
        {"hora_contato", l.hora_contato},
        {"contato", l.contato},
        {"area_atuacao", l.area_atuacao},
        {"status_conversa", l.status_conversa},
        {"observacoes", l.observacoes},
        {"criado_por", l.criado_por},
        {"valor_fechamento", l.valor_fechamento},
        {"tipo_fechamento", l.tipo_fechamento}
    };
}

void from_json(const json& j, Lead& l) {
    safe_get_snowflake(j, "id", l.id);
    j.at("nome").get_to(l.nome);
    j.at("origem").get_to(l.origem);
    j.at("data_contato").get_to(l.data_contato);
    if (j.contains("hora_contato")) j.at("hora_contato").get_to(l.hora_contato);
    j.at("contato").get_to(l.contato);
    if (j.contains("area_atuacao")) j.at("area_atuacao").get_to(l.area_atuacao);
    if (j.contains("status_conversa")) j.at("status_conversa").get_to(l.status_conversa);
    if (j.contains("observacoes")) j.at("observacoes").get_to(l.observacoes);
    if (j.contains("criado_por")) j.at("criado_por").get_to(l.criado_por); else l.criado_por = "N/A";
    if (j.contains("valor_fechamento")) j.at("valor_fechamento").get_to(l.valor_fechamento); else l.valor_fechamento = 0;
    if (j.contains("tipo_fechamento")) j.at("tipo_fechamento").get_to(l.tipo_fechamento); else l.tipo_fechamento = "N/A";
}

// --- SERIALIZAÇÃO JSON (Compra) ---
void to_json(json& j, const Compra& c) {
    j = json{
        {"id", c.id},
        {"descricao", c.descricao},
        {"valor", c.valor},
        {"local_compra", c.local_compra},
        {"categoria", c.categoria},
        {"unidade", c.unidade},
        {"observacao", c.observacao},
        {"registrado_por", c.registrado_por},
        {"data_registro", c.data_registro}
    };
}

void from_json(const json& j, Compra& c) {
    safe_get_snowflake(j, "id", c.id);
    j.at("descricao").get_to(c.descricao);
    j.at("valor").get_to(c.valor);
    j.at("local_compra").get_to(c.local_compra);
    j.at("categoria").get_to(c.categoria);
    j.at("unidade").get_to(c.unidade);
    j.at("observacao").get_to(c.observacao);
    j.at("registrado_por").get_to(c.registrado_por);
    j.at("data_registro").get_to(c.data_registro);
}

// --- SERIALIZAÇÃO JSON (Visita) ---
void to_json(json& j, const Visita& v) {
    j = json{
        {"id", v.id},
        {"doutor", v.doutor},
        {"data", v.data},
        {"horario", v.horario},
        {"unidade", v.unidade},
        {"status", v.status},
        {"quem_marcou_nome", v.quem_marcou_nome},
        {"telefone", v.telefone},
        {"area", v.area},
        {"observacoes", v.observacoes},
        {"ficha_path", v.ficha_path}
    };
}

void from_json(const json& j, Visita& v) {
    safe_get_snowflake(j, "id", v.id);
    j.at("doutor").get_to(v.doutor);
    j.at("data").get_to(v.data);
    j.at("horario").get_to(v.horario);
    j.at("unidade").get_to(v.unidade);
    j.at("status").get_to(v.status);
    j.at("quem_marcou_nome").get_to(v.quem_marcou_nome);
    j.at("telefone").get_to(v.telefone);
    j.at("area").get_to(v.area);
    if (j.contains("observacoes")) j.at("observacoes").get_to(v.observacoes);
    if (j.contains("ficha_path")) j.at("ficha_path").get_to(v.ficha_path);
}

// --- SERIALIZAÇÃO JSON (Placa) ---
void to_json(json& j, const Placa& p) {
    j = json{
        {"id", p.id},
        {"doutor", p.doutor},
        {"status", p.status},
        {"tipo_placa", p.tipo_placa},
        {"solicitado_por_nome", p.solicitado_por_nome},
        {"imagem_referencia_path", p.imagem_referencia_path},
        {"arte_final_path", p.arte_final_path}
    };
}

void from_json(const json& j, Placa& p) {
    safe_get_snowflake(j, "id", p.id);
    j.at("doutor").get_to(p.doutor);
    j.at("status").get_to(p.status);
    j.at("tipo_placa").get_to(p.tipo_placa);
    j.at("solicitado_por_nome").get_to(p.solicitado_por_nome);
    if (j.contains("imagem_referencia_path")) j.at("imagem_referencia_path").get_to(p.imagem_referencia_path);
    if (j.contains("arte_final_path")) j.at("arte_final_path").get_to(p.arte_final_path);
}

// --- SERIALIZAÇÃO JSON (Estoque) ---
void to_json(json& j, const EstoqueItem& e) {
    j = json{
        {"id", e.id},
        {"nome_item", e.nome_item},
        {"quantidade", e.quantidade},
        {"quantidade_minima", e.quantidade_minima},
        {"unidade", e.unidade},
        {"local_estoque", e.local_estoque},
        {"categoria", e.categoria}
    };
}

void from_json(const json& j, EstoqueItem& e) {
    safe_get_snowflake(j, "id", e.id);
    j.at("nome_item").get_to(e.nome_item);
    j.at("quantidade").get_to(e.quantidade);
    j.at("quantidade_minima").get_to(e.quantidade_minima);
    j.at("unidade").get_to(e.unidade);
    j.at("local_estoque").get_to(e.local_estoque);
    j.at("categoria").get_to(e.categoria);
}

// --- SERIALIZAÇÃO JSON (Relatorio) ---
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
    j.at("username").get_to(r.username);
    j.at("data_hora").get_to(r.data_hora);
    j.at("conteudo").get_to(r.conteudo);
}


// --- LÓGICA DE CARGA E SALVAMENTO ---

template<typename T>
bool DatabaseManager::loadFromFile(const std::string& filename, std::map<uint64_t, T>& database) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Utils::log_to_file("AVISO: Arquivo " + filename + " nao encontrado. Iniciando vazio.");
        database.clear();
        return true;
    }
    try {
        json j;
        file >> j;
        database.clear();
        if (j.is_array()) {
            for (const auto& item : j) {
                if (item.is_array() && item.size() >= 2) {
                    uint64_t id = 0;
                    if (item[0].is_number()) id = item[0].get<uint64_t>();
                    else if (item[0].is_string()) id = std::stoull(item[0].get<std::string>());

                    if (id != 0 && item[1].is_object()) {
                        database[id] = item[1].get<T>();
                    }
                }
            }
        }
        else if (j.is_object()) {
            database = j.get<std::map<uint64_t, T>>();
        }
    }
    catch (const std::exception& e) {
        Utils::log_to_file("ERRO ao carregar " + filename + ": " + e.what());
        return false;
    }
    return true;
}

template<typename T>
bool DatabaseManager::saveToFile(const std::string& filename, const std::map<uint64_t, T>& database) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        json j = database;
        file << j.dump(4);
    }
    catch (...) {
        return false;
    }
    return true;
}

// --- GESTÃO DE CONHECIMENTO (NOVO SISTEMA DE APRENDIZADO) ---

bool DatabaseManager::loadConhecimento() {
    std::ifstream file(CEREBRO_DATABASE_FILE);
    if (!file.is_open()) {
        Utils::log_to_file("CEREBRO: Memória vazia. Iniciando com manual padrão.");
        conhecimento_bot_.clear();
        conhecimento_bot_.push_back(MANUAL_PADRAO);
        return saveConhecimento();
    }
    try {
        json j;
        file >> j;
        conhecimento_bot_ = j.get<std::vector<std::string>>();
    }
    catch (...) {
        return false;
    }
    return true;
}

bool DatabaseManager::saveConhecimento() {
    try {
        std::ofstream file(CEREBRO_DATABASE_FILE);
        json j = conhecimento_bot_;
        file << j.dump(4);
    }
    catch (...) {
        return false;
    }
    return true;
}

std::string DatabaseManager::getManualCompleto() const {
    std::string full = "";
    for (const auto& s : conhecimento_bot_) {
        full += s + "\n";
    }
    return full;
}

bool DatabaseManager::adicionarConhecimento(const std::string& info) {
    conhecimento_bot_.push_back(info);
    return saveConhecimento();
}

// --- FUNÇÕES PRINCIPAIS DE LOAD/SAVE ---

bool DatabaseManager::loadAll() {
    bool success = true;
    success &= loadFromFile(DATABASE_FILE, solicitacoes_);
    success &= loadFromFile(LEADS_DATABASE_FILE, leads_);
    success &= loadFromFile(COMPRAS_DATABASE_FILE, compras_);
    success &= loadFromFile(VISITAS_DATABASE_FILE, visitas_);
    success &= loadFromFile(PLACAS_DATABASE_FILE, placas_);
    success &= loadFromFile(ESTOQUE_DATABASE_FILE, estoque_);
    success &= loadFromFile(RELATORIOS_DATABASE_FILE, relatorios_);
    success &= loadConhecimento(); // Carrega o Cérebro
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
    success &= saveConhecimento(); // Salva o Cérebro
    return success;
}


// --- GETTERS E SETTERS (Implementação Padrão) ---

// Solicitacoes
const std::map<uint64_t, Solicitacao>& DatabaseManager::getSolicitacoes() const { return solicitacoes_; }
std::optional<Solicitacao> DatabaseManager::getSolicitacao(uint64_t id) const {
    auto it = solicitacoes_.find(id);
    if (it != solicitacoes_.end()) return it->second;
    return std::nullopt;
}
bool DatabaseManager::addOrUpdateSolicitacao(const Solicitacao& s) {
    solicitacoes_[s.id] = s;
    return saveSolicitacoes();
}
bool DatabaseManager::removeSolicitacao(uint64_t id) {
    if (solicitacoes_.erase(id)) return saveSolicitacoes();
    return false;
}
void DatabaseManager::clearSolicitacoes() {
    solicitacoes_.clear();
    saveSolicitacoes();
}
bool DatabaseManager::saveSolicitacoes() { return saveToFile(DATABASE_FILE, solicitacoes_); }

// Leads
const std::map<uint64_t, Lead>& DatabaseManager::getLeads() const { return leads_; }
std::optional<Lead> DatabaseManager::getLead(uint64_t id) const {
    auto it = leads_.find(id);
    if (it != leads_.end()) return it->second;
    return std::nullopt;
}
Lead* DatabaseManager::getLeadPtr(uint64_t id) {
    auto it = leads_.find(id);
    if (it != leads_.end()) return &it->second;
    return nullptr;
}
Lead* DatabaseManager::findLeadByContato(const std::string& contato) {
    for (auto& [id, l] : leads_) {
        if (l.contato == contato) return &l;
    }
    return nullptr;
}
bool DatabaseManager::addOrUpdateLead(const Lead& l) {
    leads_[l.id] = l;
    return saveLeads();
}
bool DatabaseManager::removeLead(uint64_t id) {
    if (leads_.erase(id)) return saveLeads();
    return false;
}
void DatabaseManager::clearLeads() {
    leads_.clear();
    saveLeads();
}
bool DatabaseManager::saveLeads() { return saveToFile(LEADS_DATABASE_FILE, leads_); }

// Compras
const std::map<uint64_t, Compra>& DatabaseManager::getCompras() const { return compras_; }
std::optional<Compra> DatabaseManager::getCompra(uint64_t id) const {
    auto it = compras_.find(id);
    if (it != compras_.end()) return it->second;
    return std::nullopt;
}
bool DatabaseManager::addOrUpdateCompra(const Compra& c) {
    compras_[c.id] = c;
    return saveCompras();
}
bool DatabaseManager::removeCompra(uint64_t id) {
    if (compras_.erase(id)) return saveCompras();
    return false;
}
void DatabaseManager::clearCompras() {
    compras_.clear();
    saveCompras();
}
bool DatabaseManager::saveCompras() { return saveToFile(COMPRAS_DATABASE_FILE, compras_); }

// Visitas
const std::map<uint64_t, Visita>& DatabaseManager::getVisitas() const { return visitas_; }
std::optional<Visita> DatabaseManager::getVisita(uint64_t id) const {
    auto it = visitas_.find(id);
    if (it != visitas_.end()) return it->second;
    return std::nullopt;
}
Visita* DatabaseManager::getVisitaPtr(uint64_t id) {
    auto it = visitas_.find(id);
    if (it != visitas_.end()) return &it->second;
    return nullptr;
}
bool DatabaseManager::addOrUpdateVisita(const Visita& v) {
    visitas_[v.id] = v;
    return saveVisitas();
}
bool DatabaseManager::removeVisita(uint64_t id) {
    if (visitas_.erase(id)) return saveVisitas();
    return false;
}
void DatabaseManager::clearVisitas() {
    visitas_.clear();
    saveVisitas();
}
bool DatabaseManager::saveVisitas() { return saveToFile(VISITAS_DATABASE_FILE, visitas_); }

// Placas
const std::map<uint64_t, Placa>& DatabaseManager::getPlacas() const { return placas_; }
std::optional<Placa> DatabaseManager::getPlaca(uint64_t id) const {
    auto it = placas_.find(id);
    if (it != placas_.end()) return it->second;
    return std::nullopt;
}
Placa* DatabaseManager::getPlacaPtr(uint64_t id) {
    auto it = placas_.find(id);
    if (it != placas_.end()) return &it->second;
    return nullptr;
}
bool DatabaseManager::addOrUpdatePlaca(const Placa& p) {
    placas_[p.id] = p;
    return savePlacas();
}
bool DatabaseManager::removePlaca(uint64_t id) {
    if (placas_.erase(id)) return savePlacas();
    return false;
}
void DatabaseManager::clearPlacas() {
    placas_.clear();
    savePlacas();
}
bool DatabaseManager::savePlacas() { return saveToFile(PLACAS_DATABASE_FILE, placas_); }

// Estoque
const std::map<uint64_t, EstoqueItem>& DatabaseManager::getEstoque() const { return estoque_; }
std::optional<EstoqueItem> DatabaseManager::getEstoqueItem(uint64_t id) const {
    auto it = estoque_.find(id);
    if (it != estoque_.end()) return it->second;
    return std::nullopt;
}
EstoqueItem* DatabaseManager::getEstoqueItemPtr(uint64_t id) {
    auto it = estoque_.find(id);
    if (it != estoque_.end()) return &it->second;
    return nullptr;
}
EstoqueItem* DatabaseManager::getEstoqueItemPorNome(const std::string& nome) {
    std::string nome_lower = nome;
    std::transform(nome_lower.begin(), nome_lower.end(), nome_lower.begin(), ::tolower);
    for (auto& [id, item] : estoque_) {
        std::string item_nome_lower = item.nome_item;
        std::transform(item_nome_lower.begin(), item_nome_lower.end(), item_nome_lower.begin(), ::tolower);
        if (item_nome_lower == nome_lower) return &item;
    }
    return nullptr;
}
bool DatabaseManager::addOrUpdateEstoqueItem(const EstoqueItem& item) {
    estoque_[item.id] = item;
    return saveEstoque();
}
bool DatabaseManager::removeEstoqueItem(uint64_t id) {
    if (estoque_.erase(id)) return saveEstoque();
    return false;
}
bool DatabaseManager::saveEstoque() { return saveToFile(ESTOQUE_DATABASE_FILE, estoque_); }

// Relatorios
const std::map<uint64_t, RelatorioDiario>& DatabaseManager::getRelatorios() const { return relatorios_; }
std::vector<RelatorioDiario> DatabaseManager::getTodosRelatorios() {
    std::vector<RelatorioDiario> lista;
    for (const auto& [id, r] : relatorios_) lista.push_back(r);
    return lista;
}
bool DatabaseManager::addOrUpdateRelatorio(const RelatorioDiario& r) {
    relatorios_[r.id] = r;
    return saveRelatorios();
}
bool DatabaseManager::saveRelatorios() { return saveToFile(RELATORIOS_DATABASE_FILE, relatorios_); }