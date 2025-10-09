#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <map>
#include <random>
#include <variant>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <iomanip>

using json = nlohmann::json;

// --- ATENÇÃO: Lembre-se de resetar seu token e guardá-lo de forma segura ---
const std::string BOT_TOKEN = "OTQ3MTk2MzkzNjgzMDM0MTY0.G3gyvE.rDp0LWfaHj7MAQ3vTsQ5yfAiJ6At5qyau6DsRY";
const dpp::snowflake SEU_SERVER_ID = 907527594096853002;

const std::string DATABASE_FILE = "database.json";
const std::string SCRIPT_LEADS_URL = "https://script.google.com/macros/s/AKfycbwlk3cHYD8ls2WmjVDO5WLMOvmgV_ijDrjtSci1OdZL4KcZK5yTTYDLurlb9TjaTsZB/exec";
const std::string SCRIPT_SECRET = "#Logan23";

const dpp::snowflake CANAL_VISITAS = 12345;
const dpp::snowflake CANAL_AVISO_DEMANDAS = 12345;
const dpp::snowflake CANAL_FINALIZADAS = 12345;
const dpp::snowflake CARGO_PERMITIDO = 12345;
const dpp::snowflake CANAL_DE_LOGS = 12345;
const std::vector<dpp::snowflake> CANAIS_SO_IMAGENS = { 1422223702346432643ULL };
const std::vector<dpp::snowflake> CANAIS_SO_LINKS = { 1423015416510418974ULL, 1422624829869264977ULL, 1425171343673921639ULL, 1425207568862547988ULL };
const std::vector<dpp::snowflake> CANAIS_SO_DOCUMENTOS = { 1422225563061588008ULL, 1423015380447658086ULL, 1422225099385339997ULL, 1422223725054398615ULL, 1421188315620839637ULL };
const dpp::snowflake CANAL_AVISO_PEDIDOS = 12345;
const dpp::snowflake CANAL_PEDIDOS_CONCLUIDOS = 12345;

struct Solicitacao {
    uint64_t id;
    dpp::snowflake id_usuario_responsavel;
    std::string texto;
    std::string prazo;
    std::string nome_usuario_responsavel;
    bool is_pedido = false;
};
std::map<uint64_t, Solicitacao> banco_de_dados;

void to_json(json& j, const Solicitacao& s) { j = json{ {"id", s.id}, {"id_usuario_responsavel", s.id_usuario_responsavel}, {"texto", s.texto}, {"prazo", s.prazo}, {"nome_usuario_responsavel", s.nome_usuario_responsavel}, {"is_pedido", s.is_pedido} }; }
void from_json(const json& j, Solicitacao& s) { j.at("id").get_to(s.id); j.at("id_usuario_responsavel").get_to(s.id_usuario_responsavel); j.at("texto").get_to(s.texto); j.at("prazo").get_to(s.prazo); j.at("nome_usuario_responsavel").get_to(s.nome_usuario_responsavel); if (j.contains("is_pedido")) { j.at("is_pedido").get_to(s.is_pedido); } else { s.is_pedido = false; } }

void save_database() { std::ofstream file(DATABASE_FILE); json j = banco_de_dados; file << j.dump(4); file.close(); }
void load_database() { std::ifstream file(DATABASE_FILE); if (file.is_open()) { json j; file >> j; if (!j.is_null() && !j.empty()) { banco_de_dados = j.get<std::map<uint64_t, Solicitacao>>(); } file.close(); } }

uint64_t gerar_codigo() { std::random_device rd; std::mt19937_64 gen(rd()); std::uniform_int_distribution<uint64_t> distrib(1000000000, 9999999999); return distrib(gen); }

void enviar_lembretes(dpp::cluster& bot) {
    bot.log(dpp::ll_info, "Verificando e enviando lembretes de demandas...");
    std::map<dpp::snowflake, std::vector<Solicitacao>> solicitacoes_por_usuario;
    for (auto const& [id, solicitacao] : banco_de_dados) {
        if (!solicitacao.is_pedido) {
            solicitacoes_por_usuario[solicitacao.id_usuario_responsavel].push_back(solicitacao);
        }
    }
    for (auto const& [id_usuario, lista_demandas] : solicitacoes_por_usuario) {
        if (!lista_demandas.empty()) {
            std::string conteudo_dm = u8"Olá! 👋 Este é um lembrete das suas demandas ativas:\n\n";
            for (const auto& demanda : lista_demandas) {
                conteudo_dm += "**Código:** `" + std::to_string(demanda.id) + "`\n";
                conteudo_dm += "**Demanda:** " + demanda.texto + "\n";
                conteudo_dm += "**Prazo:** " + demanda.prazo + "\n\n";
            }
            conteudo_dm += "Para finalizar uma demanda, use `/finalizar_demanda` em qualquer canal do servidor.";
            dpp::message dm(conteudo_dm);
            bot.direct_message_create(id_usuario, dm);
        }
    }
}

