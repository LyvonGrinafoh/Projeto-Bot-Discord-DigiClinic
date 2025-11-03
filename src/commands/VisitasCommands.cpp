#include "VisitasCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "EventHandler.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <ctime>
#include <variant>
#include <cmath>

dpp::embed generatePageEmbed(const PaginationState& state);

VisitasCommands::VisitasCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler, EventHandler& eventHandler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler), eventHandler_(eventHandler) {
}

void VisitasCommands::handle_visitas(const dpp::slashcommand_t& event) {
    dpp::user quem_marcou = event.command.get_resolved_user(std::get<dpp::snowflake>(event.get_parameter("quem_marcou")));
    Visita nova_visita;
    nova_visita.id = Utils::gerar_codigo();
    nova_visita.quem_marcou_id = quem_marcou.id;
    nova_visita.quem_marcou_nome = quem_marcou.username;
    nova_visita.doutor = std::get<std::string>(event.get_parameter("doutor"));
    nova_visita.area = std::get<std::string>(event.get_parameter("area"));
    nova_visita.data = std::get<std::string>(event.get_parameter("data"));
    nova_visita.horario = std::get<std::string>(event.get_parameter("horario"));
    nova_visita.unidade = std::get<std::string>(event.get_parameter("unidade"));
    nova_visita.telefone = std::get<std::string>(event.get_parameter("telefone"));
    std::string timestamp_obs = Utils::format_timestamp(std::time(nullptr));
    nova_visita.observacoes = "[" + timestamp_obs + " | " + quem_marcou.username + "]: Visita agendada.";
    nova_visita.status = "agendada";

    auto obs_param = event.get_parameter("observacao");
    if (std::holds_alternative<std::string>(obs_param)) {
        nova_visita.observacoes += "\n  Observação inicial: " + std::get<std::string>(obs_param);
    }

    if (db_.addOrUpdateVisita(nova_visita)) {
        dpp::embed embed = dpp::embed().set_color(dpp::colors::blue).set_title("🔔 Nova Visita Agendada").add_field("Agendado por", quem_marcou.get_mention(), true).add_field("Nome do Dr(a).", nova_visita.doutor, true).add_field("Área", nova_visita.area, true).add_field("Data", nova_visita.data, true).add_field("Horário", nova_visita.horario, true).add_field("Unidade", nova_visita.unidade, true).add_field("Telefone", nova_visita.telefone, false);
        if (std::holds_alternative<std::string>(obs_param)) {
            embed.add_field("Observação Inicial", std::get<std::string>(obs_param), false);
        }
        embed.set_footer(dpp::embed_footer().set_text("Código da Visita: " + std::to_string(nova_visita.id)));
        dpp::message msg_canal(config_.canal_visitas, embed);
        bot_.message_create(msg_canal);
        cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita agendada e salva com sucesso! Código: `" + std::to_string(nova_visita.id) + "`"));
    }
    else {
        event.reply(dpp::message("❌ Erro ao salvar a visita no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void VisitasCommands::handle_cancelar_visita(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo"));
    uint64_t codigo = static_cast<uint64_t>(codigo_int);
    std::string motivo = std::get<std::string>(event.get_parameter("motivo"));
    dpp::user cancelador = event.command.get_issuing_user();

    Visita* visita_ptr = db_.getVisitaPtr(codigo);

    if (visita_ptr) {
        Visita& visita = *visita_ptr;

        if (visita.status != "agendada") {
            event.reply(dpp::message("⚠️ Esta visita não pode ser cancelada (Status: " + visita.status + ").").set_flags(dpp::m_ephemeral));
            return;
        }

        std::string timestamp_obs = Utils::format_timestamp(std::time(nullptr));
        std::string nota_cancelamento = "\n\n[" + timestamp_obs + " | " + cancelador.username + "]: Visita Cancelada - Motivo: " + motivo;

        visita.observacoes += nota_cancelamento;
        visita.status = "cancelada";

        if (db_.addOrUpdateVisita(visita)) {
            dpp::embed embed = dpp::embed().set_color(dpp::colors::red).set_title("❌ Visita Cancelada").add_field("Dr(a).", visita.doutor, true).add_field("Data", visita.data + " às " + visita.horario, true).add_field("Motivo", motivo, false).set_footer(dpp::embed_footer().set_text("Cancelamento registrado por: " + cancelador.username + " | Código: " + std::to_string(codigo)));
            bot_.message_create(dpp::message(config_.canal_visitas, embed));
            cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita `" + std::to_string(codigo) + "` marcada como cancelada com sucesso!"));
        }
        else {
            event.reply(dpp::message("❌ Erro ao salvar as alterações da visita.").set_flags(dpp::m_ephemeral));
        }
    }
    else {
        event.reply(dpp::message("❌ Código de visita não encontrado.").set_flags(dpp::m_ephemeral));
    }
}

void VisitasCommands::handle_finalizar_visita(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo"));
    uint64_t codigo = static_cast<uint64_t>(codigo_int);
    std::string relatorio = std::get<std::string>(event.get_parameter("relatorio"));
    dpp::user finalizador = event.command.get_issuing_user();

    Visita* visita_ptr = db_.getVisitaPtr(codigo);

    if (visita_ptr) {
        Visita& visita = *visita_ptr;

        if (visita.status != "agendada") {
            event.reply(dpp::message("⚠️ Esta visita não está agendada (Status: " + visita.status + ").").set_flags(dpp::m_ephemeral));
            return;
        }

        std::string timestamp_obs = Utils::format_timestamp(std::time(nullptr));
        std::string nota_final = "\n\n[" + timestamp_obs + " | " + finalizador.username + "]: Visita Finalizada.";
        nota_final += "\n  Relatório: " + relatorio;

        visita.observacoes += nota_final;
        visita.status = "finalizada";
        visita.relatorio_visita = relatorio;

        if (db_.addOrUpdateVisita(visita)) {
            dpp::embed embed = dpp::embed().set_color(dpp::colors::dark_green).set_title("✅ Visita Finalizada")
                .add_field("Dr(a).", visita.doutor, true)
                .add_field("Data", visita.data + " às " + visita.horario, true)
                .add_field("Relatório", relatorio, false)
                .set_footer(dpp::embed_footer().set_text("Finalizado por: " + finalizador.username + " | Código: " + std::to_string(codigo)));

            bot_.message_create(dpp::message(config_.canal_visitas, embed));
            cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita `" + std::to_string(codigo) + "` finalizada com sucesso!"));
        }
        else {
            event.reply(dpp::message("❌ Erro ao salvar as alterações da visita.").set_flags(dpp::m_ephemeral));
        }
    }
    else {
        event.reply(dpp::message("❌ Código de visita não encontrado.").set_flags(dpp::m_ephemeral));
    }
}


void VisitasCommands::handle_lista_visitas(const dpp::slashcommand_t& event) {
    std::string status_filtro = "agendada";
    auto param = event.get_parameter("status");
    if (auto val_ptr = std::get_if<std::string>(&param)) {
        status_filtro = *val_ptr;
    }

    const auto& visitas_map = db_.getVisitas();
    std::vector<PaginatedItem> filtered_items;

    for (const auto& [id, visita] : visitas_map) {
        if (status_filtro == "todas") {
            filtered_items.push_back(visita);
        }
        else if (visita.status == status_filtro) {
            filtered_items.push_back(visita);
        }
    }

    if (filtered_items.empty()) {
        event.reply(dpp::message("Nenhuma visita encontrada com o status `" + status_filtro + "`.").set_flags(dpp::m_ephemeral));
        return;
    }

    PaginationState state;
    state.channel_id = event.command.channel_id;
    state.items = std::move(filtered_items);
    state.originalUserID = event.command.get_issuing_user().id;
    state.listType = "visitas";
    state.currentPage = 1;
    state.itemsPerPage = 5;

    dpp::embed firstPageEmbed = generatePageEmbed(state);
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
                                bot_.message_add_reaction(msg.id, msg.channel_id, "◀️", [this, msg](const dpp::confirmation_callback_t& reaction_cb) {
                                    if (!reaction_cb.is_error()) {
                                        bot_.message_add_reaction(msg.id, msg.channel_id, "▶️");
                                    }
                                    });
                            }
                            else {
                                Utils::log_to_file("Erro: get_original_response não retornou um dpp::message.");
                            }
                        }
                        else {
                            Utils::log_to_file("Erro ao *buscar* a resposta original: " + msg_cb.get_error().message);
                        }
                        });
                }
            }
            else {
                Utils::log_to_file("Erro ao *enviar* a resposta inicial paginada para /lista_visitas: " + cb.get_error().message);
            }
        });
}

