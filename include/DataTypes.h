#pragma once

#include <string>
#include <vector>
#include <map>
#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <variant>

using json = nlohmann::json;

struct BotConfig {
    std::string token;
    dpp::snowflake server_id;
    dpp::snowflake canal_visitas;
    dpp::snowflake canal_aviso_demandas;
    dpp::snowflake canal_finalizadas;
    dpp::snowflake canal_logs;
    dpp::snowflake canal_aviso_pedidos;
    dpp::snowflake canal_pedidos_concluidos;
    dpp::snowflake canal_solicitacao_placas;
    dpp::snowflake canal_placas_finalizadas;
    dpp::snowflake canal_sugestoes;
    dpp::snowflake canal_bugs;

    dpp::snowflake cargo_permitido;
    dpp::snowflake cargo_tatuape;
    dpp::snowflake cargo_campo_belo;
    dpp::snowflake cargo_adm;

    std::vector<dpp::snowflake> canais_so_imagens;
    std::vector<dpp::snowflake> canais_so_links;
    std::vector<dpp::snowflake> canais_so_documentos;
};

enum TipoSolicitacao {
    DEMANDA,
    PEDIDO,
    LEMBRETE
};

struct Solicitacao {
    uint64_t id = 0;
    dpp::snowflake id_usuario_responsavel = 0;
    std::string texto;
    std::string prazo;
    std::string nome_usuario_responsavel;
    TipoSolicitacao tipo;
    std::string status = "pendente";
    std::string anexo_path;
    int prioridade = 1;
    std::string data_criacao;
    std::string data_finalizacao;
    std::string observacao_finalizacao;
};

void to_json(json& j, const Solicitacao& s);
void from_json(const json& j, Solicitacao& s);

struct Lead {
    uint64_t id = 0;
    std::string origem;
    std::string data_contato;
    std::string hora_contato;
    std::string nome;
    std::string contato;
    std::string area_atuacao;
    bool conhece_formato = false;
    bool veio_campanha = false;
    bool respondido = false;
    std::string data_hora_resposta;
    bool problema_contato = false;
    std::string observacoes;
    std::string status_conversa = "Nao Contatado";
    std::string status_followup = "Primeiro Contato";
    std::string unidade;
    bool convidou_visita = false;
    bool agendou_visita = false;
    std::string tipo_fechamento;
    bool teve_adicionais = false;
    double valor_fechamento = 0.0;
    std::string print_final_conversa;
    std::string criado_por;
};

void to_json(json& j, const Lead& l);
void from_json(const json& j, Lead& l);

struct Compra {
    uint64_t id = 0;
    std::string descricao;
    std::string local_compra;
    double valor = 0.0;
    std::string observacao;
    std::string registrado_por;
    std::string data_registro;
    std::string categoria;
    std::string unidade;
};

void to_json(json& j, const Compra& c);
void from_json(const json& j, Compra& c);

struct Visita {
    uint64_t id = 0;
    dpp::snowflake quem_marcou_id;
    std::string quem_marcou_nome;
    std::string doutor;
    std::string area;
    std::string data;
    std::string horario;
    std::string unidade;
    std::string telefone;
    std::string observacoes;
    std::string status = "agendada";
    std::string relatorio_visita;
    std::string ficha_path;
};

void to_json(json& j, const Visita& v);
void from_json(const json& j, Visita& v);

struct Ticket {
    uint64_t ticket_id = 0;
    dpp::snowflake channel_id = 0;
    dpp::snowflake user_a_id = 0;
    dpp::snowflake user_b_id = 0;
    std::string status = "aberto";
    std::string log_filename;
    std::string nome_ticket;
};

void to_json(json& j, const Ticket& t);
void from_json(const json& j, Ticket& t);

struct Placa {
    uint64_t id = 0;
    std::string doutor;
    std::string tipo_placa;
    dpp::snowflake solicitado_por_id = 0;
    std::string solicitado_por_nome;
    std::string imagem_referencia_path;
    std::string arte_final_path;
    std::string status = "pendente";
};

void to_json(json& j, const Placa& p);
void from_json(const json& j, Placa& p);

struct EstoqueItem {
    uint64_t id = 0;
    std::string nome_item;
    int quantidade = 0;
    std::string unidade;
    std::string local_estoque;
    std::string atualizado_por_nome;
    std::string data_ultima_att;
    int quantidade_minima = 0;
    std::string categoria;
};

void to_json(json& j, const EstoqueItem& e);
void from_json(const json& j, EstoqueItem& e);

struct RelatorioDiario {
    uint64_t id = 0;
    dpp::snowflake user_id = 0;
    std::string username;
    std::string data_hora;
    std::string conteudo;
};

void to_json(json& j, const RelatorioDiario& r);
void from_json(const json& j, RelatorioDiario& r);

const std::string DATABASE_FILE = "database.json";
const std::string LEADS_DATABASE_FILE = "leads_database.json";
const std::string COMPRAS_DATABASE_FILE = "compras_database.json";
const std::string VISITAS_DATABASE_FILE = "visitas_database.json";
const std::string TICKETS_DATABASE_FILE = "tickets.json";
const std::string PLACAS_DATABASE_FILE = "placas.json";
const std::string ESTOQUE_DATABASE_FILE = "estoque.json";
const std::string RELATORIOS_DATABASE_FILE = "relatorios.json";
const std::string LOG_FILE = "bot_log.txt";

using PaginatedItem = std::variant<Lead, Solicitacao, Visita, Placa, EstoqueItem, RelatorioDiario>;

struct PaginationState {
    dpp::snowflake channel_id;
    dpp::snowflake originalUserID;
    std::vector<PaginatedItem> items;
    std::string listType;
    int currentPage = 1;
    int itemsPerPage = 5;
};