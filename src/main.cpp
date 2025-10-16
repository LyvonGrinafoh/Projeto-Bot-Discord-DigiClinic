#include <dpp/dpp.h>
#include <fstream>
#include <string>
#include <map>
#include <random>
#include <variant>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <xlsxwriter.h>
#include <iostream>
#include <vector>
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <csignal>

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
BotConfig config;

const std::string DATABASE_FILE = "database.json";
const std::string LEADS_DATABASE_FILE = "leads_database.json";
const std::string COMPRAS_DATABASE_FILE = "compras_database.json";
const std::string VISITAS_DATABASE_FILE = "visitas_database.json";
const std::string LOG_FILE = "bot_log.txt";

enum TipoSolicitacao {
    DEMANDA,  // 0
    PEDIDO,   // 1
    LEMBRETE  // 2
};

struct Solicitacao {
    uint64_t id = 0;
    dpp::snowflake id_usuario_responsavel = 0;
    std::string texto;
    std::string prazo;
    std::string nome_usuario_responsavel;
    TipoSolicitacao tipo;
};
std::map<uint64_t, Solicitacao> banco_de_dados;

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
std::map<uint64_t, Lead> leads_database;

struct Compra {
    uint64_t id = 0;
    std::string item;
    std::string observacao;
    std::string solicitado_por;
    std::string data_solicitacao;
};
std::map<uint64_t, Compra> compras_database;

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
};
std::map<uint64_t, Visita> visitas_database;

dpp::cluster* bot_ptr = nullptr;

std::string format_timestamp(time_t timestamp) { std::stringstream ss; ss << std::put_time(std::localtime(&timestamp), "%d/%m/%Y %H:%M:%S"); return ss.str(); }

void log_to_file(const std::string& message) {
    std::ofstream log_file(LOG_FILE, std::ios_base::app);
    if (log_file.is_open()) {
        log_file << "[" << format_timestamp(std::time(nullptr)) << "] " << message << std::endl;
        log_file.close();
    }
}

bool load_config() {
    std::ifstream config_file("config.json");
    if (!config_file.is_open()) {
        std::cerr << "AVISO: Arquivo config.json nao encontrado. Criando um novo..." << std::endl;
        log_to_file("AVISO: Arquivo config.json nao encontrado. Criando um novo...");
        json j;
        j["BOT_TOKEN"] = "SEU_TOKEN_AQUI";
        j["SEU_SERVER_ID"] = 0;
        j["CANAIS"] = {
            {"VISITAS", 0},
            {"AVISO_DEMANDAS", 0},
            {"FINALIZADAS", 0},
            {"LOGS", 0},
            {"AVISO_PEDIDOS", 0},
            {"PEDIDOS_CONCLUIDOS", 0},
            {"SOLICITACAO_PLACAS", 0},
            {"PLACAS_FINALIZADAS", 0},
            {"SUGESTOES", 0},
            {"BUGS", 0}
        };
        j["CARGOS"] = {
            {"PERMITIDO", 0}
        };
        j["CANAIS_RESTRITOS"] = {
            {"SO_IMAGENS", json::array()},
            {"SO_LINKS", json::array()},
            {"SO_DOCUMENTOS", json::array()}
        };

        std::ofstream new_config_file("config.json");
        new_config_file << j.dump(4);
        new_config_file.close();

        std::cerr << "ERRO FATAL: Um novo config.json foi criado. Por favor, preencha as informacoes e reinicie o bot." << std::endl;
        log_to_file("ERRO FATAL: config.json criado. Bot precisa de configuracao manual para iniciar.");
        return false;
    }
    json j;
    try {
        config_file >> j;
        config.token = j.at("BOT_TOKEN").get<std::string>();
        config.server_id = j.at("SEU_SERVER_ID").get<dpp::snowflake>();
        config.canal_visitas = j.at("CANAIS").at("VISITAS").get<dpp::snowflake>();
        config.canal_aviso_demandas = j.at("CANAIS").at("AVISO_DEMANDAS").get<dpp::snowflake>();
        config.canal_finalizadas = j.at("CANAIS").at("FINALIZADAS").get<dpp::snowflake>();
        config.canal_logs = j.at("CANAIS").at("LOGS").get<dpp::snowflake>();
        config.canal_aviso_pedidos = j.at("CANAIS").at("AVISO_PEDIDOS").get<dpp::snowflake>();
        config.canal_pedidos_concluidos = j.at("CANAIS").at("PEDIDOS_CONCLUIDOS").get<dpp::snowflake>();
        config.canal_solicitacao_placas = j.at("CANAIS").at("SOLICITACAO_PLACAS").get<dpp::snowflake>();
        config.canal_placas_finalizadas = j.at("CANAIS").at("PLACAS_FINALIZADAS").get<dpp::snowflake>();
        config.canal_sugestoes = j.at("CANAIS").at("SUGESTOES").get<dpp::snowflake>();
        config.canal_bugs = j.at("CANAIS").at("BUGS").get<dpp::snowflake>();
        config.cargo_permitido = j.at("CARGOS").at("PERMITIDO").get<dpp::snowflake>();
        config.canais_so_imagens = j.at("CANAIS_RESTRITOS").at("SO_IMAGENS").get<std::vector<dpp::snowflake>>();
        config.canais_so_links = j.at("CANAIS_RESTRITOS").at("SO_LINKS").get<std::vector<dpp::snowflake>>();
        config.canais_so_documentos = j.at("CANAIS_RESTRITOS").at("SO_DOCUMENTOS").get<std::vector<dpp::snowflake>>();
    }
    catch (const json::exception& e) {
        std::cerr << "ERRO FATAL: Erro ao ler config.json: " << e.what() << std::endl;
        std::cerr << "Verifique se o arquivo config.json esta formatado corretamente." << std::endl;
        log_to_file("ERRO FATAL: Erro ao ler config.json: " + std::string(e.what()));
        return false;
    }
    if (config.token == "SEU_TOKEN_AQUI" || config.token.empty()) {
        std::cerr << "ERRO FATAL: O token do bot nao foi definido no arquivo config.json!" << std::endl;
        log_to_file("ERRO FATAL: Token do bot nao definido no config.json.");
        return false;
    }
    return true;
}

void to_json(json& j, const Solicitacao& s) { j = json{ {"id", s.id}, {"id_usuario_responsavel", s.id_usuario_responsavel}, {"texto", s.texto}, {"prazo", s.prazo}, {"nome_usuario_responsavel", s.nome_usuario_responsavel}, {"tipo", s.tipo} }; }
void from_json(const json& j, Solicitacao& s) { j.at("id").get_to(s.id); j.at("id_usuario_responsavel").get_to(s.id_usuario_responsavel); j.at("texto").get_to(s.texto); j.at("prazo").get_to(s.prazo); j.at("nome_usuario_responsavel").get_to(s.nome_usuario_responsavel); if (j.contains("tipo")) { j.at("tipo").get_to(s.tipo); } else if (j.contains("is_pedido") && j.at("is_pedido").get<bool>()) { s.tipo = PEDIDO; } else { s.tipo = DEMANDA; } }
void to_json(json& j, const Lead& l) { j = json{ {"id", l.id}, {"origem", l.origem}, {"data_contato", l.data_contato}, {"hora_contato", l.hora_contato}, {"nome", l.nome}, {"contato", l.contato}, {"area_atuacao", l.area_atuacao}, {"conhece_formato", l.conhece_formato}, {"veio_campanha", l.veio_campanha}, {"respondido", l.respondido}, {"data_hora_resposta", l.data_hora_resposta}, {"problema_contato", l.problema_contato}, {"observacoes", l.observacoes}, {"status_conversa", l.status_conversa}, {"status_followup", l.status_followup}, {"unidade", l.unidade}, {"convidou_visita", l.convidou_visita}, {"agendou_visita", l.agendou_visita}, {"tipo_fechamento", l.tipo_fechamento}, {"teve_adicionais", l.teve_adicionais}, {"valor_fechamento", l.valor_fechamento}, {"print_final_conversa", l.print_final_conversa}, {"criado_por", l.criado_por} }; }
void from_json(const json& j, Lead& l) { j.at("id").get_to(l.id); j.at("origem").get_to(l.origem); j.at("data_contato").get_to(l.data_contato); j.at("hora_contato").get_to(l.hora_contato); j.at("nome").get_to(l.nome); j.at("contato").get_to(l.contato); j.at("area_atuacao").get_to(l.area_atuacao); j.at("conhece_formato").get_to(l.conhece_formato); j.at("veio_campanha").get_to(l.veio_campanha); j.at("respondido").get_to(l.respondido); j.at("data_hora_resposta").get_to(l.data_hora_resposta); j.at("problema_contato").get_to(l.problema_contato); j.at("observacoes").get_to(l.observacoes); j.at("status_conversa").get_to(l.status_conversa); j.at("status_followup").get_to(l.status_followup); j.at("unidade").get_to(l.unidade); j.at("convidou_visita").get_to(l.convidou_visita); j.at("agendou_visita").get_to(l.agendou_visita); j.at("tipo_fechamento").get_to(l.tipo_fechamento); j.at("teve_adicionais").get_to(l.teve_adicionais); j.at("valor_fechamento").get_to(l.valor_fechamento); j.at("print_final_conversa").get_to(l.print_final_conversa); if (j.contains("criado_por")) { j.at("criado_por").get_to(l.criado_por); } }
void to_json(json& j, const Compra& c) { j = json{ {"id", c.id}, {"item", c.item}, {"observacao", c.observacao}, {"solicitado_por", c.solicitado_por}, {"data_solicitacao", c.data_solicitacao} }; }
void from_json(const json& j, Compra& c) { j.at("id").get_to(c.id); j.at("item").get_to(c.item); j.at("observacao").get_to(c.observacao); j.at("solicitado_por").get_to(c.solicitado_por); j.at("data_solicitacao").get_to(c.data_solicitacao); }
void to_json(json& j, const Visita& v) { j = json{ {"id", v.id}, {"quem_marcou_id", v.quem_marcou_id}, {"quem_marcou_nome", v.quem_marcou_nome}, {"doutor", v.doutor}, {"area", v.area}, {"data", v.data}, {"horario", v.horario}, {"unidade", v.unidade}, {"telefone", v.telefone} }; }
void from_json(const json& j, Visita& v) { j.at("id").get_to(v.id); j.at("quem_marcou_id").get_to(v.quem_marcou_id); j.at("quem_marcou_nome").get_to(v.quem_marcou_nome); j.at("doutor").get_to(v.doutor); j.at("area").get_to(v.area); j.at("data").get_to(v.data); j.at("horario").get_to(v.horario); j.at("unidade").get_to(v.unidade); j.at("telefone").get_to(v.telefone); }

