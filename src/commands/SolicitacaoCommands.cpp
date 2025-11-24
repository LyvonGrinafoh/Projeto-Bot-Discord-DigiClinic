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
#include <ctime>


SolicitacaoCommands::SolicitacaoCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler, EventHandler& eventHandler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler), eventHandler_(eventHandler)
{
}


void SolicitacaoCommands::handle_demanda_pedido(const dpp::slashcommand_t& event) {

    event.thinking(dpp::m_ephemeral);

    bool is_pedido = event.command.get_command_name() == "pedido";
    dpp::snowflake responsavel_id = std::get<dpp::snowflake>(event.get_parameter("responsavel"));
    dpp::user responsavel = event.command.get_resolved_user(responsavel_id);
    std::string texto = std::get<std::string>(event.get_parameter(is_pedido ? "pedido" : "demanda"));

    std::string prazo = "N/A";
    if (!is_pedido) {
        auto prazo_param = event.get_parameter("prazo");
        if (std::holds_alternative<std::string>(prazo_param)) {
            prazo = std::get<std::string>(prazo_param);
            if (!Utils::validarFormatoData(prazo)) {
                event.edit_response(dpp::message("❌ Formato de data inválido. O prazo deve estar no formato `DD/MM/AAAA`.").set_flags(dpp::m_ephemeral));
                return;
            }
        }
    }

    int prioridade = 1;
    auto prio_param = event.get_parameter("prioridade");
    if (std::holds_alternative<std::string>(prio_param)) {
        std::string p = std::get<std::string>(prio_param);
        if (p == "urgente") prioridade = 2;
        else if (p == "sempressa") prioridade = 0;
    }

    uint64_t novo_codigo = Utils::gerar_codigo();

    Solicitacao nova_solicitacao;
    nova_solicitacao.id = novo_codigo;
    nova_solicitacao.id_usuario_responsavel = responsavel.id;
    nova_solicitacao.nome_usuario_responsavel = responsavel.username;
    nova_solicitacao.texto = texto;
    nova_solicitacao.prazo = prazo;
    nova_solicitacao.tipo = is_pedido ? PEDIDO : DEMANDA;
    nova_solicitacao.status = "pendente";
    nova_solicitacao.prioridade = prioridade;
    nova_solicitacao.data_criacao = Utils::format_timestamp(std::time(nullptr));

    auto finish_solicitacao_creation = [this, event, nova_solicitacao, is_pedido, responsavel](const std::string& anexo_filename_for_embed) mutable {
        if (db_.addOrUpdateSolicitacao(nova_solicitacao)) {

            uint32_t color = dpp::colors::orange;
            std::string icon_prio = "🟡";
            std::string text_prio = "Padrão";

            if (nova_solicitacao.prioridade == 2) { color = dpp::colors::red; icon_prio = "🔴"; text_prio = "URGENTE"; }
            else if (nova_solicitacao.prioridade == 0) { color = dpp::colors::green; icon_prio = "🟢"; text_prio = "Sem Pressa"; }

            std::string title_prefix = is_pedido ? "📢 Novo Pedido" : "❗ Nova Demanda";

            dpp::embed embed = dpp::embed()
                .set_color(color)
                .set_title(icon_prio + " " + title_prefix)
                .add_field("Código", std::to_string(nova_solicitacao.id), false)
                .add_field("Prioridade", icon_prio + " " + text_prio, true)
                .add_field("Responsável", responsavel.get_mention(), true)
                .add_field("Criado por", event.command.get_issuing_user().get_mention(), true);

            if (!is_pedido) { embed.add_field("Prazo de Entrega", nova_solicitacao.prazo, true); }
            embed.add_field(is_pedido ? "Pedido" : "Demanda", nova_solicitacao.texto, false);

            dpp::message msg;
            msg.set_channel_id(is_pedido ? config_.canal_aviso_pedidos : config_.canal_aviso_demandas);
            msg.set_content("Nova solicitação para o responsável: " + responsavel.get_mention());

            if (!nova_solicitacao.anexo_path.empty()) {
                try {
                    std::string file_content = dpp::utility::read_file(nova_solicitacao.anexo_path);
                    msg.add_file(anexo_filename_for_embed, file_content);
                    embed.set_image("attachment://" + anexo_filename_for_embed);
                }
                catch (const dpp::exception& e) { Utils::log_to_file("Erro ao ler anexo: " + std::string(e.what())); }
            }

            msg.add_embed(embed);
            bot_.message_create(msg);

            dpp::embed dm_embed = dpp::embed()
                .set_color(color)
                .set_title("📬 Você recebeu " + (std::string)(is_pedido ? "um Pedido " : "uma Demanda ") + text_prio + "!")
                .add_field("Código", std::to_string(nova_solicitacao.id), false)
                .add_field("Prioridade", icon_prio + " " + text_prio, true)
                .add_field(is_pedido ? "Pedido" : "Demanda", nova_solicitacao.texto, false);

            if (!is_pedido) { dm_embed.add_field("Prazo de Entrega", nova_solicitacao.prazo, false); }
            if (!nova_solicitacao.anexo_path.empty()) { dm_embed.add_field("Anexo", "Um anexo foi incluído, verifique o canal principal.", false); }
            dm_embed.set_footer(dpp::embed_footer().set_text("Solicitado por: " + event.command.get_issuing_user().username));

            dpp::message dm_message; dm_message.add_embed(dm_embed); bot_.direct_message_create(responsavel.id, dm_message);

            dpp::embed log_dm_embed = dpp::embed().set_color(dpp::colors::black).set_title("LOG: Mensagem Privada Enviada").add_field("Tipo", (is_pedido ? "Novo Pedido" : "Nova Demanda"), true).add_field("Destinatário", "`" + responsavel.username + "` (`" + std::to_string(responsavel.id) + "`)", true).add_field("Autor da Solicitação", "`" + event.command.get_issuing_user().username + "` (`" + std::to_string(event.command.get_issuing_user().id) + "`)", false);
            for (const auto& field : dm_embed.fields) { log_dm_embed.add_field(field.name, field.value, field.is_inline); }
            bot_.message_create(dpp::message(config_.canal_logs, log_dm_embed));

            event.edit_response(dpp::message(std::string(is_pedido ? "Pedido" : "Demanda") + (const char*)" criado e notificado com sucesso! Código: `" + std::to_string(nova_solicitacao.id) + "`"));
        }
        else {
            event.edit_response(dpp::message("❌ Erro ao salvar a solicitação no banco de dados.").set_flags(dpp::m_ephemeral));
        }
        };


    auto param_anexo = event.get_parameter("anexo");
    if (!is_pedido && std::holds_alternative<dpp::snowflake>(param_anexo)) {
        dpp::attachment anexo = event.command.get_resolved_attachment(std::get<dpp::snowflake>(param_anexo));
        std::string anexo_filename = "demanda_" + std::to_string(nova_solicitacao.id) + "_" + anexo.filename;
        std::string caminho_salvar = "./uploads/" + anexo_filename;
        Utils::BaixarAnexo(&bot_, anexo.url, caminho_salvar, [this, event, nova_solicitacao, anexo_filename, finish_solicitacao_creation, caminho_salvar](bool sucesso) mutable {
            if (sucesso) { nova_solicitacao.anexo_path = caminho_salvar; finish_solicitacao_creation(anexo_filename); }
            else { event.edit_response(dpp::message("❌ Erro ao baixar o anexo. A solicitação não foi salva.").set_flags(dpp::m_ephemeral)); }
            });
    }
    else {
        finish_solicitacao_creation("");
    }
}