std::string format_timestamp(time_t timestamp) { std::stringstream ss; ss << std::put_time(std::localtime(&timestamp), "%d/%m/%Y %H:%M:%S"); return ss.str(); }

int main() {
    load_database();
    dpp::cluster bot(BOT_TOKEN, dpp::i_guilds | dpp::i_guild_messages | dpp::i_guild_members | dpp::i_guild_message_reactions | dpp::i_message_content);
    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
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
        bot->message_create(dpp::message(CANAL_DE_LOGS, log_message));

        if (event.command.get_command_name() == "visitas") {
            dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("quem_marcou"));
            dpp::user quem_marcou = event.command.get_resolved_user(user_id);
            std::string doutor = std::get<std::string>(event.get_parameter("doutor"));
            std::string area = std::get<std::string>(event.get_parameter("area"));
            std::string data = std::get<std::string>(event.get_parameter("data"));
            std::string horario = std::get<std::string>(event.get_parameter("horario"));
            std::string unidade = std::get<std::string>(event.get_parameter("unidade"));
            std::string telefone = std::get<std::string>(event.get_parameter("telefone"));
            dpp::embed embed = dpp::embed().set_color(dpp::colors::blue).set_title(u8"🔔 Informe de Visita").add_field("Agendado por", quem_marcou.get_mention(), true).add_field("Nome do Dr(a).", doutor, true).add_field("Área", area, true).add_field("Data", data, true).add_field("Horário", horario, true).add_field("Unidade", unidade, true).add_field("Telefone", telefone, false);
            dpp::message msg(CANAL_VISITAS, embed);
            bot->message_create(msg);
            event.reply(u8"✅ Visita agendada e informada com sucesso!");
        }
        else if (event.command.get_command_name() == "demanda" || event.command.get_command_name() == "pedido") {
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
            nova_solicitacao.is_pedido = is_pedido;
            banco_de_dados[novo_codigo] = nova_solicitacao;
            save_database();
            dpp::embed embed = dpp::embed().set_color(is_pedido ? dpp::colors::light_sea_green : dpp::colors::orange).set_title(is_pedido ? u8"📢 Novo Pedido Criado" : u8"❗ Nova Demanda Criada").add_field("Código", std::to_string(novo_codigo), false).add_field("Responsável", responsavel.get_mention(), true).add_field("Criado por", event.command.get_issuing_user().get_mention(), true);
            if (!is_pedido) { embed.add_field("Prazo de Entrega", prazo, true); }
            embed.add_field(is_pedido ? "Pedido" : "Demanda", texto, false);
            dpp::message msg;
            msg.set_channel_id(is_pedido ? CANAL_AVISO_PEDIDOS : CANAL_AVISO_DEMANDAS);
            msg.set_content(std::string("Nova solicitação para o responsável: ") + responsavel.get_mention());
            msg.add_embed(embed);
            bot->message_create(msg);
            dpp::embed dm_embed = dpp::embed().set_color(is_pedido ? dpp::colors::light_sea_green : dpp::colors::orange).set_title(is_pedido ? u8"📬 Você recebeu um novo pedido!" : u8"📬 Você recebeu uma nova demanda!").add_field("Código", std::to_string(novo_codigo), false).add_field(is_pedido ? "Pedido" : "Demanda", texto, false);
            if (!is_pedido) { dm_embed.add_field("Prazo de Entrega", prazo, false); }
            dm_embed.set_footer(dpp::embed_footer().set_text(std::string("Solicitado por: ") + event.command.get_issuing_user().username));
            dpp::message dm_message;
            dm_message.add_embed(dm_embed);
            bot->direct_message_create(responsavel.id, dm_message);
            dpp::embed log_dm_embed = dpp::embed().set_color(dpp::colors::black).set_title("LOG: Mensagem Privada Enviada").add_field("Tipo", (is_pedido ? "Novo Pedido" : "Nova Demanda"), true).add_field("Destinatário", "`" + responsavel.username + "` (`" + std::to_string(responsavel.id) + "`)", true).add_field("Autor da Solicitação", "`" + event.command.get_issuing_user().username + "` (`" + std::to_string(event.command.get_issuing_user().id) + "`)", false).set_description(dm_embed.description);
            for (const auto& field : dm_embed.fields) { log_dm_embed.add_field(field.name, field.value, field.is_inline); }
            bot->message_create(dpp::message(CANAL_DE_LOGS, log_dm_embed));
            event.reply(std::string(is_pedido ? "Pedido" : "Demanda") + " criado e notificado com sucesso! Código: `" + std::to_string(novo_codigo) + "`");
        }
        else if (event.command.get_command_name() == "finalizar_demanda" || event.command.get_command_name() == "finalizar_pedido" || event.command.get_command_name() == "cancelar_pedido") {
            bool is_cancel = event.command.get_command_name() == "cancelar_pedido";
            int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
            if (banco_de_dados.count(codigo)) {
                Solicitacao item_concluido = banco_de_dados[codigo];
                banco_de_dados.erase(codigo);
                save_database();
                std::string title = item_concluido.is_pedido ? (is_cancel ? u8"❌ Pedido Cancelado" : u8"✅ Pedido Finalizado") : u8"✅ Demanda Finalizada";
                dpp::embed embed = dpp::embed().set_color(is_cancel ? dpp::colors::red : dpp::colors::dark_green).set_title(title).add_field("Código", std::to_string(item_concluido.id), false).add_field("Responsável", "<@" + std::to_string(item_concluido.id_usuario_responsavel) + ">", true).add_field("Finalizado por", event.command.get_issuing_user().get_mention(), true).add_field(item_concluido.is_pedido ? "Pedido" : "Demanda", item_concluido.texto, false);
                if (event.command.get_command_name() != "cancelar_pedido") {
                    auto param_anexo = event.get_parameter("prova");
                    if (auto attachment_id_ptr = std::get_if<dpp::snowflake>(&param_anexo)) {
                        dpp::snowflake anexo_id = *attachment_id_ptr;
                        dpp::attachment anexo = event.command.get_resolved_attachment(anexo_id);
                        if (anexo.content_type.find("image/") == 0) { embed.set_image(anexo.url); }
                    }
                }
                dpp::message msg(item_concluido.is_pedido ? CANAL_PEDIDOS_CONCLUIDOS : CANAL_FINALIZADAS, embed);
                bot->message_create(msg);
                event.reply(std::string(item_concluido.is_pedido ? "Pedido" : "Demanda") + " `" + std::to_string(codigo) + (is_cancel ? "` cancelado" : "` finalizado") + " com sucesso!");
            }
            else {
                event.reply(dpp::message("Código não encontrado.").set_flags(dpp::m_ephemeral));
            }
        }
        else if (event.command.get_command_name() == "limpar_demandas") {
            banco_de_dados.clear();
            save_database();
            event.reply(dpp::message(u8"🧹 Todas as demandas e pedidos ativos foram limpos do banco de dados.").set_flags(dpp::m_ephemeral));
        }
        else if (event.command.get_command_name() == "lista_demandas") {
            dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("usuario"));
            dpp::user usuario_alvo = event.command.get_resolved_user(user_id);
            std::string lista_texto;
            int count = 0;
            for (auto const& [id_solicitacao, solicitacao] : banco_de_dados) {
                if (solicitacao.id_usuario_responsavel == usuario_alvo.id) {
                    lista_texto += "**" + std::string(solicitacao.is_pedido ? "Pedido" : "Demanda") + "**\n";
                    lista_texto += "**Código:** `" + std::to_string(solicitacao.id) + "`\n";
                    lista_texto += "**Descrição:** " + solicitacao.texto + "\n";
                    if (!solicitacao.is_pedido) { lista_texto += "**Prazo:** " + solicitacao.prazo + "\n"; }
                    lista_texto += "\n";
                    count++;
                }
            }
            dpp::embed embed;
            embed.set_color(dpp::colors::white).set_title(u8"📋 Solicitações Pendentes de " + usuario_alvo.username);
            if (count > 0) { embed.set_description(lista_texto); }
            else { embed.set_description("Este usuário não possui nenhuma demanda ou pedido pendente."); }
            event.reply(dpp::message().add_embed(embed));
        }
        else if (event.command.get_command_name() == "novo_lead") {
            std::string origem = std::get<std::string>(event.get_parameter("origem"));
            std::string data = std::get<std::string>(event.get_parameter("data"));
            std::string hora = std::get<std::string>(event.get_parameter("hora"));
            std::string nome = std::get<std::string>(event.get_parameter("nome"));
            std::string contato = std::get<std::string>(event.get_parameter("contato"));
            std::string area = std::get<std::string>(event.get_parameter("area"));
            uint64_t codigo_lead = gerar_codigo();
            json payload;
            payload["secret"] = SCRIPT_SECRET;
            payload["origem"] = origem;
            payload["data"] = data;
            payload["hora"] = hora;
            payload["nome"] = nome;
            payload["contato"] = contato;
            payload["area"] = area;
            payload["codigo_lead"] = std::to_string(codigo_lead);

            bot->request(
                SCRIPT_LEADS_URL,
                dpp::m_post,
                [bot, event](const dpp::http_request_completion_t& cc) {
                    if (cc.status == 200) {
                        bot->log(dpp::ll_info, "Lead enviado para a planilha com sucesso.");
                        event.edit_original_response(dpp::message(u8"✅ Lead adicionado à planilha com sucesso!"));
                    }
                    else {
                        bot->log(dpp::ll_error, "Falha ao enviar lead para a planilha: " + cc.body);
                        event.edit_original_response(dpp::message(u8"❌ Falha ao adicionar lead na planilha. Verifique o console do bot."));
                    }
                },
                payload.dump(),
                "application/json"
            );
            event.reply(dpp::message("Processando...").set_flags(dpp::m_ephemeral));
        }
        });

    bot.on_message_create([](const dpp::message_create_t& event) {
        if (event.msg.author.is_bot() || event.msg.channel_id == CANAL_DE_LOGS) { return; }
        dpp::cluster* bot = event.from()->creator;
        dpp::channel* c = dpp::find_channel(event.msg.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.msg.channel_id);
        std::string log_message = std::string("`[") + format_timestamp(event.msg.sent) + "]` `(#" + channel_name + ")` `" + event.msg.author.username + "` `(" + std::to_string(event.msg.author.id) + ")`: " + event.msg.content;
        if (!event.msg.attachments.empty()) { for (const auto& att : event.msg.attachments) { log_message += " [Anexo: " + att.url + "]"; } }
        bot->message_create(dpp::message(CANAL_DE_LOGS, log_message));
        dpp::snowflake channel_id = event.msg.channel_id;
        if (std::find(CANAIS_SO_IMAGENS.begin(), CANAIS_SO_IMAGENS.end(), channel_id) != CANAIS_SO_IMAGENS.end()) { if (event.msg.attachments.empty() && event.msg.embeds.empty()) { bot->message_delete(event.msg.id, event.msg.channel_id); bot->direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de imagens foi removida. Por favor, envie apenas imagens ou GIFs nesse canal.")); } }
        if (std::find(CANAIS_SO_LINKS.begin(), CANAIS_SO_LINKS.end(), channel_id) != CANAIS_SO_LINKS.end()) { if (event.msg.content.find("http") == std::string::npos) { bot->message_delete(event.msg.id, event.msg.channel_id); bot->direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de links foi removida. Por favor, envie apenas links nesse canal.")); } }
        if (std::find(CANAIS_SO_DOCUMENTOS.begin(), CANAIS_SO_DOCUMENTOS.end(), channel_id) != CANAIS_SO_DOCUMENTOS.end()) { if (event.msg.attachments.empty()) { bot->message_delete(event.msg.id, event.msg.channel_id); bot->direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de documentos foi removida. Por favor, envie apenas arquivos nesse canal.")); } }
        });

    bot.on_message_update([](const dpp::message_update_t& event) {
        if (event.msg.author.is_bot() || event.msg.channel_id == CANAL_DE_LOGS) { return; }
        dpp::cluster* bot = event.from()->creator;
        dpp::channel* c = dpp::find_channel(event.msg.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.msg.channel_id);
        std::string log_message = std::string("`[") + format_timestamp(event.msg.edited) + "]` `[EDITADA]` `(#" + channel_name + ")` `" + event.msg.author.username + "` `(" + std::to_string(event.msg.author.id) + ")`: " + event.msg.content;
        bot->message_create(dpp::message(CANAL_DE_LOGS, log_message));
        });

    bot.on_message_delete([](const dpp::message_delete_t& event) {
        if (event.channel_id == CANAL_DE_LOGS) { return; }
        dpp::cluster* bot = event.from()->creator;
        dpp::channel* c = dpp::find_channel(event.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.channel_id);
        std::string log_message = std::string("`[") + format_timestamp(std::time(nullptr)) + "]` `[DELETADA]` `(#" + channel_name + ")` `Mensagem ID: " + std::to_string(event.id) + "`";
        bot->message_create(dpp::message(CANAL_DE_LOGS, log_message));
        });

    bot.on_message_reaction_add([](const dpp::message_reaction_add_t& event) {
        if (event.reacting_user.is_bot() || event.channel_id == CANAL_DE_LOGS) { return; }
        dpp::cluster* bot = event.from()->creator;
        dpp::channel* c = dpp::find_channel(event.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.channel_id);
        std::string log_message = std::string("`[") + format_timestamp(std::time(nullptr)) + "]` `[REAÇÃO]` `(#" + channel_name + ")` `" + event.reacting_user.username + "` `(" + std::to_string(event.reacting_user.id) + ")` reagiu com " + event.reacting_emoji.get_mention() + " à mensagem " + dpp::utility::message_url(event.reacting_guild.id, event.channel_id, event.message_id);
        bot->message_create(dpp::message(CANAL_DE_LOGS, log_message));
        });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands_v13>()) {
            dpp::slashcommand visitas_cmd("visitas", "Agenda uma nova visita.", bot.me.id);
            visitas_cmd.add_option(dpp::command_option(dpp::co_user, "quem_marcou", "Quem está agendando a visita.", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome do Dr(a).", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Área de atuação do Dr(a).", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "data", "Data da visita (ex: 10/10/2025).", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "horario", "Horário da visita (ex: 14h).", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade da visita.", true));
            visitas_cmd.add_option(dpp::command_option(dpp::co_string, "telefone", "Telefone de contato.", true));

            dpp::slashcommand demanda_cmd("demanda", "Cria uma nova demanda para um usuário.", bot.me.id);
            demanda_cmd.add_option(dpp::command_option(dpp::co_user, "responsavel", "O usuário que receberá a demanda.", true));
            demanda_cmd.add_option(dpp::command_option(dpp::co_string, "demanda", "Descrição detalhada da demanda.", true));
            demanda_cmd.add_option(dpp::command_option(dpp::co_string, "prazo", "Prazo para a conclusão da demanda.", true));
            demanda_cmd.set_default_permissions(0);
            demanda_cmd.add_permission(dpp::command_permission(CARGO_PERMITIDO, dpp::cpt_role, true));

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
            cancelar_pedido_cmd.add_permission(dpp::command_permission(CARGO_PERMITIDO, dpp::cpt_role, true));

            dpp::slashcommand novolead_cmd("novo_lead", "Adiciona um novo lead à planilha.", bot.me.id);
            novolead_cmd.add_option(
                dpp::command_option(dpp::co_string, "origem", "De onde veio o lead", true)
                .add_choice(dpp::command_option_choice("Whatsapp", std::string("Whatsapp")))
                .add_choice(dpp::command_option_choice("Instagram", std::string("Instagram")))
                .add_choice(dpp::command_option_choice("Facebook", std::string("Facebook")))
                .add_choice(dpp::command_option_choice("Site", std::string("Site")))
            );
            novolead_cmd.add_option(dpp::command_option(dpp::co_string, "data", "Data do primeiro contato (dd/mm/aaaa)", true));
            novolead_cmd.add_option(dpp::command_option(dpp::co_string, "hora", "Hora do primeiro contato (hh:mm)", true));
            novolead_cmd.add_option(dpp::command_option(dpp::co_string, "nome", "Nome completo do lead", true));
            novolead_cmd.add_option(dpp::command_option(dpp::co_string, "contato", "Telefone de contato do lead", true));
            novolead_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Área de atuação do lead", true));

            bot.guild_bulk_command_create({
                visitas_cmd,
                demanda_cmd,
                finalizar_demanda_cmd,
                limpar_cmd,
                lista_cmd,
                pedido_cmd,
                finalizar_pedido_cmd,
                cancelar_pedido_cmd,
                novolead_cmd
                }, SEU_SERVER_ID);
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

    return 0;
}