void save_database() { std::ofstream file(DATABASE_FILE); json j = banco_de_dados; file << j.dump(4); file.close(); }
void load_database() {
    std::ifstream file(DATABASE_FILE);
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            if (!j.is_null() && !j.empty()) {
                banco_de_dados = j.get<std::map<uint64_t, Solicitacao>>();
            }
        }
        catch (const json::exception& e) {
            std::string error_msg = "AVISO: Erro ao ler " + DATABASE_FILE + ": " + e.what() + ". O arquivo pode estar corrompido. Renomeando para backup e continuando com DB vazio.";
            std::cerr << error_msg << std::endl;
            log_to_file(error_msg);
            file.close();
            std::string backup_name = DATABASE_FILE + ".bak";
            std::rename(DATABASE_FILE.c_str(), backup_name.c_str());
            banco_de_dados.clear();
            return;
        }
        file.close();
    }
}
uint64_t gerar_codigo() { std::random_device rd; std::mt19937_64 gen(rd()); std::uniform_int_distribution<uint64_t> distrib(1000000000, 9999999999); return distrib(gen); }
void save_leads_database() { std::ofstream file(LEADS_DATABASE_FILE); json j = leads_database; file << j.dump(4); file.close(); }
void load_leads_database() {
    std::ifstream file(LEADS_DATABASE_FILE);
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            if (!j.is_null() && !j.empty()) {
                leads_database = j.get<std::map<uint64_t, Lead>>();
            }
        }
        catch (const json::exception& e) {
            std::string error_msg = "AVISO: Erro ao ler " + LEADS_DATABASE_FILE + ": " + e.what() + ". O arquivo pode estar corrompido. Renomeando para backup e continuando com DB vazio.";
            std::cerr << error_msg << std::endl;
            log_to_file(error_msg);
            file.close();
            std::string backup_name = LEADS_DATABASE_FILE + ".bak";
            std::rename(LEADS_DATABASE_FILE.c_str(), backup_name.c_str());
            leads_database.clear();
            return;
        }
        file.close();
    }
}
void save_compras_database() { std::ofstream file(COMPRAS_DATABASE_FILE); json j = compras_database; file << j.dump(4); file.close(); }
void load_compras_database() {
    std::ifstream file(COMPRAS_DATABASE_FILE);
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            if (!j.is_null() && !j.empty()) {
                compras_database = j.get<std::map<uint64_t, Compra>>();
            }
        }
        catch (const json::exception& e) {
            std::string error_msg = "AVISO: Erro ao ler " + COMPRAS_DATABASE_FILE + ": " + e.what() + ". O arquivo pode estar corrompido. Renomeando para backup e continuando com DB vazio.";
            std::cerr << error_msg << std::endl;
            log_to_file(error_msg);
            file.close();
            std::string backup_name = COMPRAS_DATABASE_FILE + ".bak";
            std::rename(COMPRAS_DATABASE_FILE.c_str(), backup_name.c_str());
            compras_database.clear();
            return;
        }
        file.close();
    }
}
void save_visitas_database() { std::ofstream file(VISITAS_DATABASE_FILE); json j = visitas_database; file << j.dump(4); file.close(); }
void load_visitas_database() {
    std::ifstream file(VISITAS_DATABASE_FILE);
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            if (!j.is_null() && !j.empty()) {
                visitas_database = j.get<std::map<uint64_t, Visita>>();
            }
        }
        catch (const json::exception& e) {
            std::string error_msg = "AVISO: Erro ao ler " + VISITAS_DATABASE_FILE + ": " + e.what() + ". O arquivo pode estar corrompido. Renomeando para backup e continuando com DB vazio.";
            std::cerr << error_msg << std::endl;
            log_to_file(error_msg);
            file.close();
            std::string backup_name = VISITAS_DATABASE_FILE + ".bak";
            std::rename(VISITAS_DATABASE_FILE.c_str(), backup_name.c_str());
            visitas_database.clear();
            return;
        }
        file.close();
    }
}


void gerar_e_enviar_planilha_leads(const dpp::slashcommand_t& event);
void gerar_e_enviar_planilha_compras(const dpp::slashcommand_t& event);
void gerar_e_enviar_planilha_visitas(const dpp::slashcommand_t& event);

void enviar_lembretes(dpp::cluster& bot) {
    bot.log(dpp::ll_info, "Verificando e enviando lembretes de demandas...");
    std::map<dpp::snowflake, std::vector<Solicitacao>> solicitacoes_por_usuario;
    for (auto const& [id, solicitacao] : banco_de_dados) {
        if (solicitacao.tipo != PEDIDO) { // Pedidos não geram lembretes
            solicitacoes_por_usuario[solicitacao.id_usuario_responsavel].push_back(solicitacao);
        }
    }
    for (auto const& [id_usuario, lista_solicitacoes] : solicitacoes_por_usuario) {
        if (!lista_solicitacoes.empty()) {
            std::string conteudo_dm = "Olá! 👋 Este é um lembrete das suas pendências ativas:\n\n";
            for (const auto& solicitacao : lista_solicitacoes) {
                std::string tipo_str = (solicitacao.tipo == DEMANDA) ? "Demanda" : "Lembrete";
                conteudo_dm += "**" + tipo_str + ":**\n";
                conteudo_dm += "**Código:** `" + std::to_string(solicitacao.id) + "`\n";
                conteudo_dm += "**Descrição:** " + solicitacao.texto + "\n";
                conteudo_dm += "**Prazo:** " + solicitacao.prazo + "\n\n";
            }
            conteudo_dm += "Para finalizar, use o comando correspondente no servidor (`/finalizar_demanda` ou `/finalizar_lembrete`).";
            dpp::message dm(conteudo_dm);
            bot.direct_message_create(id_usuario, dm);
        }
    }
}

void handle_visitas(const dpp::slashcommand_t& event);
void handle_cancelar_visita(const dpp::slashcommand_t& event);
void handle_demanda_pedido(const dpp::slashcommand_t& event);
void handle_finalizar_solicitacao(const dpp::slashcommand_t& event);
void handle_limpar_demandas(const dpp::slashcommand_t& event);
void handle_lista_demandas(const dpp::slashcommand_t& event);
void handle_adicionar_lead(const dpp::slashcommand_t& event);
void handle_modificar_lead(const dpp::slashcommand_t& event);
void handle_listar_leads(const dpp::slashcommand_t& event);
void handle_gerar_planilha(const dpp::slashcommand_t& event);
void handle_ver_lead(const dpp::slashcommand_t& event);
void handle_limpar_leads(const dpp::slashcommand_t& event);
void handle_deletar_lead(const dpp::slashcommand_t& event);
void handle_placa(const dpp::slashcommand_t& event);
void handle_finalizar_placa(const dpp::slashcommand_t& event);
void handle_adicionar_compra(const dpp::slashcommand_t& event);
void handle_lembrete(const dpp::slashcommand_t& event);

void signal_handler(int signum) {
    std::string msg = "Bot finalizando via sinal " + std::to_string(signum) + " (Ctrl+C)...";
    std::cout << msg << std::endl;
    log_to_file(msg);
    if (bot_ptr) {
        bot_ptr->shutdown();
    }
}