// ... (Mantenha handle_finalizar_solicitacao, handle_cancelar_demanda e handle_limpar_demandas IGUAIS) ...

void SolicitacaoCommands::handle_finalizar_solicitacao(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo")); uint64_t codigo = static_cast<uint64_t>(codigo_int); std::string command_name = event.command.get_command_name();
    std::string observacao = "Nenhuma.";
    auto obs_param = event.get_parameter("observacao"); if (std::holds_alternative<std::string>(obs_param)) { observacao = std::get<std::string>(obs_param); }
    std::optional<Solicitacao> item_opt = db_.getSolicitacao(codigo);

    if (item_opt) {
        Solicitacao item_concluido = *item_opt;
        if (command_name == "cancelar_pedido") { if (item_concluido.tipo != PEDIDO) { event.reply(dpp::message("❌ Este comando é apenas para cancelar **pedidos**.").set_flags(dpp::m_ephemeral)); return; } }
        if (item_concluido.status != "pendente") { event.reply(dpp::message("❌ Esta solicitação não está mais pendente (Status: " + item_concluido.status + ").").set_flags(dpp::m_ephemeral)); return; }
        bool is_cancelamento = (command_name == "cancelar_pedido");
        item_concluido.status = is_cancelamento ? "cancelada" : "finalizada";
        item_concluido.data_finalizacao = Utils::format_timestamp(std::time(nullptr));

        if (db_.addOrUpdateSolicitacao(item_concluido)) {
            std::string tipo_str; std::string title; dpp::snowflake channel_id = 0;
            switch (item_concluido.tipo) {
            case DEMANDA: tipo_str = "Demanda"; title = "✅ Demanda Finalizada"; channel_id = config_.canal_finalizadas; break;
            case PEDIDO: tipo_str = "Pedido"; title = is_cancelamento ? "❌ Pedido Cancelado" : "✅ Pedido Finalizado"; channel_id = config_.canal_pedidos_concluidos; break;
            case LEMBRETE: tipo_str = "Lembrete"; db_.removeSolicitacao(codigo); cmdHandler_.replyAndDelete(event, dpp::message("✅ Lembrete `" + std::to_string(codigo) + "` finalizado com sucesso!")); return;
            }
            if (channel_id != 0) {
                dpp::embed embed = dpp::embed().set_color(is_cancelamento ? dpp::colors::red : dpp::colors::dark_green).set_title(title).add_field("Código", std::to_string(item_concluido.id), false).add_field("Responsável", "<@" + std::to_string(item_concluido.id_usuario_responsavel) + ">", true).add_field(is_cancelamento ? "Cancelado por" : "Finalizado por", event.command.get_issuing_user().get_mention(), true).add_field(tipo_str, item_concluido.texto, false);
                if (item_concluido.tipo == DEMANDA) { embed.add_field("Prazo de Entrega", item_concluido.prazo, true); }
                if (!is_cancelamento) {
                    if (observacao != "Nenhuma.") { embed.add_field("Observação", observacao, false); }
                    auto param_anexo = event.get_parameter("prova"); if (auto attachment_id_ptr = std::get_if<dpp::snowflake>(&param_anexo)) { dpp::attachment anexo = event.command.get_resolved_attachment(*attachment_id_ptr); if (anexo.content_type.find("image/") == 0) { embed.set_image(anexo.url); } }
                }
                bot_.message_create(dpp::message(channel_id, embed));
            }
            cmdHandler_.replyAndDelete(event, dpp::message(tipo_str + " `" + std::to_string(codigo) + "` " + (is_cancelamento ? "cancelado" : "finalizado") + " com sucesso!"));
        }
        else { event.reply(dpp::message("❌ Erro ao atualizar a solicitação no banco de dados.").set_flags(dpp::m_ephemeral)); }
    }
    else { event.reply(dpp::message("❌ Código não encontrado no banco de dados.").set_flags(dpp::m_ephemeral)); }
}

