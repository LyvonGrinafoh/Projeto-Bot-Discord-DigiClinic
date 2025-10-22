#include "SolicitacaoCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "EventHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <variant>
#include <string>
#include <vector>
#include <cmath>

dpp::embed generatePageEmbed(const PaginationState& state);

SolicitacaoCommands::SolicitacaoCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler, EventHandler& eventHandler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler), eventHandler_(eventHandler)
{
}

void SolicitacaoCommands::handle_demanda_pedido(const dpp::slashcommand_t& event) {
    bool is_pedido = event.command.get_command_name() == "pedido";
    dpp::snowflake responsavel_id = std::get<dpp::snowflake>(event.get_parameter("responsavel"));
    dpp::user responsavel = event.command.get_resolved_user(responsavel_id);
    std::string texto = std::get<std::string>(event.get_parameter(is_pedido ? "pedido" : "demanda"));
    std::string prazo = is_pedido ? "N/A" : std::get<std::string>(event.get_parameter("prazo"));
    uint64_t novo_codigo = Utils::gerar_codigo();
    Solicitacao nova_solicitacao;
    nova_solicitacao.id = novo_codigo;
    nova_solicitacao.id_usuario_responsavel = responsavel.id;
    nova_solicitacao.nome_usuario_responsavel = responsavel.username;
    nova_solicitacao.texto = texto;
    nova_solicitacao.prazo = prazo;
    nova_solicitacao.tipo = is_pedido ? PEDIDO : DEMANDA;
    if (db_.addOrUpdateSolicitacao(nova_solicitacao)) {
        dpp::embed embed = dpp::embed().set_color(is_pedido ? dpp::colors::light_sea_green : dpp::colors::orange).set_title(is_pedido ? "📢 Novo Pedido Criado" : "❗ Nova Demanda Criada").add_field("Código", std::to_string(novo_codigo), false).add_field("Responsável", responsavel.get_mention(), true).add_field("Criado por", event.command.get_issuing_user().get_mention(), true);
        if (!is_pedido) { embed.add_field("Prazo de Entrega", prazo, true); }
        embed.add_field(is_pedido ? "Pedido" : "Demanda", texto, false);
        dpp::message msg; msg.set_channel_id(is_pedido ? config_.canal_aviso_pedidos : config_.canal_aviso_demandas); msg.set_content("Nova solicitação para o responsável: " + responsavel.get_mention()); msg.add_embed(embed); bot_.message_create(msg);
        dpp::embed dm_embed = dpp::embed().set_color(is_pedido ? dpp::colors::light_sea_green : dpp::colors::orange).set_title(is_pedido ? "📬 Você recebeu um novo pedido!" : "📬 Você recebeu uma nova demanda!").add_field("Código", std::to_string(novo_codigo), false).add_field(is_pedido ? "Pedido" : "Demanda", texto, false);
        if (!is_pedido) { dm_embed.add_field("Prazo de Entrega", prazo, false); }
        dm_embed.set_footer(dpp::embed_footer().set_text("Solicitado por: " + event.command.get_issuing_user().username));
        dpp::message dm_message; dm_message.add_embed(dm_embed); bot_.direct_message_create(responsavel.id, dm_message);
        dpp::embed log_dm_embed = dpp::embed().set_color(dpp::colors::black).set_title("LOG: Mensagem Privada Enviada").add_field("Tipo", (is_pedido ? "Novo Pedido" : "Nova Demanda"), true).add_field("Destinatário", "`" + responsavel.username + "` (`" + std::to_string(responsavel.id) + "`)", true).add_field("Autor da Solicitação", "`" + event.command.get_issuing_user().username + "` (`" + std::to_string(event.command.get_issuing_user().id) + "`)", false);
        for (const auto& field : dm_embed.fields) { log_dm_embed.add_field(field.name, field.value, field.is_inline); }
        bot_.message_create(dpp::message(config_.canal_logs, log_dm_embed));
        cmdHandler_.replyAndDelete(event, dpp::message(std::string(is_pedido ? "Pedido" : "Demanda") + (const char*)" criado e notificado com sucesso! Código: `" + std::to_string(novo_codigo) + "`"));
    }
    else {
        event.reply(dpp::message("❌ Erro ao salvar a solicitação no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void SolicitacaoCommands::handle_finalizar_solicitacao(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo")); uint64_t codigo = static_cast<uint64_t>(codigo_int); std::string command_name = event.command.get_command_name();
    std::optional<Solicitacao> item_opt = db_.getSolicitacao(codigo);
    if (item_opt) {
        Solicitacao item_concluido = *item_opt;
        if (command_name == "cancelar_pedido") { if (item_concluido.tipo != PEDIDO) { event.reply(dpp::message("❌ Este comando é apenas para cancelar **pedidos**.").set_flags(dpp::m_ephemeral)); return; } }
        if (db_.removeSolicitacao(codigo)) {
            std::string tipo_str; std::string title; dpp::snowflake channel_id = 0;
            switch (item_concluido.tipo) {
            case DEMANDA: tipo_str = "Demanda"; title = "✅ Demanda Finalizada"; channel_id = config_.canal_finalizadas; break;
            case PEDIDO: tipo_str = "Pedido"; title = (command_name == "cancelar_pedido") ? "❌ Pedido Cancelado" : "✅ Pedido Finalizado"; channel_id = config_.canal_pedidos_concluidos; break;
            case LEMBRETE: tipo_str = "Lembrete"; cmdHandler_.replyAndDelete(event, dpp::message("✅ Lembrete `" + std::to_string(codigo) + "` finalizado com sucesso!")); return;
            }
            if (channel_id != 0) {
                dpp::embed embed = dpp::embed().set_color((command_name == "cancelar_pedido") ? dpp::colors::red : dpp::colors::dark_green).set_title(title).add_field("Código", std::to_string(item_concluido.id), false).add_field("Responsável", "<@" + std::to_string(item_concluido.id_usuario_responsavel) + ">", true).add_field((command_name == "cancelar_pedido" ? "Cancelado por" : "Finalizado por"), event.command.get_issuing_user().get_mention(), true).add_field(tipo_str, item_concluido.texto, false);
                if (command_name != "cancelar_pedido") { auto param_anexo = event.get_parameter("prova"); if (auto attachment_id_ptr = std::get_if<dpp::snowflake>(&param_anexo)) { dpp::attachment anexo = event.command.get_resolved_attachment(*attachment_id_ptr); if (anexo.content_type.find("image/") == 0) { embed.set_image(anexo.url); } } }
                bot_.message_create(dpp::message(channel_id, embed));
            }
            cmdHandler_.replyAndDelete(event, dpp::message(tipo_str + " `" + std::to_string(codigo) + "` " + (command_name == "cancelar_pedido" ? "cancelado" : "finalizado") + " com sucesso!"));
        }
        else {
            event.reply(dpp::message("❌ Erro ao remover a solicitação do banco de dados após encontrá-la.").set_flags(dpp::m_ephemeral));
        }
    }
    else {
        event.reply(dpp::message("❌ Código não encontrado no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void SolicitacaoCommands::handle_limpar_demandas(const dpp::slashcommand_t& event) {
    db_.clearSolicitacoes();
    event.reply(dpp::message("🧹 Todas as demandas, pedidos e lembretes ativos foram limpos.").set_flags(dpp::m_ephemeral));
}

void SolicitacaoCommands::handle_lista_demandas(const dpp::slashcommand_t& event) {
    dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("usuario"));
    std::vector<PaginatedItem> solicitacoes_do_usuario; const auto& todas_solicitacoes = db_.getSolicitacoes();
    for (const auto& [id, solicitacao] : todas_solicitacoes) { if (solicitacao.id_usuario_responsavel == user_id) { solicitacoes_do_usuario.push_back(solicitacao); } }
    std::sort(solicitacoes_do_usuario.begin(), solicitacoes_do_usuario.end(), [](const PaginatedItem& a, const PaginatedItem& b) { const auto& sol_a = std::get<Solicitacao>(a); const auto& sol_b = std::get<Solicitacao>(b); return sol_a.tipo < sol_b.tipo; });
    if (solicitacoes_do_usuario.empty()) {
        event.reply(dpp::message("Este usuário não possui nenhuma demanda, pedido ou lembrete pendente.").set_flags(dpp::m_ephemeral));
        return;
    }
    PaginationState state; state.items = std::move(solicitacoes_do_usuario); state.originalUserID = event.command.get_issuing_user().id; state.listType = "demandas"; state.currentPage = 1; state.itemsPerPage = 5;
    dpp::embed firstPageEmbed = generatePageEmbed(state); bool needsPagination = state.items.size() > state.itemsPerPage;
    event.reply(dpp::message(event.command.channel_id, firstPageEmbed),
        [this, state, needsPagination, event](const dpp::confirmation_callback_t& cb) mutable {
            if (!cb.is_error()) {
                if (needsPagination) {
                    event.get_original_response([this, state](const dpp::confirmation_callback_t& msg_cb) mutable {
                        if (!msg_cb.is_error()) {
                            auto* msg_ptr = std::get_if<dpp::message>(&msg_cb.value);
                            if (msg_ptr) {
                                auto& msg = *msg_ptr;
                                eventHandler_.addPaginationState(msg.id, std::move(state));
                                bot_.message_add_reaction(msg.id, msg.channel_id, "◀️");
                                bot_.message_add_reaction(msg.id, msg.channel_id, "▶️");
                            }
                            else { Utils::log_to_file("Erro: get_original_response não retornou um dpp::message."); }
                        }
                        else { Utils::log_to_file("Erro ao *buscar* a resposta original: " + msg_cb.get_error().message); }
                        });
                }
            }
            else {
                Utils::log_to_file("Erro ao *enviar* a resposta inicial paginada para /lista_demandas: " + cb.get_error().message);
            }
        });
}

void SolicitacaoCommands::handle_lembrete(const dpp::slashcommand_t& event) {
    dpp::user autor = event.command.get_issuing_user();
    Solicitacao novo_lembrete;
    novo_lembrete.id = Utils::gerar_codigo();
    novo_lembrete.id_usuario_responsavel = autor.id;
    novo_lembrete.nome_usuario_responsavel = autor.username;
    novo_lembrete.texto = std::get<std::string>(event.get_parameter("lembrete"));
    novo_lembrete.prazo = std::get<std::string>(event.get_parameter("prazo"));
    novo_lembrete.tipo = LEMBRETE;
    if (db_.addOrUpdateSolicitacao(novo_lembrete)) {
        dpp::embed dm_embed = dpp::embed().set_color(dpp::colors::sti_blue).set_title("🔔 Novo Lembrete Pessoal Criado!").add_field("Código", "`" + std::to_string(novo_lembrete.id) + "`", false).add_field("Lembrete", novo_lembrete.texto, false).add_field("Prazo", novo_lembrete.prazo, false);
        bot_.direct_message_create(autor.id, dpp::message().add_embed(dm_embed));
        cmdHandler_.replyAndDelete(event, dpp::message("✅ Lembrete criado com sucesso! Enviei os detalhes no seu privado. Código: `" + std::to_string(novo_lembrete.id) + "`"));
    }
    else {
        event.reply(dpp::message("❌ Erro ao salvar o lembrete no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void SolicitacaoCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id, const BotConfig& config) {
    dpp::slashcommand demanda_cmd("demanda", "Cria uma nova demanda para um usuário.", bot_id);
    demanda_cmd.add_option(dpp::command_option(dpp::co_user, "responsavel", "O usuário que receberá a demanda.", true));
    demanda_cmd.add_option(dpp::command_option(dpp::co_string, "demanda", "Descrição detalhada da demanda.", true));
    demanda_cmd.add_option(dpp::command_option(dpp::co_string, "prazo", "Prazo para a conclusão da demanda.", true));
    demanda_cmd.set_default_permissions(0); demanda_cmd.add_permission(dpp::command_permission(config.cargo_permitido, dpp::cpt_role, true)); commands.push_back(demanda_cmd);
    dpp::slashcommand finalizar_demanda_cmd("finalizar_demanda", "Finaliza uma demanda existente.", bot_id);
    finalizar_demanda_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código de 10 dígitos da demanda.", true));
    finalizar_demanda_cmd.add_option(dpp::command_option(dpp::co_attachment, "prova", "Uma imagem que comprova a finalização.", false));
    commands.push_back(finalizar_demanda_cmd);
    dpp::slashcommand limpar_cmd("limpar_demandas", "Limpa TODAS as demandas, pedidos e lembretes ativos.", bot_id);
    limpar_cmd.default_member_permissions = dpp::p_administrator; commands.push_back(limpar_cmd);
    dpp::slashcommand lista_cmd("lista_demandas", "Mostra todas as pendências (demandas, pedidos, lembretes) de um usuário.", bot_id);
    lista_cmd.add_option(dpp::command_option(dpp::co_user, "usuario", "O usuário que você quer consultar.", true));
    commands.push_back(lista_cmd);
    dpp::slashcommand pedido_cmd("pedido", "Cria um novo pedido para um usuário.", bot_id);
    pedido_cmd.add_option(dpp::command_option(dpp::co_user, "responsavel", "O usuário que receberá o pedido.", true));
    pedido_cmd.add_option(dpp::command_option(dpp::co_string, "pedido", "Descrição detalhada do pedido.", true));
    commands.push_back(pedido_cmd);
    dpp::slashcommand finalizar_pedido_cmd("finalizar_pedido", "Finaliza um pedido existente.", bot_id);
    finalizar_pedido_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código do pedido.", true));
    finalizar_pedido_cmd.add_option(dpp::command_option(dpp::co_attachment, "prova", "Uma imagem que comprova a finalização.", false));
    commands.push_back(finalizar_pedido_cmd);
    dpp::slashcommand cancelar_pedido_cmd("cancelar_pedido", "Cancela um pedido existente.", bot_id);
    cancelar_pedido_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código do pedido.", true));
    cancelar_pedido_cmd.set_default_permissions(0); cancelar_pedido_cmd.add_permission(dpp::command_permission(config.cargo_permitido, dpp::cpt_role, true)); commands.push_back(cancelar_pedido_cmd);
    dpp::slashcommand lembrete_cmd("lembrete", "Cria um lembrete pessoal para você mesmo.", bot_id);
    lembrete_cmd.add_option(dpp::command_option(dpp::co_string, "lembrete", "Descrição do que você precisa lembrar.", true));
    lembrete_cmd.add_option(dpp::command_option(dpp::co_string, "prazo", "Quando você precisa ser lembrado.", true));
    commands.push_back(lembrete_cmd);
    dpp::slashcommand finalizar_lembrete_cmd("finalizar_lembrete", "Finaliza um de seus lembretes pessoais.", bot_id);
    finalizar_lembrete_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código do lembrete a ser finalizado.", true));
    commands.push_back(finalizar_lembrete_cmd);
}