int main() {
    log_to_file("--- Bot iniciado ---");
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (!load_config()) {
        std::cout << "Pressione Enter para sair." << std::endl;
        std::cin.get();
        log_to_file("--- Bot finalizado devido a erro de configuracao ---");
        return 1;
    }
    load_database();
    load_leads_database();
    load_compras_database();
    load_visitas_database();

    std::map<std::string, std::function<void(const dpp::slashcommand_t&)>> command_handler;
    command_handler["visitas"] = handle_visitas;
    command_handler["cancelar_visita"] = handle_cancelar_visita;
    command_handler["demanda"] = handle_demanda_pedido;
    command_handler["pedido"] = handle_demanda_pedido;
    command_handler["finalizar_demanda"] = handle_finalizar_solicitacao;
    command_handler["finalizar_pedido"] = handle_finalizar_solicitacao;
    command_handler["cancelar_pedido"] = handle_finalizar_solicitacao;
    command_handler["finalizar_lembrete"] = handle_finalizar_solicitacao;
    command_handler["limpar_demandas"] = handle_limpar_demandas;
    command_handler["lista_demandas"] = handle_lista_demandas;
    command_handler["adicionar_lead"] = handle_adicionar_lead;
    command_handler["modificar_lead"] = handle_modificar_lead;
    command_handler["listar_leads"] = handle_listar_leads;
    command_handler["gerar_planilha"] = handle_gerar_planilha;
    command_handler["ver_lead"] = handle_ver_lead;
    command_handler["limpar_leads"] = handle_limpar_leads;
    command_handler["deletar_lead"] = handle_deletar_lead;
    command_handler["placa"] = handle_placa;
    command_handler["finalizar_placa"] = handle_finalizar_placa;
    command_handler["adicionar_compra"] = handle_adicionar_compra;
    command_handler["lembrete"] = handle_lembrete;

    dpp::cluster bot(config.token, dpp::i_guilds | dpp::i_guild_messages | dpp::i_guild_members | dpp::i_guild_message_reactions | dpp::i_message_content);
    bot_ptr = &bot;

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&command_handler](const dpp::slashcommand_t& event) {
        dpp::cluster* bot = event.from()->creator;
        dpp::user author = event.command.get_issuing_user();
        dpp::channel* c = dpp::find_channel(event.command.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.command.channel_id);
        dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);
        std::string options_str;
        for (const auto& opt : cmd_data.options) {
            options_str += " `" + opt.name + "`";
        }
        std::string log_message = std::string("`[") + format_timestamp(event.command.id.get_creation_time()) + "]` `[COMANDO]` `(#" + channel_name + ")` `" + author.username + "` `(" + std::to_string(author.id) + ")`: /" + event.command.get_command_name() + options_str;
        bot->message_create(dpp::message(config.canal_logs, log_message));
        auto it = command_handler.find(event.command.get_command_name());
        if (it != command_handler.end()) {
            it->second(event);
        }
        });

    bot.on_message_create([](const dpp::message_create_t& event) {
        if (event.msg.author.is_bot() || event.msg.channel_id == config.canal_logs) { return; }

        dpp::cluster* bot = event.from()->creator;

        if (event.msg.channel_id == config.canal_sugestoes || event.msg.channel_id == config.canal_bugs) {
            if (event.msg.content.empty()) {
                return;
            }

            bool is_sugestao = (event.msg.channel_id == config.canal_sugestoes);

            dpp::embed embed = dpp::embed()
                .set_author(event.msg.author.username, "", event.msg.author.get_avatar_url())
                .set_title(is_sugestao ? "💡 Nova Sugestão" : "🐞 Novo Bug Reportado")
                .set_description(event.msg.content)
                .set_color(is_sugestao ? dpp::colors::yellow : dpp::colors::red)
                .set_timestamp(time(0));

            dpp::message msg(event.msg.channel_id, embed);

            bot->message_create(msg, [bot, event](const dpp::confirmation_callback_t& callback) {
                if (!callback.is_error()) {
                    auto new_message = callback.get<dpp::message>();
                    bot->message_add_reaction(new_message.id, new_message.channel_id, "👍");
                    bot->message_add_reaction(new_message.id, new_message.channel_id, "👎");
                    bot->message_delete(event.msg.id, event.msg.channel_id);
                }
                });
            return;
        }

        dpp::channel* c = dpp::find_channel(event.msg.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.msg.channel_id);
        std::string log_message = std::string("`[") + format_timestamp(event.msg.sent) + "]` `(#" + channel_name + ")` `" + event.msg.author.username + "` `(" + std::to_string(event.msg.author.id) + ")`: " + event.msg.content;
        if (!event.msg.attachments.empty()) { for (const auto& att : event.msg.attachments) { log_message += " [Anexo: " + att.url + "]"; } }
        bot->message_create(dpp::message(config.canal_logs, log_message));
        dpp::snowflake channel_id = event.msg.channel_id;
        if (std::find(config.canais_so_imagens.begin(), config.canais_so_imagens.end(), channel_id) != config.canais_so_imagens.end()) { if (event.msg.attachments.empty() && event.msg.embeds.empty()) { bot->message_delete(event.msg.id, event.msg.channel_id); bot->direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de imagens foi removida. Por favor, envie apenas imagens ou GIFs nesse canal.")); } }
        if (std::find(config.canais_so_links.begin(), config.canais_so_links.end(), channel_id) != config.canais_so_links.end()) { if (event.msg.content.find("http") == std::string::npos) { bot->message_delete(event.msg.id, event.msg.channel_id); bot->direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de links foi removida. Por favor, envie apenas links nesse canal.")); } }
        if (std::find(config.canais_so_documentos.begin(), config.canais_so_documentos.end(), channel_id) != config.canais_so_documentos.end()) { if (event.msg.attachments.empty()) { bot->message_delete(event.msg.id, event.msg.channel_id); bot->direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de documentos foi removida. Por favor, envie apenas arquivos nesse canal.")); } }
        });

    bot.on_message_update([](const dpp::message_update_t& event) {
        if (event.msg.author.is_bot() || event.msg.channel_id == config.canal_logs) { return; }
        dpp::cluster* bot = event.from()->creator;
        dpp::channel* c = dpp::find_channel(event.msg.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.msg.channel_id);
        std::string log_message = std::string("`[") + format_timestamp(event.msg.edited) + "]` `[EDITADA]` `(#" + channel_name + ")` `" + event.msg.author.username + "` `(" + std::to_string(event.msg.author.id) + ")`: " + event.msg.content;
        bot->message_create(dpp::message(config.canal_logs, log_message));
        });

    bot.on_message_delete([](const dpp::message_delete_t& event) {
        if (event.channel_id == config.canal_logs) { return; }
        dpp::cluster* bot = event.from()->creator;
        dpp::channel* c = dpp::find_channel(event.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.channel_id);
        std::string log_message = std::string("`[") + format_timestamp(std::time(nullptr)) + "]` `[DELETADA]` `(#" + channel_name + ")` `Mensagem ID: " + std::to_string(event.id) + "`";
        bot->message_create(dpp::message(config.canal_logs, log_message));
        });

    bot.on_message_reaction_add([](const dpp::message_reaction_add_t& event) {
        if (event.reacting_user.is_bot() || event.channel_id == config.canal_logs) { return; }
        dpp::cluster* bot = event.from()->creator;
        dpp::channel* c = dpp::find_channel(event.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.channel_id);
        std::string log_message = std::string("`[") + format_timestamp(std::time(nullptr)) + "]` `[REAÇÃO]` `(#" + channel_name + ")` `" + event.reacting_user.username + "` `(" + std::to_string(event.reacting_user.id) + ")` reagiu com " + event.reacting_emoji.get_mention() + " à mensagem " + dpp::utility::message_url(event.reacting_guild.id, event.channel_id, event.message_id);
        bot->message_create(dpp::message(config.canal_logs, log_message));
        });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand visitas_cmd("visitas", "Agenda uma nova visita.", bot.me.id);
            visitas_cmd.add_option(dpp::command_option(dpp::co_user, "quem_marcou", "Quem está agendando a visita.", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome do Dr(a).", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Área de atuação do Dr(a).", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "data", "Data da visita (ex: 10/10/2025).", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "horario", "Horário da visita (ex: 14h).", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade da visita.", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "telefone", "Telefone de contato.", true));

            dpp::slashcommand cancelar_visita_cmd("cancelar_visita", "Cancela uma visita agendada.", bot.me.id);
            cancelar_visita_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código da visita a ser cancelada.", true));

            dpp::slashcommand demanda_cmd("demanda", "Cria uma nova demanda para um usuário.", bot.me.id);
            demanda_cmd.add_option(dpp::command_option(dpp::co_user, "responsavel", "O usuário que receberá a demanda.", true));
            demanda_cmd.add_option(dpp::command_option(dpp::co_string, "demanda", "Descrição detalhada da demanda.", true));
            demanda_cmd.add_option(dpp::command_option(dpp::co_string, "prazo", "Prazo para a conclusão da demanda.", true));
            demanda_cmd.set_default_permissions(0);
            demanda_cmd.add_permission(dpp::command_permission(config.cargo_permitido, dpp::cpt_role, true));
            dpp::slashcommand finalizar_demanda_cmd("finalizar_demanda", "Finaliza uma demanda existente.", bot.me.id);
            finalizar_demanda_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código de 10 dígitos da demanda.", true));
            finalizar_demanda_cmd.add_option(dpp::command_option(dpp::co_attachment, "prova", "Uma imagem que comprova a finalização.", false));
            dpp::slashcommand limpar_cmd("limpar_demandas", "Limpa TODAS as demandas e pedidos ativos.", bot.me.id);
            limpar_cmd.default_member_permissions = dpp::p_administrator;
            dpp::slashcommand lista_cmd("lista_demandas", "Mostra todas as solicitações pendentes de um usuário.", bot.me.id);
            lista_cmd.add_option(dpp::command_option(dpp::co_user, "usuario", "O usuário que você quer consultar.", true));
            dpp::slashcommand pedido_cmd("pedido", "Cria um novo pedido para um usuário.", bot.me.id);
            pedido_cmd.add_option(dpp::command_option(dpp::co_user, "responsavel", "O usuário que receberá o pedido.", true));
            pedido_cmd.add_option(dpp::command_option(dpp::co_string, "pedido", "Descrição detalhada do pedido.", true));
            dpp::slashcommand finalizar_pedido_cmd("finalizar_pedido", "Finaliza um pedido existente.", bot.me.id);
            finalizar_pedido_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código do pedido.", true));
            finalizar_pedido_cmd.add_option(dpp::command_option(dpp::co_attachment, "prova", "Uma imagem que comprova a finalização.", false));
            dpp::slashcommand cancelar_pedido_cmd("cancelar_pedido", "Cancela um pedido existente.", bot.me.id);
            cancelar_pedido_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código do pedido.", true));
            cancelar_pedido_cmd.set_default_permissions(0);
            cancelar_pedido_cmd.add_permission(dpp::command_permission(config.cargo_permitido, dpp::cpt_role, true));
            dpp::slashcommand adicionar_lead_cmd("adicionar_lead", "Adiciona um novo lead ao banco de dados.", bot.me.id);
            adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "origem", "De onde o lead veio.", true).add_choice(dpp::command_option_choice("Whatsapp", std::string("Whatsapp"))).add_choice(dpp::command_option_choice("Instagram", std::string("Instagram"))).add_choice(dpp::command_option_choice("Facebook", std::string("Facebook"))).add_choice(dpp::command_option_choice("Site", std::string("Site"))));
            adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "data", "Data do primeiro contato (DD/MM/AAAA).", true));
            adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "hora", "Hora do primeiro contato (HH:MM).", true));
            adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "nome", "Nome do lead.", true));
            adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Area de atuacao do lead.", true));
            adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade de interesse.", true).add_choice(dpp::command_option_choice("Campo Belo", std::string("Campo Belo"))).add_choice(dpp::command_option_choice("Tatuape", std::string("Tatuape"))).add_choice(dpp::command_option_choice("Ambas", std::string("Ambas"))));
            adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "conhece_formato", "O lead ja conhece o formato de coworking?", true).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
            adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "veio_campanha", "O lead veio de alguma campanha paga?", true).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
            adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "contato", "Telefone ou e-mail do lead.", false));
            dpp::slashcommand modificar_lead_cmd("modificar_lead", "Modifica ou adiciona informacoes a um lead existente.", bot.me.id);
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O codigo unico do lead a ser modificado.", true));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "MOTIVO da modificacao (obrigatorio).", true));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "nome", "Altera o nome do lead.", false));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "contato", "Altera o numero ou email de contato do lead.", false));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Altera a area de atuacao do lead.", false));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Altera a unidade de interesse.", false).add_choice(dpp::command_option_choice("Campo Belo", std::string("Campo Belo"))).add_choice(dpp::command_option_choice("Tatuape", std::string("Tatuape"))).add_choice(dpp::command_option_choice("Ambas", std::string("Ambas"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "conhece_formato", "Altera se o lead conhece o formato.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "veio_campanha", "Altera se o lead veio de campanha.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "respondido", "Marcar/desmarcar se o lead foi respondido.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "status_conversa", "Altera o status da conversa.", false).add_choice(dpp::command_option_choice("Orcamento enviado", std::string("Orcamento enviado"))).add_choice(dpp::command_option_choice("Desqualificado", std::string("Desqualificado"))).add_choice(dpp::command_option_choice("Em negociacao", std::string("Em negociacao"))).add_choice(dpp::command_option_choice("Fechamento", std::string("Fechamento"))).add_choice(dpp::command_option_choice("Sem Resposta", std::string("Sem Resposta"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "status_followup", "Altera o status do followup.", false).add_choice(dpp::command_option_choice("Primeiro contato", std::string("Primeiro contato"))).add_choice(dpp::command_option_choice("Follow Up 1", std::string("Follow Up 1"))).add_choice(dpp::command_option_choice("Follow Up 2", std::string("Follow Up 2"))).add_choice(dpp::command_option_choice("Follow Up 3", std::string("Follow Up 3"))).add_choice(dpp::command_option_choice("Follow Up 4", std::string("Follow Up 4"))).add_choice(dpp::command_option_choice("Contato encerrado", std::string("Contato encerrado"))).add_choice(dpp::command_option_choice("Retornar contato", std::string("Retornar contato"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "problema_contato", "Marcar/desmarcar problema no contato.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "convidou_visita", "Marcar se convidou para visitar.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "agendou_visita", "Marcar se agendou visita.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "tipo_fechamento", "Define o tipo de fechamento.", false).add_choice(dpp::command_option_choice("Diaria", std::string("Diaria"))).add_choice(dpp::command_option_choice("Pacote de horas", std::string("Pacote de horas"))).add_choice(dpp::command_option_choice("Mensal", std::string("Mensal"))).add_choice(dpp::command_option_choice("Hora avulsa", std::string("Hora avulsa"))).add_choice(dpp::command_option_choice("Outro", std::string("Outro"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "teve_adicionais", "Marcar se teve adicionais no fechamento.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_number, "valor_fechamento", "Define o valor final do fechamento.", false));
            modificar_lead_cmd.add_option(dpp::command_option(dpp::co_attachment, "print_final", "Anexa o print do final da conversa.", false));
            dpp::slashcommand listar_leads_cmd("listar_leads", "Lista os leads com base em um filtro.", bot.me.id);
            listar_leads_cmd.add_option(dpp::command_option(dpp::co_string, "tipo", "O tipo de lista que voce quer ver.", true).add_choice(dpp::command_option_choice("Todos", std::string("todos"))).add_choice(dpp::command_option_choice("Nao Contatados", std::string("nao_contatados"))).add_choice(dpp::command_option_choice("Com Problema", std::string("com_problema"))).add_choice(dpp::command_option_choice("Nao Respondidos", std::string("nao_respondidos"))));

            dpp::slashcommand gerar_planilha_cmd("gerar_planilha", "Gera e envia uma planilha de dados.", bot.me.id);
            gerar_planilha_cmd.add_option(
                dpp::command_option(dpp::co_string, "tipo", "Qual planilha você deseja gerar.", true)
                .add_choice(dpp::command_option_choice("Leads", std::string("leads")))
                .add_choice(dpp::command_option_choice("Lista de Compras", std::string("compras")))
                .add_choice(dpp::command_option_choice("Visitas", std::string("visitas")))
            );

            dpp::slashcommand ver_lead_cmd("ver_lead", "Mostra todas as informacoes de um lead especifico.", bot.me.id);
            ver_lead_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O codigo do lead que voce quer ver.", true));
            dpp::slashcommand limpar_leads_cmd("limpar_leads", "Limpa TODOS os leads do banco de dados.", bot.me.id);
            limpar_leads_cmd.default_member_permissions = dpp::p_administrator;
            dpp::slashcommand deletar_lead_cmd("deletar_lead", "Deleta um lead permanentemente do banco de dados.", bot.me.id);
            deletar_lead_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O codigo do lead que voce deseja deletar.", true));
            deletar_lead_cmd.default_member_permissions = dpp::p_administrator;

            dpp::slashcommand placa_cmd("placa", "Solicita a criação de uma placa para um profissional.", bot.me.id);
            placa_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome do profissional para a placa.", true));
            placa_cmd.add_option(dpp::command_option(dpp::co_attachment, "imagem_referencia", "Uma imagem de referência (opcional).", false));

            dpp::slashcommand finalizar_placa_cmd("finalizar_placa", "Envia a arte finalizada de uma placa.", bot.me.id);
            finalizar_placa_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome do profissional da placa finalizada.", true));
            finalizar_placa_cmd.add_option(dpp::command_option(dpp::co_attachment, "arte_final", "A imagem da placa finalizada.", true));

            dpp::slashcommand adicionar_compra_cmd("adicionar_compra", "Adiciona um item à lista de compras.", bot.me.id);
            adicionar_compra_cmd.add_option(dpp::command_option(dpp::co_string, "item", "O que precisa ser comprado.", true));
            adicionar_compra_cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "Alguma observação sobre o item (marca, quantidade, etc).", false));

            dpp::slashcommand lembrete_cmd("lembrete", "Cria um lembrete pessoal para você mesmo.", bot.me.id);
            lembrete_cmd.add_option(dpp::command_option(dpp::co_string, "lembrete", "Descrição do que você precisa lembrar.", true));
            lembrete_cmd.add_option(dpp::command_option(dpp::co_string, "prazo", "Quando você precisa ser lembrado.", true));

            dpp::slashcommand finalizar_lembrete_cmd("finalizar_lembrete", "Finaliza um de seus lembretes pessoais.", bot.me.id);
            finalizar_lembrete_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código do lembrete a ser finalizado.", true));

            bot.guild_bulk_command_create({
                visitas_cmd, cancelar_visita_cmd, demanda_cmd, finalizar_demanda_cmd, limpar_cmd, lista_cmd,
                pedido_cmd, finalizar_pedido_cmd, cancelar_pedido_cmd,
                adicionar_lead_cmd, modificar_lead_cmd, listar_leads_cmd, gerar_planilha_cmd, ver_lead_cmd, limpar_leads_cmd, deletar_lead_cmd,
                placa_cmd, finalizar_placa_cmd, adicionar_compra_cmd, lembrete_cmd, finalizar_lembrete_cmd
                }, config.server_id);
        }
        static bool lembretes_enviados_hoje = false;
        bot.start_timer([&bot](dpp::timer t) {
            std::time_t tt = std::time(nullptr);
            std::tm* agora = std::localtime(&tt);
            if (agora->tm_hour == 9 && agora->tm_min == 0 && !lembretes_enviados_hoje) {
                enviar_lembretes(bot);
                lembretes_enviados_hoje = true;
            }
            if (agora->tm_hour != 9 && lembretes_enviados_hoje) {
                lembretes_enviados_hoje = false;
            }
            }, 60);
        });

    bot.start(dpp::st_wait);

    log_to_file("--- Bot finalizado com sucesso ---");
    return 0;
}

