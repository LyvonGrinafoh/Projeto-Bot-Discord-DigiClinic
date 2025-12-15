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
    event.thinking(false);

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
                cmdHandler_.replyAndDelete(event, dpp::message("❌ Data inválida. Use DD/MM/AAAA."));
                return;
            }
        }
    }

    int prioridade = 1;
    auto prio_param = event.get_parameter("prioridade");
    if (std::holds_alternative<std::string>(prio_param)) {
        std::string p = std::get<std::string>(prio_param);
        if (p == "urgente") prioridade = 2; else if (p == "sempressa") prioridade = 0;
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

    auto finish = [this, event, nova_solicitacao, is_pedido, responsavel](const std::string& filename, const std::string& content) mutable {
        if (db_.addOrUpdateSolicitacao(nova_solicitacao)) {
            uint32_t color = (nova_solicitacao.prioridade == 2) ? dpp::colors::red : ((nova_solicitacao.prioridade == 0) ? dpp::colors::green : dpp::colors::orange);
            std::string title = is_pedido ? "📢 Novo Pedido" : "❗ Nova Demanda";

            dpp::embed embed = Utils::criarEmbedPadrao(title, nova_solicitacao.texto, color);
            embed.add_field("Código", std::to_string(nova_solicitacao.id), false)
                .add_field("Responsável", responsavel.get_mention(), true)
                .add_field("Criado por", event.command.get_issuing_user().get_mention(), true);
            if (!is_pedido) embed.add_field("Prazo", nova_solicitacao.prazo, true);

            dpp::message msg;
            msg.set_channel_id(is_pedido ? config_.canal_aviso_pedidos : config_.canal_aviso_demandas);
            msg.set_content("Atenção: " + responsavel.get_mention());

            if (!filename.empty() && !content.empty()) {
                msg.add_file(filename, content);
                embed.set_image("attachment://" + filename);
            }
            msg.add_embed(embed);
            bot_.message_create(msg);

            cmdHandler_.editAndDelete(event, dpp::message("✅ " + std::string(is_pedido ? "Pedido" : "Demanda") + " criado! Código: `" + std::to_string(nova_solicitacao.id) + "`"));
        }
        else {
            cmdHandler_.editAndDelete(event, dpp::message("❌ Erro ao salvar."));
        }
        };

    auto param_anexo = event.get_parameter("anexo");
    if (!is_pedido && std::holds_alternative<dpp::snowflake>(param_anexo)) {
        dpp::attachment anexo = event.command.get_resolved_attachment(std::get<dpp::snowflake>(param_anexo));
        std::string filename = "demanda_" + std::to_string(nova_solicitacao.id) + "_" + anexo.filename;
        std::string path = "./uploads/" + filename;
        Utils::BaixarAnexo(&bot_, anexo.url, path, [this, event, nova_solicitacao, path, filename, finish](bool ok, std::string c) mutable {
            if (ok) { nova_solicitacao.anexo_path = path; finish(filename, c); }
            else { cmdHandler_.editAndDelete(event, dpp::message("❌ Erro no anexo.")); }
            });
    }
    else { finish("", ""); }
}

void SolicitacaoCommands::handle_finalizar_solicitacao_form(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo"));
    std::string codigo_str = std::to_string(codigo_int);
    dpp::interaction_modal_response modal("modal_fin_" + codigo_str, "Finalizar #" + codigo_str);
    modal.add_component(dpp::component().set_label("Obs").set_id("obs_field").set_type(dpp::cot_text).set_text_style(dpp::text_paragraph));
    event.dialog(modal);
}

void SolicitacaoCommands::handle_cancelar_demanda(const dpp::slashcommand_t& event) {
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    std::optional<Solicitacao> item_opt = db_.getSolicitacao(codigo);

    if (!item_opt || item_opt->status != "pendente") {
        cmdHandler_.replyAndDelete(event, dpp::message("❌ Não encontrado ou não pendente."));
        return;
    }

    Solicitacao item = *item_opt;
    item.status = "cancelada";
    item.data_finalizacao = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateSolicitacao(item)) {
        dpp::embed embed = Utils::criarEmbedPadrao("❌ Demanda Cancelada", item.texto, dpp::colors::red);
        embed.add_field("Código", std::to_string(item.id), true);
        bot_.message_create(dpp::message(config_.canal_finalizadas, embed));
        cmdHandler_.replyAndDelete(event, dpp::message("✅ Cancelada com sucesso."));
    }
}

void SolicitacaoCommands::handle_limpar_demandas(const dpp::slashcommand_t& event) {
    db_.clearSolicitacoes();
    cmdHandler_.replyAndDelete(event, dpp::message("🧹 Limpeza concluída."));
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
        cmdHandler_.replyAndDelete(event, dpp::message("Nenhuma solicitação encontrada."));
        return;
    }

    PaginationState state;
    state.channel_id = event.command.channel_id;
    state.items = std::move(solicitacoes_do_usuario);
    state.originalUserID = event.command.get_issuing_user().id;
    state.listType = "demandas";
    state.currentPage = 1;
    state.itemsPerPage = 5;

    dpp::embed firstPage = Utils::generatePageEmbed(state);
    event.reply(dpp::message(event.command.channel_id, firstPage), [this, state, event](const auto& cb) {
        if (!cb.is_error()) {
            event.get_original_response([this, state](const auto& msg_cb) {
                if (!msg_cb.is_error()) {
                    auto m = std::get<dpp::message>(msg_cb.value);
                    eventHandler_.addPaginationState(m.id, state);

                    bot_.message_add_reaction(m.id, m.channel_id, "🗑️", [this, m](auto cb) {
                        if (!cb.is_error()) {
                            bot_.message_add_reaction(m.id, m.channel_id, "◀️", [this, m](auto cb2) {
                                if (!cb2.is_error()) bot_.message_add_reaction(m.id, m.channel_id, "▶️");
                                });
                        }
                        });

                    bot_.start_timer([this, m](dpp::timer t) { bot_.message_delete(m.id, m.channel_id); bot_.stop_timer(t); }, 60);
                }
                });
        }
        });
}