void SolicitacaoCommands::handle_cancelar_demanda(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo")); uint64_t codigo = static_cast<uint64_t>(codigo_int);
    std::optional<Solicitacao> item_opt = db_.getSolicitacao(codigo);
    if (!item_opt) { event.reply(dpp::message("❌ Código não encontrado.").set_flags(dpp::m_ephemeral)); return; }
    Solicitacao item = *item_opt;
    if (item.tipo != DEMANDA) { event.reply(dpp::message("❌ Este comando é apenas para cancelar **demandas**.").set_flags(dpp::m_ephemeral)); return; }
    if (item.status != "pendente") { event.reply(dpp::message("❌ Esta demanda não pode ser cancelada (Status: " + item.status + ").").set_flags(dpp::m_ephemeral)); return; }
    item.status = "cancelada"; item.data_finalizacao = Utils::format_timestamp(std::time(nullptr));
    if (db_.addOrUpdateSolicitacao(item)) {
        dpp::embed embed = dpp::embed().set_color(dpp::colors::red).set_title("❌ Demanda Cancelada").add_field("Código", std::to_string(item.id), false).add_field("Responsável", "<@" + std::to_string(item.id_usuario_responsavel) + ">", true).add_field("Cancelado por", event.command.get_issuing_user().get_mention(), true).add_field("Demanda", item.texto, false).add_field("Prazo (Original)", item.prazo, true);
        bot_.message_create(dpp::message(config_.canal_finalizadas, embed));
        cmdHandler_.replyAndDelete(event, dpp::message("✅ Demanda `" + std::to_string(codigo) + "` cancelada com sucesso."));
    }
    else { event.reply(dpp::message("❌ Erro ao salvar a demanda cancelada no banco de dados.").set_flags(dpp::m_ephemeral)); }
}