void handle_visitas(const dpp::slashcommand_t& event) {
    dpp::cluster* bot = event.from()->creator;
    dpp::user quem_marcou = event.command.get_resolved_user(std::get<dpp::snowflake>(event.get_parameter("quem_marcou")));

    Visita nova_visita;
    nova_visita.id = gerar_codigo();
    nova_visita.quem_marcou_id = quem_marcou.id;
    nova_visita.quem_marcou_nome = quem_marcou.username;
    nova_visita.doutor = std::get<std::string>(event.get_parameter("doutor"));
    nova_visita.area = std::get<std::string>(event.get_parameter("area"));
    nova_visita.data = std::get<std::string>(event.get_parameter("data"));
    nova_visita.horario = std::get<std::string>(event.get_parameter("horario"));
    nova_visita.unidade = std::get<std::string>(event.get_parameter("unidade"));
    nova_visita.telefone = std::get<std::string>(event.get_parameter("telefone"));

    visitas_database[nova_visita.id] = nova_visita;
    save_visitas_database();

    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::blue)
        .set_title("🔔 Nova Visita Agendada")
        .add_field("Agendado por", quem_marcou.get_mention(), true)
        .add_field("Nome do Dr(a).", nova_visita.doutor, true)
        .add_field("Área", nova_visita.area, true)
        .add_field("Data", nova_visita.data, true)
        .add_field("Horário", nova_visita.horario, true)
        .add_field("Unidade", nova_visita.unidade, true)
        .add_field("Telefone", nova_visita.telefone, false)
        .set_footer(dpp::embed_footer().set_text("Código da Visita: " + std::to_string(nova_visita.id)));

    dpp::message msg(config.canal_visitas, embed);
    bot->message_create(msg);
    event.reply("✅ Visita agendada e salva com sucesso! Código: `" + std::to_string(nova_visita.id) + "`");
}