void SolicitacaoCommands::handle_ver_demanda(const dpp::slashcommand_t& event) {
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    auto sol_opt = db_.getSolicitacao(codigo);
    if (!sol_opt) { cmdHandler_.replyAndDelete(event, dpp::message("❌ 404 Not Found")); return; }

    Solicitacao s = *sol_opt;
    dpp::embed embed = Utils::criarEmbedPadrao("Detalhes #" + std::to_string(s.id), s.texto, dpp::colors::blue);
    embed.add_field("Status", s.status, true);

    dpp::message msg(event.command.channel_id, embed);
    if (s.status == "pendente") {
        msg.add_component(dpp::component().add_component(dpp::component().set_type(dpp::cot_button).set_label("Finalizar").set_id("btn_fin_" + std::to_string(s.id))));
    }

    event.reply(msg, [this, event](const auto& cb) {
        if (!cb.is_error()) {
            event.get_original_response([this, event](const auto& m_cb) {
                if (!m_cb.is_error()) {
                    auto m = std::get<dpp::message>(m_cb.value);
                    bot_.message_add_reaction(m.id, m.channel_id, "🗑️");
                    bot_.start_timer([this, m](auto t) { bot_.message_delete(m.id, m.channel_id); bot_.stop_timer(t); }, 60);
                }
                });
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

    auto prazo_param = event.get_parameter("prazo");
    if (std::holds_alternative<std::string>(prazo_param)) {
        novo_lembrete.prazo = std::get<std::string>(prazo_param);
        if (!Utils::validarFormatoData(novo_lembrete.prazo)) {
            event.reply(dpp::message("❌ Formato de data inválido.").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    else { novo_lembrete.prazo = "N/A"; }
    novo_lembrete.tipo = LEMBRETE;

    if (db_.addOrUpdateSolicitacao(novo_lembrete)) {
        cmdHandler_.replyAndDelete(event, dpp::message("✅ Lembrete criado com sucesso! Código: `" + std::to_string(novo_lembrete.id) + "`"));
    }
}

void SolicitacaoCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id, const BotConfig& config) {
    dpp::slashcommand demanda_cmd("demanda", "Cria demanda.", bot_id);
    demanda_cmd.add_option(dpp::command_option(dpp::co_user, "responsavel", "Resp.", true));
    demanda_cmd.add_option(dpp::command_option(dpp::co_string, "demanda", "Desc.", true));
    demanda_cmd.add_option(dpp::command_option(dpp::co_string, "prioridade", "Prio.", true).add_choice(dpp::command_option_choice("Urgente", "urgente")).add_choice(dpp::command_option_choice("Padrao", "padrao")).add_choice(dpp::command_option_choice("Sem Pressa", "sempressa")));
    demanda_cmd.add_option(dpp::command_option(dpp::co_string, "prazo", "DD/MM/AAAA", false));
    demanda_cmd.add_option(dpp::command_option(dpp::co_attachment, "anexo", "Img", false));
    commands.push_back(demanda_cmd);

    dpp::slashcommand ver("ver_demanda", "Detalhes.", bot_id); ver.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID", true)); commands.push_back(ver);
    dpp::slashcommand fin("finalizar_demanda", "Fin.", bot_id); fin.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID", true)); commands.push_back(fin);
    dpp::slashcommand finp("finalizar_pedido", "Fin.", bot_id); finp.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID", true)); commands.push_back(finp);
    dpp::slashcommand can("cancelar_demanda", "Can.", bot_id); can.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID", true)); commands.push_back(can);
    dpp::slashcommand limp("limpar_demandas", "Limpa.", bot_id); commands.push_back(limp);
    dpp::slashcommand lis("lista_demandas", "Lista.", bot_id); lis.add_option(dpp::command_option(dpp::co_user, "usuario", "User", true)); lis.add_option(dpp::command_option(dpp::co_string, "status", "Stat", false)); commands.push_back(lis);
    dpp::slashcommand ped("pedido", "Ped.", bot_id); ped.add_option(dpp::command_option(dpp::co_user, "responsavel", "User", true)); ped.add_option(dpp::command_option(dpp::co_string, "pedido", "Desc", true)); ped.add_option(dpp::command_option(dpp::co_string, "prioridade", "Prio", true)); commands.push_back(ped);
    dpp::slashcommand canp("cancelar_pedido", "Can.", bot_id); canp.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID", true)); commands.push_back(canp);
    dpp::slashcommand lem("lembrete", "Lemb.", bot_id); lem.add_option(dpp::command_option(dpp::co_string, "lembrete", "Desc", true)); lem.add_option(dpp::command_option(dpp::co_string, "prazo", "Data", false)); commands.push_back(lem);
    dpp::slashcommand finl("finalizar_lembrete", "Fin.", bot_id); finl.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID", true)); commands.push_back(finl);
}