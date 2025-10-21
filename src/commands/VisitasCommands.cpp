#include "VisitasCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <ctime>
#include <variant>

VisitasCommands::VisitasCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler) {
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
    auto obs_param = event.get_parameter("observacao");
    if (std::holds_alternative<std::string>(obs_param)) {
        nova_visita.observacoes += "\n  Observação inicial: " + std::get<std::string>(obs_param);
    }

    if (db_.addOrUpdateVisita(nova_visita)) {
        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::blue)
            .set_title("🔔 Nova Visita Agendada")
            .add_field("Agendado por", quem_marcou.get_mention(), true)
            .add_field("Nome do Dr(a).", nova_visita.doutor, true)
            .add_field("Área", nova_visita.area, true)
            .add_field("Data", nova_visita.data, true)
            .add_field("Horário", nova_visita.horario, true)
            .add_field("Unidade", nova_visita.unidade, true)
            .add_field("Telefone", nova_visita.telefone, false);

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
        std::string timestamp_obs = Utils::format_timestamp(std::time(nullptr));
        std::string nota_cancelamento = "\n\n[" + timestamp_obs + " | " + cancelador.username + "]: Visita Cancelada - Motivo: " + motivo;

        if (visita.observacoes.find("Visita Cancelada") == std::string::npos) {
            visita.observacoes += nota_cancelamento;
            if (db_.saveVisitas()) {
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::red)
                    .set_title("❌ Visita Cancelada Registrada")
                    .add_field("Dr(a).", visita.doutor, true)
                    .add_field("Data", visita.data + " às " + visita.horario, true)
                    .add_field("Motivo", motivo, false)
                    .set_footer(dpp::embed_footer().set_text("Cancelamento registrado por: " + cancelador.username + " | Código: " + std::to_string(codigo)));
                bot_.message_create(dpp::message(config_.canal_visitas, embed));
                cmdHandler_.replyAndDelete(event, dpp::message("✅ Visita `" + std::to_string(codigo) + "` marcada como cancelada com sucesso!"));
            }
            else {
                event.reply(dpp::message("❌ Erro ao salvar as alterações da visita.").set_flags(dpp::m_ephemeral));
            }
        }
        else {
            event.reply(dpp::message("⚠️ Esta visita já consta como cancelada no histórico.").set_flags(dpp::m_ephemeral));
        }
    }
    else {
        event.reply(dpp::message("❌ Código de visita não encontrado.").set_flags(dpp::m_ephemeral));
    }
}

void VisitasCommands::handle_lista_visitas(const dpp::slashcommand_t& event) {
    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::blue)
        .set_title("📅 Próximas Visitas Agendadas");
    std::string description; int count = 0; int canceladas_count = 0;
    const auto& visitas_map = db_.getVisitas(); std::vector<Visita> sorted_visitas;
    for (const auto& pair : visitas_map) { sorted_visitas.push_back(pair.second); }

    if (sorted_visitas.empty()) {
        description = "Nenhuma visita agendada no momento.";
    }
    else {
        for (const auto& visita : sorted_visitas) {
            bool cancelada = (visita.observacoes.find("Visita Cancelada") != std::string::npos);
            if (cancelada) { description += "**[CANCELADA]** "; canceladas_count++; }
            description += "**Dr(a):** " + visita.doutor + "\n";
            description += "**Código:** `" + std::to_string(visita.id) + "`\n";
            description += "**Data:** " + visita.data + " às " + visita.horario + "\n";
            description += "**Unidade:** " + visita.unidade + "\n";
            description += "**Agendado por:** " + visita.quem_marcou_nome + "\n";
            description += "--------------------\n"; count++;
        }
        embed.set_title(embed.title + " (" + std::to_string(count - canceladas_count) + " ativas, " + std::to_string(canceladas_count) + " canceladas)");
    }
    if (description.length() > 4000) { description = description.substr(0, 4000) + "\n... (lista muito longa para exibir)"; }
    embed.set_description(description);
    event.reply(dpp::message().add_embed(embed));
}

void VisitasCommands::handle_modificar_visita(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo")); uint64_t codigo = static_cast<uint64_t>(codigo_int); std::string motivo = std::get<std::string>(event.get_parameter("motivo")); dpp::user modificador = event.command.get_issuing_user();
    Visita* visita_ptr = db_.getVisitaPtr(codigo);

    if (visita_ptr) {
        Visita& visita = *visita_ptr; std::string timestamp_obs = Utils::format_timestamp(std::time(nullptr)); std::string log_modificacao = "\n\n[" + timestamp_obs + " | " + modificador.username + "]: Modificação - " + motivo + "."; bool modificado = false;
        auto check_and_update = [&](const std::string& param_name, std::string& field_to_update, const std::string& field_label) { auto param = event.get_parameter(param_name); if (auto val_ptr = std::get_if<std::string>(&param)) { if (*val_ptr != field_to_update) { log_modificacao += "\n  - " + field_label + " alterado de '" + field_to_update + "' para '" + *val_ptr + "'."; field_to_update = *val_ptr; return true; } } return false; };

        modificado |= check_and_update("doutor", visita.doutor, "Doutor"); modificado |= check_and_update("area", visita.area, "Área"); modificado |= check_and_update("data", visita.data, "Data"); modificado |= check_and_update("horario", visita.horario, "Horário"); modificado |= check_and_update("unidade", visita.unidade, "Unidade"); modificado |= check_and_update("telefone", visita.telefone, "Telefone");
        auto nova_obs_param = event.get_parameter("nova_observacao");
        if (auto val_ptr = std::get_if<std::string>(&nova_obs_param)) { log_modificacao += "\n  - Nova observação adicionada: " + *val_ptr; modificado = true; }

        if (modificado) {
            visita.observacoes += log_modificacao;
            if (db_.saveVisitas()) {
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
        event.reply(dpp::message("❌ Código da visita não encontrado.").set_flags(dpp::m_ephemeral));
    }
}

void VisitasCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
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

    dpp::slashcommand cancelar_visita_cmd("cancelar_visita", "Cancela uma visita agendada, registrando o motivo.", bot_id);
    cancelar_visita_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código da visita a ser cancelada.", true));
    cancelar_visita_cmd.add_option(dpp::command_option(dpp::co_string, "motivo", "O motivo do cancelamento.", true));
    commands.push_back(cancelar_visita_cmd);

    commands.push_back(dpp::slashcommand("lista_visitas", "Mostra as próximas visitas agendadas.", bot_id));

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