void handle_cancelar_visita(const dpp::slashcommand_t& event) {
    dpp::cluster* bot = event.from()->creator;
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));

    if (visitas_database.count(codigo)) {
        Visita visita_cancelada = visitas_database[codigo];
        visitas_database.erase(codigo);
        save_visitas_database();

        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::red)
            .set_title("❌ Visita Cancelada")
            .add_field("Dr(a).", visita_cancelada.doutor, true)
            .add_field("Data", visita_cancelada.data + " às " + visita_cancelada.horario, true)
            .set_footer(dpp::embed_footer().set_text("Cancelado por: " + event.command.get_issuing_user().username));

        bot->message_create(dpp::message(config.canal_visitas, embed));
        event.reply("✅ Visita `" + std::to_string(codigo) + "` cancelada com sucesso!");
    }
    else {
        event.reply(dpp::message("Código de visita não encontrado.").set_flags(dpp::m_ephemeral));
    }
}

void handle_demanda_pedido(const dpp::slashcommand_t& event) {
    dpp::cluster* bot = event.from()->creator;
    bool is_pedido = event.command.get_command_name() == "pedido";
    dpp::snowflake responsavel_id = std::get<dpp::snowflake>(event.get_parameter("responsavel"));
    dpp::user responsavel = event.command.get_resolved_user(responsavel_id);
    std::string texto = std::get<std::string>(event.get_parameter(is_pedido ? "pedido" : "demanda"));
    std::string prazo = is_pedido ? "N/A" : std::get<std::string>(event.get_parameter("prazo"));

    uint64_t novo_codigo = gerar_codigo();
    Solicitacao nova_solicitacao;
    nova_solicitacao.id = novo_codigo;
    nova_solicitacao.id_usuario_responsavel = responsavel.id;
    nova_solicitacao.nome_usuario_responsavel = responsavel.username;
    nova_solicitacao.texto = texto;
    nova_solicitacao.prazo = prazo;
    nova_solicitacao.tipo = is_pedido ? PEDIDO : DEMANDA;

    banco_de_dados[novo_codigo] = nova_solicitacao;
    save_database();

    dpp::embed embed = dpp::embed().set_color(is_pedido ? dpp::colors::light_sea_green : dpp::colors::orange).set_title(is_pedido ? "📢 Novo Pedido Criado" : "❗ Nova Demanda Criada").add_field("Código", std::to_string(novo_codigo), false).add_field("Responsável", responsavel.get_mention(), true).add_field("Criado por", event.command.get_issuing_user().get_mention(), true);
    if (!is_pedido) { embed.add_field("Prazo de Entrega", prazo, true); }
    embed.add_field(is_pedido ? "Pedido" : "Demanda", texto, false);
    dpp::message msg;
    msg.set_channel_id(is_pedido ? config.canal_aviso_pedidos : config.canal_aviso_demandas);
    msg.set_content(std::string("Nova solicitação para o responsável: ") + responsavel.get_mention());
    msg.add_embed(embed);
    bot->message_create(msg);
    dpp::embed dm_embed = dpp::embed().set_color(is_pedido ? dpp::colors::light_sea_green : dpp::colors::orange).set_title(is_pedido ? "📬 Você recebeu um novo pedido!" : "📬 Você recebeu uma nova demanda!").add_field("Código", std::to_string(novo_codigo), false).add_field(is_pedido ? "Pedido" : "Demanda", texto, false);
    if (!is_pedido) { dm_embed.add_field("Prazo de Entrega", prazo, false); }
    dm_embed.set_footer(dpp::embed_footer().set_text(std::string("Solicitado por: ") + event.command.get_issuing_user().username));
    dpp::message dm_message;
    dm_message.add_embed(dm_embed);
    bot->direct_message_create(responsavel.id, dm_message);
    dpp::embed log_dm_embed = dpp::embed().set_color(dpp::colors::black).set_title("LOG: Mensagem Privada Enviada").add_field("Tipo", (is_pedido ? "Novo Pedido" : "Nova Demanda"), true).add_field("Destinatário", "`" + responsavel.username + "` (`" + std::to_string(responsavel.id) + "`)", true).add_field("Autor da Solicitação", "`" + event.command.get_issuing_user().username + "` (`" + std::to_string(event.command.get_issuing_user().id) + "`)", false).set_description(dm_embed.description);
    for (const auto& field : dm_embed.fields) { log_dm_embed.add_field(field.name, field.value, field.is_inline); }
    bot->message_create(dpp::message(config.canal_logs, log_dm_embed));
    event.reply(std::string(is_pedido ? "Pedido" : "Demanda") + " criado e notificado com sucesso! Código: `" + std::to_string(novo_codigo) + "`");
}

void handle_finalizar_solicitacao(const dpp::slashcommand_t& event) {
    dpp::cluster* bot = event.from()->creator;
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));

    if (banco_de_dados.count(codigo)) {
        Solicitacao item_concluido = banco_de_dados[codigo];
        banco_de_dados.erase(codigo);
        save_database();

        std::string tipo_str;
        std::string title;
        dpp::snowflake channel_id;

        switch (item_concluido.tipo) {
        case DEMANDA:
            tipo_str = "Demanda";
            title = "✅ Demanda Finalizada";
            channel_id = config.canal_finalizadas;
            break;
        case PEDIDO:
            tipo_str = "Pedido";
            title = (event.command.get_command_name() == "cancelar_pedido") ? "❌ Pedido Cancelado" : "✅ Pedido Finalizado";
            channel_id = config.canal_pedidos_concluidos;
            break;
        case LEMBRETE:
            tipo_str = "Lembrete";
            title = "✅ Lembrete Concluído";
            event.reply("✅ Lembrete `" + std::to_string(codigo) + "` finalizado com sucesso!");
            return;
        }

        dpp::embed embed = dpp::embed()
            .set_color((title.find("❌") != std::string::npos) ? dpp::colors::red : dpp::colors::dark_green)
            .set_title(title)
            .add_field("Código", std::to_string(item_concluido.id), false)
            .add_field("Responsável", "<@" + std::to_string(item_concluido.id_usuario_responsavel) + ">", true)
            .add_field("Finalizado por", event.command.get_issuing_user().get_mention(), true)
            .add_field(tipo_str, item_concluido.texto, false);

        if (event.command.get_command_name() != "cancelar_pedido") {
            auto param_anexo = event.get_parameter("prova");
            if (auto attachment_id_ptr = std::get_if<dpp::snowflake>(&param_anexo)) {
                dpp::attachment anexo = event.command.get_resolved_attachment(*attachment_id_ptr);
                if (anexo.content_type.find("image/") == 0) { embed.set_image(anexo.url); }
            }
        }

        dpp::message msg(channel_id, embed);
        bot->message_create(msg);
        event.reply(tipo_str + " `" + std::to_string(codigo) + "` finalizado com sucesso!");
    }
    else {
        event.reply(dpp::message("Código não encontrado.").set_flags(dpp::m_ephemeral));
    }
}

void handle_limpar_demandas(const dpp::slashcommand_t& event) {
    banco_de_dados.clear();
    save_database();
    event.reply(dpp::message("🧹 Todas as demandas, pedidos e lembretes ativos foram limpos.").set_flags(dpp::m_ephemeral));
}

