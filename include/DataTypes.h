#pragma once

#include <string>
#include <vector>
#include <map>
#include <dpp/dpp.h>
#include <nlohmann/json.hpp>

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
    std::string item;
    std::string local_compra;
    std::string unidade_destino;
    int quantidade = 0;
    std::string observacao;
    std::string solicitado_por;
    std::string data_solicitacao;
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
};

void to_json(json& j, const Visita& v);
void from_json(const json& j, Visita& v);

const std::string DATABASE_FILE = "database.json";
const std::string LEADS_DATABASE_FILE = "leads_database.json";
const std::string COMPRAS_DATABASE_FILE = "compras_database.json";
const std::string VISITAS_DATABASE_FILE = "visitas_database.json";
const std::string LOG_FILE = "bot_log.txt";