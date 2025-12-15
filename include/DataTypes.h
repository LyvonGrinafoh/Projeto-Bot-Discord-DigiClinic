#pragma once
#include <string>
#include <vector>
#include <variant>
#include <dpp/dpp.h>

enum TipoSolicitacao {
    DEMANDA,
    PEDIDO,
    LEMBRETE
};

struct Solicitacao {
    uint64_t id;
    uint64_t id_usuario_responsavel;
    std::string nome_usuario_responsavel;
    std::string texto;
    std::string prazo;
    TipoSolicitacao tipo;
    std::string status;
    std::string anexo_path;
    int prioridade; // 0=Baixa, 1=Normal, 2=Urgente
    std::string data_criacao;
    std::string data_finalizacao;
    std::string observacao_finalizacao;
};

struct Lead {
    uint64_t id;
    std::string origem;
    std::string data_contato;
    std::string hora_contato;
    std::string nome;
    std::string contato;
    std::string area_atuacao;
    bool conhece_formato;
    bool veio_campanha;
    bool respondido = false;
    std::string data_hora_resposta;
    bool problema_contato = false;
    std::string observacoes;
    std::string status_conversa;
    std::string status_followup;
    std::string unidade;
    bool convidou_visita = false;
    bool agendou_visita = false;
    std::string tipo_fechamento;
    bool teve_adicionais = false;
    double valor_fechamento = 0.0;
    std::string print_final_conversa;
    std::string criado_por;
};

struct Compra {
    uint64_t id;
    std::string descricao;
    std::string local_compra;
    double valor;
    std::string observacao;
    std::string registrado_por;
    std::string data_registro;
    std::string categoria;
    std::string unidade;
};

struct Visita {
    uint64_t id;
    uint64_t quem_marcou_id;
    std::string quem_marcou_nome;
    std::string doutor;
    std::string area;
    std::string data;
    std::string horario;
    std::string unidade;
    std::string telefone;
    std::string observacoes;
    std::string status;
    std::string relatorio_visita;
    std::string ficha_path;
};

struct Ticket {
    uint64_t ticket_id;
    uint64_t channel_id;
    uint64_t user_a_id;
    uint64_t user_b_id;
    std::string status;
    std::string log_filename;
    std::string nome_ticket;
};

struct Placa {
    uint64_t id;
    std::string doutor;
    std::string tipo_placa;
    uint64_t solicitado_por_id;
    std::string solicitado_por_nome;
    std::string imagem_referencia_path;
    std::string arte_final_path;
    std::string status;
};

struct EstoqueItem {
    uint64_t id;
    std::string nome_item;
    int quantidade;
    std::string unidade;
    std::string local_estoque;
    std::string atualizado_por_nome;
    std::string data_ultima_att;
    int quantidade_minima;
    std::string categoria;
};

// ESTRUTURA NOVA
struct RelatorioDiario {
    uint64_t id;
    uint64_t user_id;
    std::string username;
    std::string data_hora;
    std::string conteudo;
};

// CONFIGURAÇÃO COMPLETA (Isso corrige os erros no ConfigManager e EventHandler)
struct BotConfig {
    std::string token;
    dpp::snowflake server_id;

    // Canais
    dpp::snowflake canal_logs;
    dpp::snowflake canal_aviso_demandas;
    dpp::snowflake canal_aviso_pedidos;
    dpp::snowflake canal_pedidos_concluidos;
    dpp::snowflake canal_finalizadas;
    dpp::snowflake canal_visitas;
    dpp::snowflake canal_solicitacao_placas;
    dpp::snowflake canal_placas_finalizadas;
    dpp::snowflake canal_sugestoes;
    dpp::snowflake canal_bugs;      
    dpp::snowflake cargo_tatuape;
    dpp::snowflake cargo_campo_belo;
    dpp::snowflake cargo_permitido; 
    dpp::snowflake cargo_adm;      


    std::vector<dpp::snowflake> canais_so_imagens;
    std::vector<dpp::snowflake> canais_so_links;
    std::vector<dpp::snowflake> canais_so_documentos;
};

struct PaginationState {
    dpp::snowflake channel_id;
    std::vector<std::variant<Solicitacao, Lead, Visita, Placa, EstoqueItem, RelatorioDiario>> items;
    dpp::snowflake originalUserID;
    std::string listType;
    int currentPage;
    int itemsPerPage;
};

using PaginatedItem = std::variant<Solicitacao, Lead, Visita, Placa, EstoqueItem, RelatorioDiario>;