void handle_lista_demandas(const dpp::slashcommand_t& event) {
    dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("usuario"));
    dpp::user usuario_alvo = event.command.get_resolved_user(user_id);

    std::vector<Solicitacao> solicitacoes_do_usuario;
    for (auto const& [id, solicitacao] : banco_de_dados) {
        if (solicitacao.id_usuario_responsavel == usuario_alvo.id) {
            solicitacoes_do_usuario.push_back(solicitacao);
        }
    }

    std::sort(solicitacoes_do_usuario.begin(), solicitacoes_do_usuario.end(), [](const Solicitacao& a, const Solicitacao& b) {
        return a.tipo < b.tipo;
        });

    std::string lista_texto;
    if (!solicitacoes_do_usuario.empty()) {
        for (const auto& solicitacao : solicitacoes_do_usuario) {
            std::string tipo_str;
            switch (solicitacao.tipo) {
            case DEMANDA: tipo_str = "Demanda"; break;
            case PEDIDO:  tipo_str = "Pedido"; break;
            case LEMBRETE:tipo_str = "Lembrete"; break;
            }
            lista_texto += "**" + tipo_str + "**\n";
            lista_texto += "**Código:** `" + std::to_string(solicitacao.id) + "`\n";
            lista_texto += "**Descrição:** " + solicitacao.texto + "\n";
            if (solicitacao.tipo != PEDIDO) {
                lista_texto += "**Prazo:** " + solicitacao.prazo + "\n";
            }
            lista_texto += "\n";
        }
    }

    dpp::embed embed;
    embed.set_color(dpp::colors::white).set_title(std::string("📋 Solicitações Pendentes de ") + usuario_alvo.username);
    if (!lista_texto.empty()) {
        embed.set_description(lista_texto);
    }
    else {
        embed.set_description("Este usuário não possui nenhuma demanda, pedido ou lembrete pendente.");
    }
    event.reply(dpp::message().add_embed(embed));
}


void handle_adicionar_lead(const dpp::slashcommand_t& event) {
    dpp::user author = event.command.get_issuing_user();
    Lead novo_lead;
    novo_lead.id = gerar_codigo();
    novo_lead.origem = std::get<std::string>(event.get_parameter("origem"));
    novo_lead.data_contato = std::get<std::string>(event.get_parameter("data"));
    novo_lead.hora_contato = std::get<std::string>(event.get_parameter("hora"));
    novo_lead.nome = std::get<std::string>(event.get_parameter("nome"));
    novo_lead.area_atuacao = std::get<std::string>(event.get_parameter("area"));
    novo_lead.unidade = std::get<std::string>(event.get_parameter("unidade"));
    novo_lead.criado_por = author.username;
    novo_lead.conhece_formato = (std::get<std::string>(event.get_parameter("conhece_formato")) == "sim");
    novo_lead.veio_campanha = (std::get<std::string>(event.get_parameter("veio_campanha")) == "sim");
    auto contato_param = event.get_parameter("contato");
    if (std::holds_alternative<std::string>(contato_param)) {
        novo_lead.contato = std::get<std::string>(contato_param);
    }
    leads_database[novo_lead.id] = novo_lead;
    save_leads_database();
    dpp::embed embed = dpp::embed().set_color(dpp::colors::green).set_title("✅ Novo Lead Adicionado").add_field("Nome", novo_lead.nome, true).add_field("Origem", novo_lead.origem, true).add_field("Contato", novo_lead.contato.empty() ? "Nao informado" : novo_lead.contato, true).set_footer(dpp::embed_footer().set_text("Codigo do Lead: " + std::to_string(novo_lead.id)));
    event.reply(dpp::message().add_embed(embed));
}

void handle_modificar_lead(const dpp::slashcommand_t& event) {
    dpp::user author = event.command.get_issuing_user();
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    if (leads_database.count(codigo)) {
        Lead& lead_a_modificar = leads_database.at(codigo);
        std::string observacao = std::get<std::string>(event.get_parameter("observacao"));
        std::string timestamp_obs = format_timestamp(std::time(nullptr));
        std::string autor_obs = author.username;

        if (!lead_a_modificar.observacoes.empty()) {
            lead_a_modificar.observacoes += "\n";
        }
        lead_a_modificar.observacoes += "[" + timestamp_obs + " | " + autor_obs + "]: " + observacao;

        auto nome_param = event.get_parameter("nome");
        if (std::holds_alternative<std::string>(nome_param)) { lead_a_modificar.nome = std::get<std::string>(nome_param); }
        auto contato_param = event.get_parameter("contato");
        if (std::holds_alternative<std::string>(contato_param)) { lead_a_modificar.contato = std::get<std::string>(contato_param); }
        auto area_param = event.get_parameter("area");
        if (std::holds_alternative<std::string>(area_param)) { lead_a_modificar.area_atuacao = std::get<std::string>(area_param); }
        auto unidade_param = event.get_parameter("unidade");
        if (std::holds_alternative<std::string>(unidade_param)) { lead_a_modificar.unidade = std::get<std::string>(unidade_param); }
        auto conhece_formato_param = event.get_parameter("conhece_formato");
        if (std::holds_alternative<std::string>(conhece_formato_param)) { lead_a_modificar.conhece_formato = (std::get<std::string>(conhece_formato_param) == "sim"); }
        auto veio_campanha_param = event.get_parameter("veio_campanha");
        if (std::holds_alternative<std::string>(veio_campanha_param)) { lead_a_modificar.veio_campanha = (std::get<std::string>(veio_campanha_param) == "sim"); }
        auto respondido_param = event.get_parameter("respondido");
        if (std::holds_alternative<std::string>(respondido_param)) {
            lead_a_modificar.respondido = (std::get<std::string>(respondido_param) == "sim");
            if (lead_a_modificar.respondido) {
                lead_a_modificar.data_hora_resposta = format_timestamp(std::time(nullptr));
            }
            else {
                lead_a_modificar.data_hora_resposta = "";
            }
        }
        auto status_conversa_param = event.get_parameter("status_conversa");
        if (std::holds_alternative<std::string>(status_conversa_param)) { lead_a_modificar.status_conversa = std::get<std::string>(status_conversa_param); }
        auto status_followup_param = event.get_parameter("status_followup");
        if (std::holds_alternative<std::string>(status_followup_param)) { lead_a_modificar.status_followup = std::get<std::string>(status_followup_param); }
        auto problema_contato_param = event.get_parameter("problema_contato");
        if (std::holds_alternative<std::string>(problema_contato_param)) { lead_a_modificar.problema_contato = (std::get<std::string>(problema_contato_param) == "sim"); }
        auto convidou_visita_param = event.get_parameter("convidou_visita");
        if (std::holds_alternative<std::string>(convidou_visita_param)) { lead_a_modificar.convidou_visita = (std::get<std::string>(convidou_visita_param) == "sim"); }
        auto agendou_visita_param = event.get_parameter("agendou_visita");
        if (std::holds_alternative<std::string>(agendou_visita_param)) { lead_a_modificar.agendou_visita = (std::get<std::string>(agendou_visita_param) == "sim"); }
        auto tipo_fechamento_param = event.get_parameter("tipo_fechamento");
        if (std::holds_alternative<std::string>(tipo_fechamento_param)) { lead_a_modificar.tipo_fechamento = std::get<std::string>(tipo_fechamento_param); }
        auto teve_adicionais_param = event.get_parameter("teve_adicionais");
        if (std::holds_alternative<std::string>(teve_adicionais_param)) { lead_a_modificar.teve_adicionais = (std::get<std::string>(teve_adicionais_param) == "sim"); }
        auto valor_fechamento_param = event.get_parameter("valor_fechamento");
        if (std::holds_alternative<double>(valor_fechamento_param)) { lead_a_modificar.valor_fechamento = std::get<double>(valor_fechamento_param); }
        auto print_param = event.get_parameter("print_final");
        if (std::holds_alternative<dpp::snowflake>(print_param)) {
            dpp::snowflake anexo_id = std::get<dpp::snowflake>(print_param);
            dpp::attachment anexo = event.command.get_resolved_attachment(anexo_id);
            lead_a_modificar.print_final_conversa = anexo.url;
        }
        save_leads_database();
        event.reply("Lead `" + std::to_string(codigo) + "` modificado com sucesso!");
    }
    else {
        event.reply(dpp::message("Codigo do lead nao encontrado.").set_flags(dpp::m_ephemeral));
    }
}

void handle_listar_leads(const dpp::slashcommand_t& event) {
    std::string tipo_lista = std::get<std::string>(event.get_parameter("tipo"));
    dpp::embed embed = dpp::embed().set_color(dpp::colors::blue);
    std::string description;
    int count = 0;
    for (const auto& [id, lead] : leads_database) {
        bool adicionar = false;
        if (tipo_lista == "todos") {
            adicionar = true;
            embed.set_title("📋 Lista de Todos os Leads");
        }
        else if (tipo_lista == "nao_contatados" && lead.status_conversa == "Nao Contatado") {
            adicionar = true;
            embed.set_title("📋 Lista de Leads Nao Contatados");
        }
        else if (tipo_lista == "com_problema" && lead.problema_contato) {
            adicionar = true;
            embed.set_title("📋 Lista de Leads com Problema de Contato");
        }
        else if (tipo_lista == "nao_respondidos" && !lead.respondido) {
            adicionar = true;
            embed.set_title("📋 Lista de Leads Nao Respondidos");
        }
        if (adicionar) {
            description += "**Nome:** " + lead.nome + "\n";
            description += "**Codigo:** `" + std::to_string(id) + "`\n";
            description += "**Data Contato:** " + lead.data_contato + "\n";
            description += "**Contato:** " + (lead.contato.empty() ? "Nao informado" : lead.contato) + "\n";
            description += "**Status:** " + lead.status_conversa + "\n";
            description += "--------------------\n";
            count++;
        }
    }
    if (count == 0) {
        description = "Nenhum lead encontrado para este filtro.";
    }
    else {
        embed.set_title(embed.title + " (" + std::to_string(count) + ")");
    }
    embed.set_description(description);
    event.reply(dpp::message().add_embed(embed));
}

