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

const std::string BOT_TOKEN = "MTQyNTEzMTA3NzQ4MzIzNzQyNg.GdfR3w.frPGr0_byFgefHxenlps21yeWdm1J3VF2xLg7g";
const dpp::snowflake SEU_SERVER_ID = 1421186452926103594;

const std::string DATABASE_FILE = "demandas.json";

const dpp::snowflake CANAL_VISITAS = 1421188100293660792;

const dpp::snowflake CANAL_AVISO_DEMANDAS = 1423324625030615085;
const dpp::snowflake CANAL_FINALIZADAS = 1425185949263986698;
const dpp::snowflake CARGO_PERMITIDO = 1421186522740293755;

const dpp::snowflake CANAL_DE_LOGS = 1425170563235844186ULL;
const std::vector<dpp::snowflake> CANAIS_SO_IMAGENS = { 1422223702346432643ULL };
const std::vector<dpp::snowflake> CANAIS_SO_LINKS = { 1423015416510418974ULL, 1422624829869264977ULL, 1425171343673921639ULL, 1425207568862547988ULL };
const std::vector<dpp::snowflake> CANAIS_SO_DOCUMENTOS = { 1422225563061588008ULL, 1423015380447658086ULL, 1422225099385339997ULL, 1422223725054398615ULL, 1421188315620839637ULL };

struct Demanda {
    uint64_t id_demanda;
    dpp::snowflake id_usuario_responsavel;
    std::string texto_demanda;
    std::string prazo;
    std::string nome_usuario_responsavel;
};
std::map<uint64_t, Demanda> banco_de_dados_demandas;

void to_json(json& j, const Demanda& d) { j = json{ {"id_demanda", d.id_demanda}, {"id_usuario_responsavel", d.id_usuario_responsavel}, {"texto_demanda", d.texto_demanda}, {"prazo", d.prazo}, {"nome_usuario_responsavel", d.nome_usuario_responsavel} }; }
void from_json(const json& j, Demanda& d) { j.at("id_demanda").get_to(d.id_demanda); j.at("id_usuario_responsavel").get_to(d.id_usuario_responsavel); j.at("texto_demanda").get_to(d.texto_demanda); j.at("prazo").get_to(d.prazo); j.at("nome_usuario_responsavel").get_to(d.nome_usuario_responsavel); }

void save_database() { std::ofstream file(DATABASE_FILE); json j = banco_de_dados_demandas; file << j.dump(4); file.close(); }
void load_database() { std::ifstream file(DATABASE_FILE); if (file.is_open()) { json j; file >> j; if (!j.is_null() && !j.empty()) { banco_de_dados_demandas = j.get<std::map<uint64_t, Demanda>>(); } file.close(); } }

uint64_t gerar_codigo_demanda() { std::random_device rd; std::mt19937_64 gen(rd()); std::uniform_int_distribution<uint64_t> distrib(1000000000, 9999999999); return distrib(gen); }

void enviar_lembretes(dpp::cluster& bot) { bot.log(dpp::ll_info, "Verificando e enviando lembretes de demandas..."); std::map<dpp::snowflake, std::vector<Demanda>> demandas_por_usuario; for (auto const& [id, demanda] : banco_de_dados_demandas) { demandas_por_usuario[demanda.id_usuario_responsavel].push_back(demanda); } for (auto const& [id_usuario, lista_demandas] : demandas_por_usuario) { if (!lista_demandas.empty()) { std::string conteudo_dm = "Olá! 👋 Este é um lembrete das suas demandas ativas:\n\n"; for (const auto& demanda : lista_demandas) { conteudo_dm += "**Código:** `" + std::to_string(demanda.id_demanda) + "`\n"; conteudo_dm += "**Demanda:** " + demanda.texto_demanda + "\n"; conteudo_dm += "**Prazo:** " + demanda.prazo + "\n\n"; } conteudo_dm += "Para finalizar uma demanda, use `/finalizar` em qualquer canal do servidor."; dpp::message dm(conteudo_dm); bot.direct_message_create(id_usuario, dm); } } }

std::string format_timestamp(time_t timestamp) {
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timestamp), "%d/%m/%Y %H:%M:%S");
    return ss.str();
}