void VisitasCommands::handle_modificar_visita(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo")); uint64_t codigo = static_cast<uint64_t>(codigo_int); std::string motivo = std::get<std::string>(event.get_parameter("motivo")); dpp::user modificador = event.command.get_issuing_user();

    Visita* visita_ptr = db_.getVisitaPtr(codigo);

    if (visita_ptr) {
        Visita& visita = *visita_ptr;

        if (visita.status != "agendada") {
            event.reply(dpp::message("⚠️ Esta visita não pode ser modificada (Status: " + visita.status + ").").set_flags(dpp::m_ephemeral));
            return;
        }

        std::string timestamp_obs = Utils::format_timestamp(std::time(nullptr)); std::string log_modificacao = "\n\n[" + timestamp_obs + " | " + modificador.username + "]: Modificação - " + motivo + "."; bool modificado = false;

        auto check_and_update = [&](const std::string& param_name, std::string& field_to_update, const std::string& field_label) { auto param = event.get_parameter(param_name); if (auto val_ptr = std::get_if<std::string>(&param)) { if (*val_ptr != field_to_update) { log_modificacao += "\n  - " + field_label + " alterado de '" + field_to_update + "' para '" + *val_ptr + "'."; field_to_update = *val_ptr; return true; } } return false; };

        modificado |= check_and_update("doutor", visita.doutor, "Doutor"); modificado |= check_and_update("area", visita.area, "Área"); modificado |= check_and_update("data", visita.data, "Data"); modificado |= check_and_update("horario", visita.horario, "Horário"); modificado |= check_and_update("unidade", visita.unidade, "Unidade"); modificado |= check_and_update("telefone", visita.telefone, "Telefone");

        auto nova_obs_param = event.get_parameter("nova_observacao");
        if (auto val_ptr = std::get_if<std::string>(&nova_obs_param)) {
            log_modificacao += "\n  - Nova observação adicionada: " + *val_ptr;
            modificado = true;
        }

        if (modificado) {
            visita.observacoes += log_modificacao;
            if (db_.addOrUpdateVisita(visita)) { 
                cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita `" + std::to_string(codigo) + "` modificada com sucesso!"));
            }
            else {
                event.reply(dpp::message("❌ Erro ao salvar as alterações da visita.").set_flags(dpp::m_ephemeral));
            }
        }
        else {
            event.reply(dpp::message("⚠️ Nenhuma alteração foi especificada (ou os valores são iguais aos existentes) para a visita `" + std::to_string(codigo) + "`.").set_flags(dpp::m_ephemeral));
        }
    }
    else {
        event.reply(dpp::message("❌ Código de visita não encontrado.").set_flags(dpp::m_ephemeral));
    }
}

void VisitasCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    // --- COMANDO /visitas ---
    dpp::slashcommand visitas_cmd("visitas", "Agenda uma nova visita.", bot_id);
    visitas_cmd.add_option(dpp::command_option(dpp::co_user, "quem_marcou", "Quem está agendando a visita.", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome do Dr(a).", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Área de atuação do Dr(a).", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "data", "Data da visita (ex: 10/10/2025).", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "horario", "Horário da visita (ex: 14h).", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade da visita.", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "telefone", "Telefone de contato.", true));
    visitas_cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "Observação inicial (opcional).", false));
    commands.push_back(visitas_cmd);

    // --- COMANDO /cancelar_visita ---
    dpp::slashcommand cancelar_visita_cmd("cancelar_visita", "Cancela uma visita agendada, registrando o motivo.", bot_id);
    cancelar_visita_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código da visita a ser cancelada.", true));
    cancelar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "motivo", "O motivo do cancelamento.", true));
    commands.push_back(cancelar_visita_cmd);

    // --- NOVO COMANDO /finalizar_visita ---
    dpp::slashcommand finalizar_visita_cmd("finalizar_visita", "Finaliza uma visita e registra um relatório.", bot_id);
    finalizar_visita_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código da visita a ser finalizada.", true));
    finalizar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "relatorio", "Relatório/observação da visita.", true));
    commands.push_back(finalizar_visita_cmd);


    // --- COMANDO /lista_visitas (ATUALIZADO) ---
    dpp::slashcommand lista_visitas_cmd("lista_visitas", "Mostra uma lista de visitas.", bot_id);
    lista_visitas_cmd.add_option(
        dpp::command_option(dpp::co_string, "status", "Filtrar por status (padrão: agendada).", false)
        .add_choice(dpp::command_option_choice("Agendadas", std::string("agendada")))
        .add_choice(dpp::command_option_choice("Finalizadas", std::string("finalizada")))
        .add_choice(dpp::command_option_choice("Canceladas", std::string("cancelada")))
        .add_choice(dpp::command_option_choice("Todas", std::string("todas")))
    );
    commands.push_back(lista_visitas_cmd);

    // --- COMANDO /modificar_visita ---
    dpp::slashcommand modificar_visita_cmd("modificar_visita", "Modifica uma visita agendada.", bot_id);
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código da visita a ser modificada.", true));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "motivo", "Motivo da modificação (registrado no histórico).", true));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Novo nome do Dr(a).", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Nova área de atuação.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "data", "Nova data da visita.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "horario", "Novo horário da visita.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Nova unidade da visita.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "telefone", "Novo telefone de contato.", false));
    modificar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "nova_observacao", "Adicionar uma nova observação ao histórico.", false));
    commands.push_back(modificar_visita_cmd);
}