void handle_gerar_planilha(const dpp::slashcommand_t& event) {
    event.thinking(true);
    std::string tipo = std::get<std::string>(event.get_parameter("tipo"));
    if (tipo == "leads") {
        gerar_e_enviar_planilha_leads(event);
    }
    else if (tipo == "compras") {
        gerar_e_enviar_planilha_compras(event);
    }
    else if (tipo == "visitas") {
        gerar_e_enviar_planilha_visitas(event);
    }
}

void handle_ver_lead(const dpp::slashcommand_t& event) {
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    if (leads_database.count(codigo)) {
        const Lead& lead = leads_database.at(codigo);
        dpp::embed embed = dpp::embed().set_color(dpp::colors::blue).set_title("Ficha do Lead: " + lead.nome);
        embed.add_field("Codigo", "`" + std::to_string(lead.id) + "`", true);
        embed.add_field("Criado Por", lead.criado_por, true);
        embed.add_field("Origem", lead.origem, true);
        embed.add_field("Data/Hora Contato", lead.data_contato + " as " + lead.hora_contato, true);
        embed.add_field("Contato", lead.contato.empty() ? "Nao informado" : lead.contato, true);
        embed.add_field("Area", lead.area_atuacao, true);
        embed.add_field("Unidade", lead.unidade, true);
        embed.add_field("Conhece o Formato?", lead.conhece_formato ? "Sim" : "Nao", true);
        embed.add_field("Veio de Campanha?", lead.veio_campanha ? "Sim" : "Nao", true);
        embed.add_field("Respondido?", lead.respondido ? "Sim" : "Nao", true);
        embed.add_field("Data/Hora Resposta", lead.data_hora_resposta.empty() ? "N/A" : lead.data_hora_resposta, true);
        embed.add_field("Problema no Contato?", lead.problema_contato ? "Sim" : "Nao", true);
        embed.add_field("Status da Conversa", lead.status_conversa, true);
        embed.add_field("Status do FollowUp", lead.status_followup, true);
        embed.add_field("Convidou Visita?", lead.convidou_visita ? "Sim" : "Nao", true);
        embed.add_field("Agendou Visita?", lead.agendou_visita ? "Sim" : "Nao", true);
        embed.add_field("Tipo de Fechamento", lead.tipo_fechamento.empty() ? "N/A" : lead.tipo_fechamento, true);
        embed.add_field("Valor do Fechamento", "R$ " + std::to_string(lead.valor_fechamento), true);
        embed.add_field("Observacoes", lead.observacoes.empty() ? "Nenhuma observacao." : lead.observacoes, false);
        if (!lead.print_final_conversa.empty()) {
            embed.set_image(lead.print_final_conversa);
        }
        event.reply(dpp::message().add_embed(embed));
    }
    else {
        event.reply(dpp::message("Codigo do lead nao encontrado.").set_flags(dpp::m_ephemeral));
    }
}

void handle_limpar_leads(const dpp::slashcommand_t& event) {
    leads_database.clear();
    save_leads_database();
    event.reply(dpp::message("🧹 Todos os leads foram limpos do banco de dados.").set_flags(dpp::m_ephemeral));
}

void handle_deletar_lead(const dpp::slashcommand_t& event) {
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    if (leads_database.count(codigo)) {
        std::string nome_do_lead = leads_database.at(codigo).nome;
        leads_database.erase(codigo);
        save_leads_database();
        event.reply("✅ O lead '" + nome_do_lead + "' (codigo `" + std::to_string(codigo) + "`) foi deletado com sucesso.");
    }
    else {
        event.reply(dpp::message("❌ Codigo do lead nao encontrado.").set_flags(dpp::m_ephemeral));
    }
}

void handle_placa(const dpp::slashcommand_t& event) {
    dpp::cluster* bot = event.from()->creator;
    std::string doutor = std::get<std::string>(event.get_parameter("doutor"));

    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::orange)
        .set_title("📢 Nova Solicitação de Placa")
        .add_field("Profissional", doutor, false)
        .add_field("Solicitado por", event.command.get_issuing_user().get_mention(), false)
        .set_footer(dpp::embed_footer().set_text("Aguardando criação da arte..."));

    auto param_anexo = event.get_parameter("imagem_referencia");
    if (auto attachment_id_ptr = std::get_if<dpp::snowflake>(&param_anexo)) {
        dpp::attachment anexo = event.command.get_resolved_attachment(*attachment_id_ptr);
        embed.set_image(anexo.url);
        embed.description = "Imagem de referência anexada.";
    }

    dpp::message msg(config.canal_solicitacao_placas, embed);
    bot->message_create(msg);

    event.reply("✅ Solicitação de placa para **" + doutor + "** enviada com sucesso!");
}

void handle_finalizar_placa(const dpp::slashcommand_t& event) {
    dpp::cluster* bot = event.from()->creator;
    std::string doutor = std::get<std::string>(event.get_parameter("doutor"));
    dpp::snowflake anexo_id = std::get<dpp::snowflake>(event.get_parameter("arte_final"));
    dpp::attachment anexo = event.command.get_resolved_attachment(anexo_id);

    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::green)
        .set_title("✅ Placa Finalizada")
        .add_field("Profissional", doutor, false)
        .set_image(anexo.url)
        .set_footer(dpp::embed_footer().set_text("Arte criada por: " + event.command.get_issuing_user().username));

    dpp::message msg(config.canal_placas_finalizadas, embed);
    bot->message_create(msg);

    event.reply("🎉 Arte da placa para **" + doutor + "** foi enviada com sucesso!");
}

void handle_adicionar_compra(const dpp::slashcommand_t& event) {
    Compra nova_compra;
    nova_compra.id = gerar_codigo();
    nova_compra.item = std::get<std::string>(event.get_parameter("item"));

    auto obs_param = event.get_parameter("observacao");
    if (std::holds_alternative<std::string>(obs_param)) {
        nova_compra.observacao = std::get<std::string>(obs_param);
    }
    else {
        nova_compra.observacao = "Nenhuma.";
    }

    nova_compra.solicitado_por = event.command.get_issuing_user().username;
    nova_compra.data_solicitacao = format_timestamp(std::time(nullptr));

    compras_database[nova_compra.id] = nova_compra;
    save_compras_database();

    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::light_sea_green)
        .set_title("🛒 Item Adicionado à Lista de Compras")
        .add_field("Item", nova_compra.item, false)
        .add_field("Observação", nova_compra.observacao, false)
        .set_footer(dpp::embed_footer().set_text("Adicionado por: " + nova_compra.solicitado_por));

    event.reply(dpp::message().add_embed(embed));
}

void handle_lembrete(const dpp::slashcommand_t& event) {
    dpp::cluster* bot = event.from()->creator;
    dpp::user autor = event.command.get_issuing_user();

    Solicitacao novo_lembrete;
    novo_lembrete.id = gerar_codigo();
    novo_lembrete.id_usuario_responsavel = autor.id;
    novo_lembrete.nome_usuario_responsavel = autor.username;
    novo_lembrete.texto = std::get<std::string>(event.get_parameter("lembrete"));
    novo_lembrete.prazo = std::get<std::string>(event.get_parameter("prazo"));
    novo_lembrete.tipo = LEMBRETE;

    banco_de_dados[novo_lembrete.id] = novo_lembrete;
    save_database();

    dpp::embed dm_embed = dpp::embed()
        .set_color(dpp::colors::sti_blue)
        .set_title("🔔 Novo Lembrete Pessoal Criado!")
        .add_field("Código", "`" + std::to_string(novo_lembrete.id) + "`", false)
        .add_field("Lembrete", novo_lembrete.texto, false)
        .add_field("Prazo", novo_lembrete.prazo, false);

    bot->direct_message_create(autor.id, dpp::message().add_embed(dm_embed));

    event.reply(dpp::message("✅ Lembrete criado com sucesso! Enviei os detalhes no seu privado. Código: `" + std::to_string(novo_lembrete.id) + "`").set_flags(dpp::m_ephemeral));
}