int main() {
    load_database();

    dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_guild_message_reactions | dpp::i_message_content);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
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
            bot.message_create(msg);
            event.reply(u8"✅ Visita agendada e informada com sucesso!");

        }
        else if (event.command.get_command_name() == "demanda") {
            dpp::snowflake responsavel_id = std::get<dpp::snowflake>(event.get_parameter("responsavel"));
            dpp::user responsavel = event.command.get_resolved_user(responsavel_id);
            std::string texto = std::get<std::string>(event.get_parameter("demanda"));
            std::string prazo = std::get<std::string>(event.get_parameter("prazo"));
            uint64_t novo_codigo = gerar_codigo_demanda();
            Demanda nova_demanda;
            nova_demanda.id_demanda = novo_codigo;
            nova_demanda.id_usuario_responsavel = responsavel.id;
            nova_demanda.nome_usuario_responsavel = responsavel.username;
            nova_demanda.texto_demanda = texto;
            nova_demanda.prazo = prazo;
            banco_de_dados_demandas[novo_codigo] = nova_demanda;
            save_database();
            dpp::embed embed = dpp::embed().set_color(dpp::colors::orange).set_title(u8"❗ Nova Demanda Criada").add_field("Código da Demanda", std::to_string(novo_codigo), false).add_field("Responsável", responsavel.get_mention(), true).add_field("Prazo de Entrega", prazo, true).add_field("Demanda", texto, false).set_footer(dpp::embed_footer().set_text("Criado por: " + event.command.get_issuing_user().username));
            dpp::message msg;
            msg.set_channel_id(CANAL_AVISO_DEMANDAS);
            msg.set_content("Nova demanda para o responsável: " + responsavel.get_mention());
            msg.add_embed(embed);
            bot.message_create(msg);
            dpp::embed dm_embed = dpp::embed().set_color(dpp::colors::orange).set_title(u8"📬 Você recebeu uma nova demanda!").add_field("Código da Demanda", std::to_string(novo_codigo), false).add_field("Demanda", texto, false).add_field("Prazo de Entrega", prazo, false);
            dpp::message dm_message;
            dm_message.add_embed(dm_embed);
            bot.direct_message_create(responsavel.id, dm_message);
            event.reply("Demanda criada e notificada com sucesso! Código: `" + std::to_string(novo_codigo) + "`");

        }
        else if (event.command.get_command_name() == "finalizar") {
            int64_t codigo_para_finalizar = std::get<int64_t>(event.get_parameter("codigo"));
            if (banco_de_dados_demandas.count(codigo_para_finalizar)) {
                Demanda demanda_finalizada = banco_de_dados_demandas[codigo_para_finalizar];
                banco_de_dados_demandas.erase(codigo_para_finalizar);
                save_database();
                dpp::embed embed = dpp::embed().set_color(dpp::colors::dark_green).set_title(u8"✅ Demanda Finalizada").add_field("Código da Demanda", std::to_string(demanda_finalizada.id_demanda), false).add_field("Responsável", "<@" + std::to_string(demanda_finalizada.id_usuario_responsavel) + ">", true).add_field("Demanda", demanda_finalizada.texto_demanda, false).set_footer(dpp::embed_footer().set_text("Finalizado por: " + event.command.get_issuing_user().username));
                auto param_anexo = event.get_parameter("prova");
                if (auto attachment_id_ptr = std::get_if<dpp::snowflake>(&param_anexo)) {
                    dpp::snowflake anexo_id = *attachment_id_ptr;
                    dpp::attachment anexo = event.command.get_resolved_attachment(anexo_id);
                    if (anexo.content_type.find("image/") == 0) {
                        embed.set_image(anexo.url);
                    }
                }
                dpp::message msg(CANAL_FINALIZADAS, embed);
                bot.message_create(msg);
                event.reply("Demanda `" + std::to_string(codigo_para_finalizar) + "` finalizada com sucesso!");
            }
            else {
                event.reply(dpp::message("Código de demanda não encontrado.").set_flags(dpp::m_ephemeral));
            }

        }
        else if (event.command.get_command_name() == "limpar_demandas") {
            banco_de_dados_demandas.clear();
            save_database();
            event.reply(dpp::message(u8"🧹 Todas as demandas ativas foram limpas do banco de dados.").set_flags(dpp::m_ephemeral));

        }
        else if (event.command.get_command_name() == "lista_demandas") {
            dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("usuario"));
            dpp::user usuario_alvo = event.command.get_resolved_user(user_id);
            std::string lista_texto;
            int count = 0;
            for (auto const& [id_demanda, demanda] : banco_de_dados_demandas) {
                if (demanda.id_usuario_responsavel == usuario_alvo.id) {
                    lista_texto += "**Código:** `" + std::to_string(demanda.id_demanda) + "`\n";
                    lista_texto += "**Demanda:** " + demanda.texto_demanda + "\n";
                    lista_texto += "**Prazo:** " + demanda.prazo + "\n\n";
                    count++;
                }
            }
            dpp::embed embed;
            embed.set_color(dpp::colors::white);
            embed.set_title(u8"📋 Demandas Pendentes de " + usuario_alvo.username);
            if (count > 0) {
                embed.set_description(lista_texto);
            }
            else {
                embed.set_description("Este usuário não possui nenhuma demanda pendente.");
            }
            event.reply(dpp::message().add_embed(embed));
        }
        });

    bot.on_message_create([&bot](const dpp::message_create_t& event) {
        if (event.msg.author.is_bot() || event.msg.channel_id == CANAL_DE_LOGS) {
            return;
        }

        dpp::channel* c = dpp::find_channel(event.msg.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.msg.channel_id);

        std::string log_message = "[" + format_timestamp(event.msg.sent) + "] (#" + channel_name + ") " + event.msg.author.username + " (" + std::to_string(event.msg.author.id) + "): " + event.msg.content;

        if (!event.msg.attachments.empty()) {
            log_message += " [Anexo: " + event.msg.attachments[0].url + "]";
        }
        bot.message_create(dpp::message(CANAL_DE_LOGS, log_message));

        dpp::snowflake channel_id = event.msg.channel_id;

        if (std::find(CANAIS_SO_IMAGENS.begin(), CANAIS_SO_IMAGENS.end(), channel_id) != CANAIS_SO_IMAGENS.end()) {
            if (event.msg.attachments.empty() && event.msg.embeds.empty()) {
                bot.message_delete(event.msg.id, event.msg.channel_id);
                bot.direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de imagens foi removida. Por favor, envie apenas imagens ou GIFs nesse canal."));
            }
        }

        if (std::find(CANAIS_SO_LINKS.begin(), CANAIS_SO_LINKS.end(), channel_id) != CANAIS_SO_LINKS.end()) {
            if (event.msg.content.find("http://") == std::string::npos && event.msg.content.find("https://") == std::string::npos) {
                bot.message_delete(event.msg.id, event.msg.channel_id);
                bot.direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de links foi removida. Por favor, envie apenas links nesse canal."));
            }
        }

        if (std::find(CANAIS_SO_DOCUMENTOS.begin(), CANAIS_SO_DOCUMENTOS.end(), channel_id) != CANAIS_SO_DOCUMENTOS.end()) {
            if (event.msg.attachments.empty()) {
                bot.message_delete(event.msg.id, event.msg.channel_id);
                bot.direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de documentos foi removida. Por favor, envie apenas arquivos nesse canal."));
            }
        }
        });

    bot.on_message_update([&bot](const dpp::message_update_t& event) {
        if (event.msg.author.is_bot() || event.msg.channel_id == CANAL_DE_LOGS) {
            return;
        }

        dpp::channel* c = dpp::find_channel(event.msg.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.msg.channel_id);

        std::string log_message = "[" + format_timestamp(event.msg.edited) + "] [EDITADA] (#" + channel_name + ") " + event.msg.author.username + " (" + std::to_string(event.msg.author.id) + "): " + event.msg.content;
        bot.message_create(dpp::message(CANAL_DE_LOGS, log_message));
        });

    bot.on_message_delete([&bot](const dpp::message_delete_t& event) {
        if (event.channel_id == CANAL_DE_LOGS) {
            return;
        }

        dpp::channel* c = dpp::find_channel(event.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.channel_id);

        std::string log_message = "[" + format_timestamp(std::time(nullptr)) + "] [DELETADA] (#" + channel_name + ") Mensagem ID: " + std::to_string(event.id);
        bot.message_create(dpp::message(CANAL_DE_LOGS, log_message));
        });

    bot.on_message_reaction_add([&bot](const dpp::message_reaction_add_t& event) {
        if (event.reacting_user.is_bot() || event.channel_id == CANAL_DE_LOGS) {
            return;
        }

        dpp::channel* c = dpp::find_channel(event.channel_id);
        std::string channel_name = c ? c->name : std::to_string(event.channel_id);

        std::string log_message = "[" + format_timestamp(std::time(nullptr)) + "] [REAÇÃO ADICIONADA] (#" + channel_name + ") " + event.reacting_user.username + " (" + std::to_string(event.reacting_user.id) + ") reagiu com " + event.reacting_emoji.get_mention() + " à mensagem " + dpp::utility::message_url(event.reacting_guild.id, event.channel_id, event.message_id);
        bot.message_create(dpp::message(CANAL_DE_LOGS, log_message));
        });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands_v7>()) {
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

            dpp::slashcommand finalizar_cmd("finalizar", "Finaliza uma demanda existente usando o código dela.", bot.me.id);
            finalizar_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código de 10 dígitos da demanda.", true));
            finalizar_cmd.add_option(dpp::command_option(dpp::co_attachment, "prova", "Uma imagem que comprova a finalização da demanda.", false));

            dpp::slashcommand limpar_cmd("limpar_demandas", "Limpa TODAS as demandas ativas da memória.", bot.me.id);
            limpar_cmd.set_default_permissions(0);
            limpar_cmd.add_permission(dpp::command_permission(CARGO_PERMITIDO, dpp::cpt_role, true));

            dpp::slashcommand lista_cmd("lista_demandas", "Mostra todas as demandas pendentes de um usuário.", bot.me.id);
            lista_cmd.add_option(dpp::command_option(dpp::co_user, "usuario", "O usuário que você quer consultar.", true));

            bot.guild_bulk_command_create({ visitas_cmd, demanda_cmd, finalizar_cmd, limpar_cmd, lista_cmd }, SEU_SERVER_ID);
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