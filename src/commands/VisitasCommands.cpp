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
        cmdHandler_.replyAndDelete(event, dpp::message("❌ Formato inválido. Use DD/MM/AAAA."));
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
    if (std::holds_alternative<std::string>(tel_param)) nova_visita.telefone = std::get<std::string>(tel_param);
    else nova_visita.telefone = "Não informado";

    auto obs_param = event.get_parameter("observacao");
    if (std::holds_alternative<std::string>(obs_param)) nova_visita.observacoes = std::get<std::string>(obs_param);
    else nova_visita.observacoes = "Nenhuma";

    if (db_.addOrUpdateVisita(nova_visita)) {
        dpp::embed embed = Utils::criarEmbedPadrao("📅 Nova Visita Agendada", "", dpp::colors::blue);
        embed.add_field("Doutor(a)", nova_visita.doutor, true)
            .add_field("Data/Hora", nova_visita.data + " - " + nova_visita.horario, true)
            .add_field("Unidade", nova_visita.unidade, true)
            .add_field("Área", nova_visita.area, true)
            .add_field("Telefone", nova_visita.telefone, true)
            .add_field("Obs", nova_visita.observacoes, false)
            .set_footer(dpp::embed_footer().set_text("Cód: " + std::to_string(nova_visita.id)));

        bot_.message_create(dpp::message(config_.canal_visitas, embed));
        cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita agendada! Código: `" + std::to_string(nova_visita.id) + "`"));
    }
    else {
        cmdHandler_.replyAndDelete(event, dpp::message("❌ Erro ao salvar."));
    }
}

void VisitasCommands::handle_cancelar_visita(const dpp::slashcommand_t& event) {
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    std::string motivo = std::get<std::string>(event.get_parameter("motivo"));

    std::optional<Visita> visita_opt = db_.getVisita(codigo);
    if (visita_opt) {
        Visita visita = *visita_opt;
        if (visita.status == "cancelada") {
            cmdHandler_.replyAndDelete(event, dpp::message("❌ Já está cancelada."));
            return;
        }
        visita.status = "cancelada";
        visita.observacoes += " [CANCELADA: " + motivo + "]";

        if (db_.addOrUpdateVisita(visita)) {
            dpp::embed embed = Utils::criarEmbedPadrao("❌ Visita Cancelada", "", dpp::colors::red);
            embed.add_field("Doutor(a)", visita.doutor, true)
                .add_field("Data Original", visita.data, true)
                .add_field("Motivo", motivo, false)
                .set_footer(dpp::embed_footer().set_text("Cancelado por: " + event.command.get_issuing_user().username));

            bot_.message_create(dpp::message(config_.canal_visitas, embed));
            cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita cancelada."));
        }
        else {
            cmdHandler_.replyAndDelete(event, dpp::message("❌ Erro ao atualizar."));
        }
    }
    else {
        cmdHandler_.replyAndDelete(event, dpp::message("❌ Código não encontrado."));
    }
}

void VisitasCommands::handle_finalizar_visita(const dpp::slashcommand_t& event) {
    event.thinking(false);
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    std::string relatorio = std::get<std::string>(event.get_parameter("relatorio"));

    std::optional<Visita> visita_opt = db_.getVisita(codigo);
    if (!visita_opt) { cmdHandler_.editAndDelete(event, dpp::message("❌ Código não encontrado.")); return; }
    Visita visita = *visita_opt;

    if (visita.status != "agendada") { cmdHandler_.editAndDelete(event, dpp::message("❌ Visita não está agendada.")); return; }

    visita.status = "realizada";
    visita.relatorio_visita = relatorio;

    auto finish = [this, event, visita, relatorio](const std::string& filename, const std::string& content) mutable {
        if (db_.addOrUpdateVisita(visita)) {
            dpp::embed embed = Utils::criarEmbedPadrao("✅ Visita Realizada", relatorio, dpp::colors::green);
            embed.add_field("Doutor(a)", visita.doutor, true)
                .add_field("Data", visita.data, true)
                .set_footer(dpp::embed_footer().set_text("Finalizado por: " + event.command.get_issuing_user().username));

            dpp::message msg(config_.canal_visitas, embed);
            if (!filename.empty() && !content.empty()) {
                msg.add_file(filename, content);
                embed.set_image("attachment://" + filename);
            }
            msg.add_embed(embed);
            bot_.message_create(msg);

            cmdHandler_.editAndDelete(event, dpp::message("✅ Visita marcada como realizada!"));
        }
        else {
            cmdHandler_.editAndDelete(event, dpp::message("❌ Erro ao atualizar."));
        }
        };

    auto param_ficha = event.get_parameter("ficha");
    if (std::holds_alternative<dpp::snowflake>(param_ficha)) {
        dpp::attachment anexo = event.command.get_resolved_attachment(std::get<dpp::snowflake>(param_ficha));
        std::string filename = "visita_" + std::to_string(visita.id) + "_" + anexo.filename;
        std::string path = "./uploads/" + filename;

        Utils::BaixarAnexo(&bot_, anexo.url, path, [this, event, visita, path, filename, finish](bool ok, std::string content) mutable {
            if (ok) { visita.ficha_path = path; finish(filename, content); }
            else { cmdHandler_.editAndDelete(event, dpp::message("Erro ao baixar ficha.")); }
            });
    }
    else { finish("", ""); }
}