void gerar_e_enviar_planilha_leads(const dpp::slashcommand_t& event) {
    std::time_t tt = std::time(nullptr);
    std::tm* agora = std::localtime(&tt);
    std::stringstream ss;
    ss << std::put_time(agora, "%d-%m-%Y");
    std::string filename = "Planilha de Leads - Atualizada " + ss.str() + ".xlsx";
    lxw_workbook* workbook = workbook_new(filename.c_str());
    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, "Leads");
    worksheet_set_column(worksheet, 0, 0, 15, NULL);
    worksheet_set_column(worksheet, 1, 1, 12, NULL);
    worksheet_set_column(worksheet, 2, 3, 12, NULL);
    worksheet_set_column(worksheet, 4, 4, 30, NULL);
    worksheet_set_column(worksheet, 5, 5, 25, NULL);
    worksheet_set_column(worksheet, 6, 6, 20, NULL);
    worksheet_set_column(worksheet, 7, 9, 18, NULL);
    worksheet_set_column(worksheet, 10, 10, 22, NULL);
    worksheet_set_column(worksheet, 11, 11, 18, NULL);
    worksheet_set_column(worksheet, 12, 12, 50, NULL);
    worksheet_set_column(worksheet, 13, 14, 20, NULL);
    worksheet_set_column(worksheet, 15, 17, 18, NULL);
    worksheet_set_column(worksheet, 18, 20, 18, NULL);
    worksheet_set_column(worksheet, 21, 21, 50, NULL);
    worksheet_set_column(worksheet, 22, 22, 20, NULL);
    lxw_format* header_format = workbook_add_format(workbook);
    format_set_bold(header_format);
    lxw_format* red_format = workbook_add_format(workbook);
    format_set_bg_color(red_format, 0xFFC7CE);
    lxw_format* green_format = workbook_add_format(workbook);
    format_set_bg_color(green_format, 0xC6EFCE);
    lxw_format* yellow_format = workbook_add_format(workbook);
    format_set_bg_color(yellow_format, 0xFFEB9C);

    lxw_format* wrap_format = workbook_add_format(workbook);
    format_set_text_wrap(wrap_format);

    const char* headers[] = { "ID", "Origem", "Data Contato", "Hora Contato", "Nome", "Contato", "Area", "Conhece Formato?", "Veio de Campanha?", "Respondido?", "Data e Hora da Resposta", "Problema Contato?", "Observacoes", "Status Conversa", "Status FollowUp", "Unidade", "Convidou Visita?", "Agendou Visita?", "Tipo Fechamento", "Teve Adicionais?", "Valor Fechamento", "Print Final", "Criado Por" };
    for (int i = 0; i < 23; ++i) {
        worksheet_write_string(worksheet, 0, i, headers[i], header_format);
    }
    std::vector<Lead> sorted_leads;
    for (const auto& pair : leads_database) { sorted_leads.push_back(pair.second); }
    std::sort(sorted_leads.begin(), sorted_leads.end(), [](const Lead& a, const Lead& b) {
        if (a.data_contato.length() < 10 || b.data_contato.length() < 10) return false;
        std::string date_a_str = a.data_contato.substr(6, 4) + a.data_contato.substr(3, 2) + a.data_contato.substr(0, 2);
        std::string date_b_str = b.data_contato.substr(6, 4) + b.data_contato.substr(3, 2) + b.data_contato.substr(0, 2);
        return date_a_str < date_b_str;
        });
    int row = 1;
    for (const auto& lead : sorted_leads) {
        lxw_format* status_format = NULL;
        if (lead.problema_contato) { status_format = yellow_format; }
        else if (lead.status_conversa == "Sem Resposta") { status_format = green_format; }
        else if (lead.status_conversa == "Nao Contatado") { status_format = red_format; }
        worksheet_write_number(worksheet, row, 0, lead.id, NULL);
        worksheet_write_string(worksheet, row, 1, lead.origem.c_str(), NULL);
        worksheet_write_string(worksheet, row, 2, lead.data_contato.c_str(), NULL);
        worksheet_write_string(worksheet, row, 3, lead.hora_contato.c_str(), NULL);
        worksheet_write_string(worksheet, row, 4, lead.nome.c_str(), NULL);
        worksheet_write_string(worksheet, row, 5, lead.contato.c_str(), NULL);
        worksheet_write_string(worksheet, row, 6, lead.area_atuacao.c_str(), NULL);
        worksheet_write_string(worksheet, row, 7, lead.conhece_formato ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 8, lead.veio_campanha ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 9, lead.respondido ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 10, lead.data_hora_resposta.c_str(), NULL);
        worksheet_write_string(worksheet, row, 11, lead.problema_contato ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 12, lead.observacoes.c_str(), wrap_format);
        worksheet_write_string(worksheet, row, 13, lead.status_conversa.c_str(), status_format);
        worksheet_write_string(worksheet, row, 14, lead.status_followup.c_str(), NULL);
        worksheet_write_string(worksheet, row, 15, lead.unidade.c_str(), NULL);
        worksheet_write_string(worksheet, row, 16, lead.convidou_visita ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 17, lead.agendou_visita ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 18, lead.tipo_fechamento.c_str(), NULL);
        worksheet_write_string(worksheet, row, 19, lead.teve_adicionais ? "Sim" : "Nao", NULL);
        worksheet_write_number(worksheet, row, 20, lead.valor_fechamento, NULL);
        if (!lead.print_final_conversa.empty()) { worksheet_write_url(worksheet, row, 21, lead.print_final_conversa.c_str(), NULL); }
        else { worksheet_write_string(worksheet, row, 21, "", NULL); }
        worksheet_write_string(worksheet, row, 22, lead.criado_por.c_str(), NULL);
        row++;
    }
    workbook_close(workbook);
    dpp::message msg;
    msg.set_content("Aqui esta a planilha de leads atualizada!");
    msg.add_file(filename, dpp::utility::read_file(filename));
    event.edit_original_response(msg);
}

void gerar_e_enviar_planilha_compras(const dpp::slashcommand_t& event) {
    std::time_t tt = std::time(nullptr);
    std::tm* agora = std::localtime(&tt);
    std::stringstream ss;
    ss << std::put_time(agora, "%d-%m-%Y");
    std::string filename = "Lista de Compras - " + ss.str() + ".xlsx";

    lxw_workbook* workbook = workbook_new(filename.c_str());
    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, "Compras");

    worksheet_set_column(worksheet, 0, 0, 20, NULL);
    worksheet_set_column(worksheet, 1, 1, 40, NULL);
    worksheet_set_column(worksheet, 2, 2, 50, NULL);
    worksheet_set_column(worksheet, 3, 3, 25, NULL);
    worksheet_set_column(worksheet, 4, 4, 25, NULL);

    lxw_format* header_format = workbook_add_format(workbook);
    format_set_bold(header_format);

    const char* headers[] = { "ID", "Item", "Observação", "Solicitado Por", "Data da Solicitação" };
    for (int i = 0; i < 5; ++i) {
        worksheet_write_string(worksheet, 0, i, headers[i], header_format);
    }

    int row = 1;
    for (const auto& [id, compra] : compras_database) {
        worksheet_write_number(worksheet, row, 0, compra.id, NULL);
        worksheet_write_string(worksheet, row, 1, compra.item.c_str(), NULL);
        worksheet_write_string(worksheet, row, 2, compra.observacao.c_str(), NULL);
        worksheet_write_string(worksheet, row, 3, compra.solicitado_por.c_str(), NULL);
        worksheet_write_string(worksheet, row, 4, compra.data_solicitacao.c_str(), NULL);
        row++;
    }

    workbook_close(workbook);
    dpp::message msg;
    msg.set_content("Aqui está a planilha da lista de compras!");
    msg.add_file(filename, dpp::utility::read_file(filename));
    event.edit_original_response(msg);
}

void gerar_e_enviar_planilha_visitas(const dpp::slashcommand_t& event) {
    std::time_t tt = std::time(nullptr);
    std::tm* agora = std::localtime(&tt);
    std::stringstream ss;
    ss << std::put_time(agora, "%d-%m-%Y");
    std::string filename = "Planilha de Visitas - " + ss.str() + ".xlsx";

    lxw_workbook* workbook = workbook_new(filename.c_str());
    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, "Visitas");

    worksheet_set_column(worksheet, 0, 0, 20, NULL);
    worksheet_set_column(worksheet, 1, 1, 30, NULL);
    worksheet_set_column(worksheet, 2, 2, 25, NULL);
    worksheet_set_column(worksheet, 3, 4, 15, NULL);
    worksheet_set_column(worksheet, 5, 5, 20, NULL);
    worksheet_set_column(worksheet, 6, 6, 20, NULL);
    worksheet_set_column(worksheet, 7, 7, 25, NULL);

    lxw_format* header_format = workbook_add_format(workbook);
    format_set_bold(header_format);

    const char* headers[] = { "ID", "Dr(a).", "Área", "Data", "Horário", "Unidade", "Telefone", "Agendado Por" };
    for (int i = 0; i < 8; ++i) {
        worksheet_write_string(worksheet, 0, i, headers[i], header_format);
    }

    int row = 1;
    for (const auto& [id, visita] : visitas_database) {
        worksheet_write_number(worksheet, row, 0, visita.id, NULL);
        worksheet_write_string(worksheet, row, 1, visita.doutor.c_str(), NULL);
        worksheet_write_string(worksheet, row, 2, visita.area.c_str(), NULL);
        worksheet_write_string(worksheet, row, 3, visita.data.c_str(), NULL);
        worksheet_write_string(worksheet, row, 4, visita.horario.c_str(), NULL);
        worksheet_write_string(worksheet, row, 5, visita.unidade.c_str(), NULL);
        worksheet_write_string(worksheet, row, 6, visita.telefone.c_str(), NULL);
        worksheet_write_string(worksheet, row, 7, visita.quem_marcou_nome.c_str(), NULL);
        row++;
    }

    workbook_close(workbook);
    dpp::message msg;
    msg.set_content("Aqui está a planilha de visitas agendadas!");
    msg.add_file(filename, dpp::utility::read_file(filename));
    event.edit_original_response(msg);
}