void SolicitacaoCommands::handle_limpar_demandas(const dpp::slashcommand_t& event) {
    auto solicitacoes_map = db_.getSolicitacoes(); std::vector<uint64_t> ids_para_remover;
    for (auto const& [id, sol] : solicitacoes_map) { if (sol.status == "pendente" || sol.tipo == LEMBRETE) { ids_para_remover.push_back(id); } }
    if (ids_para_remover.empty()) { event.reply(dpp::message("🧹 Nenhuma solicitação ativa (pendente) encontrada para limpar.").set_flags(dpp::m_ephemeral)); return; }
    int count = 0; for (uint64_t id : ids_para_remover) { if (db_.removeSolicitacao(id)) { count++; } }
    event.reply(dpp::message("🧹 " + std::to_string(count) + " solicitações ativas (pendentes/lembretes) foram limpas.").set_flags(dpp::m_ephemeral));
}

void SolicitacaoCommands::handle_lista_demandas(const dpp::slashcommand_t& event) {
    dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("usuario"));
    std::string status_filtro = "pendente";
    auto param = event.get_parameter("status");
    if (auto val_ptr = std::get_if<std::string>(&param)) { status_filtro = *val_ptr; }

    std::vector<PaginatedItem> solicitacoes_do_usuario;
    const auto& todas_solicitacoes = db_.getSolicitacoes();

    for (const auto& [id, solicitacao] : todas_solicitacoes) {
        if (solicitacao.id_usuario_responsavel == user_id) {
            if (status_filtro == "todas") { solicitacoes_do_usuario.push_back(solicitacao); }
            else if (solicitacao.status == status_filtro) { solicitacoes_do_usuario.push_back(solicitacao); }
        }
    }

    std::sort(solicitacoes_do_usuario.begin(), solicitacoes_do_usuario.end(), [](const PaginatedItem& a, const PaginatedItem& b) {
        const auto& sol_a = std::get<Solicitacao>(a); const auto& sol_b = std::get<Solicitacao>(b);
        if (sol_a.prioridade != sol_b.prioridade) { return sol_a.prioridade > sol_b.prioridade; }
        return sol_a.tipo < sol_b.tipo;
        });

    if (solicitacoes_do_usuario.empty()) {
        event.reply(dpp::message("Este usuário não possui nenhuma solicitação com o status `" + status_filtro + "`.").set_flags(dpp::m_ephemeral));
        return;
    }

    PaginationState state;
    state.channel_id = event.command.channel_id;
    state.items = std::move(solicitacoes_do_usuario);
    state.originalUserID = event.command.get_issuing_user().id;
    state.listType = "demandas";
    state.currentPage = 1;
    state.itemsPerPage = 5;

    dpp::embed firstPageEmbed = Utils::generatePageEmbed(state);
    bool needsPagination = state.items.size() > state.itemsPerPage;

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
                                bot_.message_add_reaction(msg.id, msg.channel_id, "◀️", [this, msg](const dpp::confirmation_callback_t& reaction_cb) { if (!reaction_cb.is_error()) { bot_.message_add_reaction(msg.id, msg.channel_id, "▶️", [this, msg](const auto& r2) { if (!r2.is_error()) bot_.message_add_reaction(msg.id, msg.channel_id, "🗑️"); }); } });
                            }
                            else { Utils::log_to_file("Erro: get_original_response não retornou um dpp::message."); }
                        }
                        else { Utils::log_to_file("Erro ao *buscar* a resposta original: " + msg_cb.get_error().message); }
                        });
                }
                else {
                    event.get_original_response([this, event](const dpp::confirmation_callback_t& msg_cb) {
                        if (!msg_cb.is_error()) {
                            auto* msg_ptr = std::get_if<dpp::message>(&msg_cb.value);
                            if (msg_ptr) {
                            }
                        }
                        });

                    dpp::slashcommand_t event_copy = event;
                    bot_.start_timer([this, event_copy](dpp::timer timer_handle) mutable {
                        event_copy.delete_original_response([](const dpp::confirmation_callback_t& delete_cb) {
                            if (delete_cb.is_error() && delete_cb.get_error().code != 10062) { Utils::log_to_file("Falha ao auto-deletar: " + delete_cb.get_error().message); }
                            });
                        bot_.stop_timer(timer_handle);
                        }, 60);
                }
            }
            else { Utils::log_to_file("Erro ao *enviar* a resposta inicial: " + cb.get_error().message); }
        });
}