void VisitasCommands::handle_lista_visitas(const dpp::slashcommand_t& event) {
    std::string status_filtro = "agendada";
    auto param = event.get_parameter("status");
    if (auto val_ptr = std::get_if<std::string>(&param)) status_filtro = *val_ptr;

    const auto& visitas_map = db_.getVisitas();
    std::vector<PaginatedItem> filtered_items;

    for (const auto& [id, visita] : visitas_map) {
        if (status_filtro == "todas") filtered_items.push_back(visita);
        else if (visita.status == status_filtro) filtered_items.push_back(visita);
    }
    std::sort(filtered_items.begin(), filtered_items.end(), [](const auto& a, const auto& b) { return std::get<Visita>(a).id < std::get<Visita>(b).id; });

    if (filtered_items.empty()) { cmdHandler_.replyAndDelete(event, dpp::message("Nenhuma visita encontrada.")); return; }

    PaginationState state;
    state.channel_id = event.command.channel_id;
    state.items = std::move(filtered_items);
    state.originalUserID = event.command.get_issuing_user().id;
    state.listType = "visitas";
    state.currentPage = 1;
    state.itemsPerPage = 5;

    dpp::embed firstPageEmbed = Utils::generatePageEmbed(state);

    event.reply(dpp::message(event.command.channel_id, firstPageEmbed), [this, state, event](const auto& cb) {
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

                    bot_.start_timer([this, m](auto t) { bot_.message_delete(m.id, m.channel_id); bot_.stop_timer(t); }, 60);
                }
                });
        }
        });
}

void VisitasCommands::handle_modificar_visita(const dpp::slashcommand_t& event) {
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    Visita* visita_ptr = db_.getVisitaPtr(codigo);

    if (visita_ptr) {
        Visita& visita = *visita_ptr;
        bool alterado = false;

        auto check = [&](const std::string& p, std::string& f) {
            auto par = event.get_parameter(p);
            if (auto v = std::get_if<std::string>(&par)) { if (*v != f) { f = *v; return true; } }
            return false;
            };

        alterado |= check("doutor", visita.doutor);
        alterado |= check("horario", visita.horario);
        alterado |= check("unidade", visita.unidade);
        alterado |= check("area", visita.area);
        alterado |= check("telefone", visita.telefone);

        auto data_p = event.get_parameter("data");
        if (auto v = std::get_if<std::string>(&data_p)) {
            if (Utils::validarFormatoData(*v)) { visita.data = *v; alterado = true; }
        }

        auto obs_p = event.get_parameter("observacao");
        if (auto v = std::get_if<std::string>(&obs_p)) {
            if (!visita.observacoes.empty()) visita.observacoes += " | ";
            visita.observacoes += *v; alterado = true;
        }

        if (alterado) {
            db_.saveVisitas();
            cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita atualizada!"));
        }
        else cmdHandler_.replyAndDelete(event, dpp::message("⚠️ Sem alterações."));
    }
    else cmdHandler_.replyAndDelete(event, dpp::message("❌ Código não encontrado."));
}

void VisitasCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand v("visitas", "Agenda visita.", bot_id);
    v.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome.", true));
    v.add_option(dpp::command_option(dpp::co_string, "area", "Área.", true));
    v.add_option(dpp::command_option(dpp::co_string, "data", "DD/MM/AAAA", true));
    v.add_option(dpp::command_option(dpp::co_string, "horario", "HH:MM", true));
    v.add_option(dpp::command_option(dpp::co_string, "unidade", "Local.", true).add_choice(dpp::command_option_choice("Tatuapé", "Tatuapé")).add_choice(dpp::command_option_choice("Campo Belo", "Campo Belo")));
    v.add_option(dpp::command_option(dpp::co_string, "telefone", "Tel.", false));
    v.add_option(dpp::command_option(dpp::co_string, "observacao", "Obs.", false));
    commands.push_back(v);

    dpp::slashcommand c("cancelar_visita", "Cancela visita.", bot_id);
    c.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID.", true));
    c.add_option(dpp::command_option(dpp::co_string, "motivo", "Motivo.", true));
    commands.push_back(c);

    dpp::slashcommand f("finalizar_visita", "Realiza visita.", bot_id);
    f.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID.", true));
    f.add_option(dpp::command_option(dpp::co_string, "relatorio", "Resumo.", true));
    f.add_option(dpp::command_option(dpp::co_attachment, "ficha", "Foto.", false));
    commands.push_back(f);

    dpp::slashcommand l("lista_visitas", "Lista visitas.", bot_id);
    l.add_option(dpp::command_option(dpp::co_string, "status", "Filtro.", false).add_choice(dpp::command_option_choice("Agendadas", "agendada")).add_choice(dpp::command_option_choice("Realizadas", "realizada")).add_choice(dpp::command_option_choice("Canceladas", "cancelada")).add_choice(dpp::command_option_choice("Todas", "todas")));
    commands.push_back(l);

    dpp::slashcommand m("modificar_visita", "Edita visita.", bot_id);
    m.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID.", true));
    m.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome.", false));
    m.add_option(dpp::command_option(dpp::co_string, "area", "Area.", false));
    m.add_option(dpp::command_option(dpp::co_string, "data", "Data.", false));
    m.add_option(dpp::command_option(dpp::co_string, "horario", "Hora.", false));
    m.add_option(dpp::command_option(dpp::co_string, "unidade", "Local.", false).add_choice(dpp::command_option_choice("Tatuapé", "Tatuapé")).add_choice(dpp::command_option_choice("Campo Belo", "Campo Belo")));
    m.add_option(dpp::command_option(dpp::co_string, "telefone", "Tel.", false));
    m.add_option(dpp::command_option(dpp::co_string, "observacao", "Obs.", false));
    commands.push_back(m);
}