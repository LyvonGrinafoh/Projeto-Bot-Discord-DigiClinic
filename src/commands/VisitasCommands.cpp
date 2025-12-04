#include "VisitasCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "EventHandler.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <variant>
#include <string>
#include <vector>

VisitasCommands::VisitasCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler, EventHandler& eventHandler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler), eventHandler_(eventHandler) {
}

void VisitasCommands::handle_visitas(const dpp::slashcommand_t& event) {
    std::string data = std::get<std::string>(event.get_parameter("data"));

    if (!Utils::validarFormatoData(data)) {
        event.reply(dpp::message("❌ Formato de data inválido. Use `DD/MM/AAAA`.").set_flags(dpp::m_ephemeral));
        return;
    }

    Visita nova_visita;
    nova_visita.id = Utils::gerar_codigo();
    nova_visita.quem_marcou_id = event.command.get_issuing_user().id;
    nova_visita.quem_marcou_nome = event.command.get_issuing_user().username;
    nova_visita.doutor = std::get<std::string>(event.get_parameter("doutor"));
    nova_visita.area = std::get<std::string>(event.get_parameter("area"));
    nova_visita.data = data;
    nova_visita.horario = std::get<std::string>(event.get_parameter("horario"));
    nova_visita.unidade = std::get<std::string>(event.get_parameter("unidade"));
    nova_visita.status = "agendada";

    auto tel_param = event.get_parameter("telefone");
    if (std::holds_alternative<std::string>(tel_param)) {
        nova_visita.telefone = std::get<std::string>(tel_param);
    }
    else { nova_visita.telefone = "Nao informado"; }

    auto obs_param = event.get_parameter("observacao");
    if (std::holds_alternative<std::string>(obs_param)) {
        nova_visita.observacoes = std::get<std::string>(obs_param);
    }
    else { nova_visita.observacoes = "Nenhuma"; }

    if (db_.addOrUpdateVisita(nova_visita)) {
        dpp::embed embed = dpp::embed().set_color(dpp::colors::blue).set_title("📅 Nova Visita Agendada").add_field("Doutor(a)", nova_visita.doutor, true).add_field("Data", nova_visita.data, true).add_field("Horário", nova_visita.horario, true).add_field("Unidade", nova_visita.unidade, true).add_field("Área", nova_visita.area, true).add_field("Telefone", nova_visita.telefone, true).add_field("Observação", nova_visita.observacoes, false).set_footer(dpp::embed_footer().set_text("Agendado por: " + nova_visita.quem_marcou_nome + " | Código: " + std::to_string(nova_visita.id)));
        dpp::message msg(config_.canal_visitas, embed);
        bot_.message_create(msg);
        cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita agendada com sucesso! Código: `" + std::to_string(nova_visita.id) + "`"));
    }
    else {
        event.reply(dpp::message("❌ Erro ao salvar a visita no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void VisitasCommands::handle_cancelar_visita(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo"));
    uint64_t codigo = static_cast<uint64_t>(codigo_int);
    std::string motivo = std::get<std::string>(event.get_parameter("motivo"));

    std::optional<Visita> visita_opt = db_.getVisita(codigo);
    if (visita_opt) {
        Visita visita = *visita_opt;
        if (visita.status == "cancelada") {
            event.reply(dpp::message("❌ Esta visita já está cancelada.").set_flags(dpp::m_ephemeral));
            return;
        }
        visita.status = "cancelada";
        visita.observacoes += " [CANCELADA: " + motivo + "]";

        if (db_.addOrUpdateVisita(visita)) {
            dpp::embed embed = dpp::embed().set_color(dpp::colors::red).set_title("❌ Visita Cancelada").add_field("Doutor(a)", visita.doutor, true).add_field("Data Original", visita.data, true).add_field("Motivo", motivo, false).set_footer(dpp::embed_footer().set_text("Cancelado por: " + event.command.get_issuing_user().username));
            bot_.message_create(dpp::message(config_.canal_visitas, embed));
            cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita cancelada com sucesso."));
        }
        else {
            event.reply(dpp::message("❌ Erro ao atualizar o status da visita.").set_flags(dpp::m_ephemeral));
        }
    }
    else {
        event.reply(dpp::message("❌ Visita não encontrada com esse código.").set_flags(dpp::m_ephemeral));
    }
}

void VisitasCommands::handle_finalizar_visita(const dpp::slashcommand_t& event) {
    event.thinking(false);
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo"));
    uint64_t codigo = static_cast<uint64_t>(codigo_int);
    std::string relatorio = std::get<std::string>(event.get_parameter("relatorio"));

    std::optional<Visita> visita_opt = db_.getVisita(codigo);
    if (!visita_opt) {
        event.edit_response(dpp::message("❌ Código de visita não encontrado."));
        return;
    }
    Visita visita = *visita_opt;

    if (visita.status != "agendada") {
        event.edit_response(dpp::message("❌ Esta visita não está agendada."));
        return;
    }

    visita.status = "realizada";
    visita.relatorio_visita = relatorio;

    auto finish = [this, event, visita, relatorio](const std::string& filename) mutable {
        if (db_.addOrUpdateVisita(visita)) {
            dpp::embed embed = dpp::embed().set_color(dpp::colors::green).set_title("✅ Visita Realizada").add_field("Doutor(a)", visita.doutor, true).add_field("Data", visita.data, true).add_field("Relatório", relatorio, false).set_footer(dpp::embed_footer().set_text("Finalizado por: " + event.command.get_issuing_user().username));
            dpp::message msg(config_.canal_visitas, embed);

            if (!visita.ficha_path.empty()) {
                try {
                    msg.add_file(filename, dpp::utility::read_file(visita.ficha_path));
                    embed.set_image("attachment://" + filename);
                }
                catch (...) {}
            }
            msg.add_embed(embed);

            bot_.message_create(msg);
            event.edit_response(dpp::message("✅ Visita marcada como realizada!"));
        }
        else {
            event.edit_response(dpp::message("❌ Erro ao atualizar a visita."));
        }
        };

    auto param_ficha = event.get_parameter("ficha");
    if (std::holds_alternative<dpp::snowflake>(param_ficha)) {
        dpp::attachment anexo = event.command.get_resolved_attachment(std::get<dpp::snowflake>(param_ficha));
        std::string filename = "visita_" + std::to_string(visita.id) + "_" + anexo.filename;
        std::string path = "./uploads/" + filename;

        Utils::BaixarAnexo(&bot_, anexo.url, path, [this, event, visita, path, filename, finish](bool ok) mutable {
            if (ok) { visita.ficha_path = path; finish(filename); }
            else { event.edit_response("Erro ao baixar ficha."); }
            });
    }
    else {
        finish("");
    }
}

void VisitasCommands::handle_lista_visitas(const dpp::slashcommand_t& event) {
    std::string status_filtro = "agendada";
    auto param = event.get_parameter("status");
    if (auto val_ptr = std::get_if<std::string>(&param)) { status_filtro = *val_ptr; }

    const auto& visitas_map = db_.getVisitas();
    std::vector<PaginatedItem> filtered_items;

    for (const auto& [id, visita] : visitas_map) {
        if (status_filtro == "todas") filtered_items.push_back(visita);
        else if (visita.status == status_filtro) filtered_items.push_back(visita);
    }

    std::sort(filtered_items.begin(), filtered_items.end(), [](const PaginatedItem& a, const PaginatedItem& b) {
        return std::get<Visita>(a).id < std::get<Visita>(b).id;
        });

    if (filtered_items.empty()) {
        event.reply(dpp::message("Nenhuma visita encontrada.").set_flags(dpp::m_ephemeral));
        return;
    }

    PaginationState state;
    state.channel_id = event.command.channel_id;
    state.items = std::move(filtered_items);
    state.originalUserID = event.command.get_issuing_user().id;
    state.listType = "visitas";
    state.currentPage = 1;
    state.itemsPerPage = 5;

    dpp::embed firstPageEmbed = Utils::generatePageEmbed(state);
    bool needsPagination = state.items.size() > state.itemsPerPage;

    event.reply(dpp::message(event.command.channel_id, firstPageEmbed),
        [this, state, needsPagination, event](const dpp::confirmation_callback_t& cb) mutable {
            if (!cb.is_error() && needsPagination) {
                event.get_original_response([this, state](const dpp::confirmation_callback_t& msg_cb) mutable {
                    if (!msg_cb.is_error()) {
                        auto& msg = std::get<dpp::message>(msg_cb.value);
                        eventHandler_.addPaginationState(msg.id, std::move(state));
                        bot_.message_add_reaction(msg.id, msg.channel_id, "◀️", [this, msg](auto r) {
                            if (!r.is_error()) bot_.message_add_reaction(msg.id, msg.channel_id, "▶️");
                            });
                    }
                    });
            }
        });
}

void VisitasCommands::handle_modificar_visita(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo"));
    uint64_t codigo = static_cast<uint64_t>(codigo_int);
    Visita* visita_ptr = db_.getVisitaPtr(codigo);

    if (visita_ptr) {
        Visita& visita = *visita_ptr;
        bool alterado = false;

        auto check_update = [&](const std::string& param_name, std::string& field) {
            auto param = event.get_parameter(param_name);
            if (auto val_ptr = std::get_if<std::string>(&param)) {
                if (*val_ptr != field) { field = *val_ptr; return true; }
            }
            return false;
            };

        alterado |= check_update("doutor", visita.doutor);

        auto data_param = event.get_parameter("data");
        if (auto val_ptr = std::get_if<std::string>(&data_param)) {
            if (*val_ptr != visita.data) {
                if (Utils::validarFormatoData(*val_ptr)) { visita.data = *val_ptr; alterado = true; }
                else { event.reply(dpp::message("❌ Data inválida.").set_flags(dpp::m_ephemeral)); return; }
            }
        }

        alterado |= check_update("horario", visita.horario);
        alterado |= check_update("unidade", visita.unidade);
        alterado |= check_update("area", visita.area);
        alterado |= check_update("telefone", visita.telefone);

        auto obs_param = event.get_parameter("observacao");
        if (auto val_ptr = std::get_if<std::string>(&obs_param)) {
            if (!visita.observacoes.empty()) visita.observacoes += " | ";
            visita.observacoes += *val_ptr;
            alterado = true;
        }

        if (alterado) {
            if (db_.saveVisitas()) cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita atualizada!"));
            else event.reply(dpp::message("❌ Erro ao salvar.").set_flags(dpp::m_ephemeral));
        }
        else { event.reply(dpp::message("⚠️ Nenhuma alteração.").set_flags(dpp::m_ephemeral)); }
    }
    else { event.reply(dpp::message("❌ Visita não encontrada.").set_flags(dpp::m_ephemeral)); }
}

void VisitasCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand visitas_cmd("visitas", "Agenda uma nova visita.", bot_id);
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome do doutor(a).", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Especialidade/Área.", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "data", "Data da visita (DD/MM/AAAA).", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "horario", "Horário da visita.", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade da visita.", true).add_choice(dpp::command_option_choice("Tatuapé", std::string("Tatuapé"))).add_choice(dpp::command_option_choice("Campo Belo", std::string("Campo Belo"))));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "telefone", "Telefone de contato.", false));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "Observações.", false));
    commands.push_back(visitas_cmd);

    dpp::slashcommand cancelar_visita_cmd("cancelar_visita", "Cancela uma visita agendada.", bot_id);
    cancelar_visita_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "Código da visita.", true));
    cancelar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "motivo", "Motivo do cancelamento.", true));
    commands.push_back(cancelar_visita_cmd);

    dpp::slashcommand finalizar_visita_cmd("finalizar_visita", "Marca uma visita como realizada.", bot_id);
    finalizar_visita_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "Código da visita.", true));
    finalizar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "relatorio", "Breve relatório.", true));
    finalizar_visita_cmd.add_option(dpp::command_option(dpp::co_attachment, "ficha", "Foto da ficha (opcional).", false));
    commands.push_back(finalizar_visita_cmd);

    dpp::slashcommand lista_visitas_cmd("lista_visitas", "Lista as visitas cadastradas.", bot_id);
    lista_visitas_cmd.add_option(dpp::command_option(dpp::co_string, "status", "Filtro.", false).add_choice(dpp::command_option_choice("Agendadas", std::string("agendada"))).add_choice(dpp::command_option_choice("Realizadas", std::string("realizada"))).add_choice(dpp::command_option_choice("Canceladas", std::string("cancelada"))).add_choice(dpp::command_option_choice("Todas", std::string("todas"))));
    commands.push_back(lista_visitas_cmd);

    dpp::slashcommand modificar_visita_cmd("modificar_visita", "Altera dados de uma visita.", bot_id);
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "Código da visita.", true));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Novo nome do doutor.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Nova área.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "data", "Nova data.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "horario", "Novo horário.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Nova unidade.", false).add_choice(dpp::command_option_choice("Tatuapé", std::string("Tatuapé"))).add_choice(dpp::command_option_choice("Campo Belo", std::string("Campo Belo"))));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "telefone", "Novo telefone.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "Adicionar observação.", false));
    commands.push_back(modificar_visita_cmd);
}