void SolicitacaoCommands::handle_lembrete(const dpp::slashcommand_t& event) {
    dpp::user autor = event.command.get_issuing_user();
    Solicitacao novo_lembrete;
    novo_lembrete.id = Utils::gerar_codigo();
    novo_lembrete.id_usuario_responsavel = autor.id;
    novo_lembrete.nome_usuario_responsavel = autor.username;
    novo_lembrete.texto = std::get<std::string>(event.get_parameter("lembrete"));

    auto prazo_param = event.get_parameter("prazo");
    if (std::holds_alternative<std::string>(prazo_param)) {
        novo_lembrete.prazo = std::get<std::string>(prazo_param);
        if (!Utils::validarFormatoData(novo_lembrete.prazo)) {
            event.reply(dpp::message("❌ Formato de data inválido. O prazo deve estar no formato `DD/MM/AAAA`.").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    else {
        novo_lembrete.prazo = "N/A";
    }

    novo_lembrete.tipo = LEMBRETE;
    novo_lembrete.status = "pendente";

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
    demanda_cmd.add_option(dpp::command_option(dpp::co_string, "prioridade", "Nível de urgência (Padrão: Normal).", true).add_choice(dpp::command_option_choice("🔴 Urgente", std::string("urgente"))).add_choice(dpp::command_option_choice("🟡 Padrão", std::string("padrao"))).add_choice(dpp::command_option_choice("🟢 Sem Pressa", std::string("sempressa"))));
    demanda_cmd.add_option(dpp::command_option(dpp::co_string, "prazo", "Prazo (DD/MM/AAAA). Opcional.", false));
    demanda_cmd.add_option(dpp::command_option(dpp::co_attachment, "anexo", "Uma imagem de referência para a demanda (opcional).", false));
    demanda_cmd.set_default_permissions(0); demanda_cmd.add_permission(dpp::command_permission(config.cargo_permitido, dpp::cpt_role, true));
    commands.push_back(demanda_cmd);

    dpp::slashcommand finalizar_demanda_cmd("finalizar_demanda", "Finaliza uma demanda existente.", bot_id);
    finalizar_demanda_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código de 10 dígitos da demanda.", true));
    finalizar_demanda_cmd.add_option(dpp::command_option(dpp::co_attachment, "prova", "Uma imagem que comprova a finalização.", false));
    finalizar_demanda_cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "Uma observação sobre a finalização (opcional).", false));
    commands.push_back(finalizar_demanda_cmd);

    dpp::slashcommand cancelar_demanda_cmd("cancelar_demanda", "Cancela uma demanda pendente.", bot_id);
    cancelar_demanda_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código da demanda a ser cancelada.", true));
    cancelar_demanda_cmd.set_default_permissions(0);
    cancelar_demanda_cmd.add_permission(dpp::command_permission(config.cargo_permitido, dpp::cpt_role, true));
    commands.push_back(cancelar_demanda_cmd);

    dpp::slashcommand limpar_cmd("limpar_demandas", "Limpa TODAS as demandas, pedidos e lembretes ativos (pendentes).", bot_id);
    limpar_cmd.default_member_permissions = dpp::p_administrator;
    commands.push_back(limpar_cmd);

    dpp::slashcommand lista_cmd("lista_demandas", "Mostra as pendências (demandas, pedidos, lembretes) de um usuário.", bot_id);
    lista_cmd.add_option(dpp::command_option(dpp::co_user, "usuario", "O usuário que você quer consultar.", true));
    lista_cmd.add_option(dpp::command_option(dpp::co_string, "status", "Filtrar por status (padrão: pendente).", false).add_choice(dpp::command_option_choice("Pendentes", std::string("pendente"))).add_choice(dpp::command_option_choice("Finalizadas", std::string("finalizada"))).add_choice(dpp::command_option_choice("Canceladas", std::string("cancelada"))).add_choice(dpp::command_option_choice("Todas", std::string("todas"))));
    commands.push_back(lista_cmd);

    dpp::slashcommand pedido_cmd("pedido", "Cria um novo pedido para um usuário.", bot_id);
    pedido_cmd.add_option(dpp::command_option(dpp::co_user, "responsavel", "O usuário que receberá o pedido.", true));
    pedido_cmd.add_option(dpp::command_option(dpp::co_string, "pedido", "Descrição detalhada do pedido.", true));
    pedido_cmd.add_option(dpp::command_option(dpp::co_string, "prioridade", "Nível de urgência (Padrão: Normal).", true).add_choice(dpp::command_option_choice("🔴 Urgente", std::string("urgente"))).add_choice(dpp::command_option_choice("🟡 Padrão", std::string("padrao"))).add_choice(dpp::command_option_choice("🟢 Sem Pressa", std::string("sempressa"))));
    commands.push_back(pedido_cmd);

    dpp::slashcommand finalizar_pedido_cmd("finalizar_pedido", "Finaliza um pedido existente.", bot_id);
    finalizar_pedido_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código do pedido.", true));
    finalizar_pedido_cmd.add_option(dpp::command_option(dpp::co_attachment, "prova", "Uma imagem que comprova a finalização.", false));
    finalizar_pedido_cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "Uma observação sobre a finalização (opcional).", false));
    commands.push_back(finalizar_pedido_cmd);

    dpp::slashcommand cancelar_pedido_cmd("cancelar_pedido", "Cancela um pedido existente.", bot_id);
    cancelar_pedido_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código do pedido.", true));
    cancelar_pedido_cmd.set_default_permissions(0);
    cancelar_pedido_cmd.add_permission(dpp::command_permission(config.cargo_permitido, dpp::cpt_role, true));
    commands.push_back(cancelar_pedido_cmd);

    dpp::slashcommand lembrete_cmd("lembrete", "Cria um lembrete pessoal para você mesmo.", bot_id);
    lembrete_cmd.add_option(dpp::command_option(dpp::co_string, "lembrete", "Descrição do que você precisa lembrar.", true));
    lembrete_cmd.add_option(dpp::command_option(dpp::co_string, "prazo", "Data do lembrete (DD/MM/AAAA). Opcional.", false));
    commands.push_back(lembrete_cmd);

    dpp::slashcommand finalizar_lembrete_cmd("finalizar_lembrete", "Finaliza um de seus lembretes pessoais.", bot_id);
    finalizar_lembrete_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código do lembrete a ser finalizado.", true));
    commands.push_back(finalizar_lembrete_cmd);
}