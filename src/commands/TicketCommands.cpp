#include "TicketCommands.h"
#include "TicketManager.h"
#include "CommandHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <filesystem>
#include <algorithm>

TicketCommands::TicketCommands(dpp::cluster& bot, TicketManager& tm, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), ticketManager_(tm), config_(config), cmdHandler_(handler) {
}

void TicketCommands::handle_chamar(const dpp::slashcommand_t& event) {
    dpp::user author = event.command.get_issuing_user();
    dpp::user target_user = event.command.get_resolved_user(std::get<dpp::snowflake>(event.get_parameter("usuario")));
    std::string assunto = std::get<std::string>(event.get_parameter("assunto"));

    if (target_user.is_bot() || target_user.id == author.id) {
        event.reply(dpp::message("Inválido.").set_flags(dpp::m_ephemeral));
        return;
    }

    event.thinking(true);
    dpp::slashcommand_t event_copy = event;

    dpp::channel ticket_channel;
    ticket_channel.set_name("ticket-loading");
    ticket_channel.set_guild_id(event.command.guild_id);

    std::vector<dpp::permission_overwrite> overwrites;
    overwrites.push_back({ event.command.guild_id, 0, dpp::p_view_channel, dpp::ot_role });
    overwrites.push_back({ author.id, dpp::p_view_channel | dpp::p_send_messages | dpp::p_attach_files, 0, dpp::ot_member });
    overwrites.push_back({ target_user.id, dpp::p_view_channel | dpp::p_send_messages | dpp::p_attach_files, 0, dpp::ot_member });
    overwrites.push_back({ bot_.me.id, dpp::p_view_channel | dpp::p_send_messages | dpp::p_manage_channels, 0, dpp::ot_member });
    ticket_channel.permission_overwrites = overwrites;

    bot_.channel_create(ticket_channel, [this, event_copy, author, target_user, assunto](const dpp::confirmation_callback_t& cb) {
        if (cb.is_error()) { event_copy.edit_original_response(dpp::message("Erro ao criar canal.")); return; }

        dpp::channel new_channel = std::get<dpp::channel>(cb.value);
        Ticket ticket_real = ticketManager_.createTicket(author.id, target_user.id, new_channel.id, assunto);

        new_channel.set_name("ticket-" + std::to_string(ticket_real.ticket_id));
        bot_.channel_edit(new_channel, [this, event_copy, new_channel, author, target_user, assunto, ticket_real](auto cb2) {
            dpp::embed embed = dpp::embed().set_color(dpp::colors::green).set_title("✅ Ticket Criado: " + assunto);
            embed.set_description("Canal: <#" + std::to_string(new_channel.id) + ">");
            event_copy.edit_original_response(dpp::message().add_embed(embed));

            bot_.message_create(dpp::message(new_channel.id, dpp::embed().set_title("Assunto: " + assunto).set_description("Ticket entre " + author.get_mention() + " e " + target_user.get_mention())));

            bot_.message_create(dpp::message(event_copy.command.channel_id, target_user.get_mention() + ", novo ticket de " + author.get_mention() + ": " + assunto));
            bot_.direct_message_create(target_user.id, dpp::message("Novo ticket criado com você. Assunto: " + assunto + "\nCanal: <#" + std::to_string(new_channel.id) + ">"));
            });
        });
}

void TicketCommands::handle_finalizar_ticket(const dpp::slashcommand_t& event) {
    dpp::user closer = event.command.get_issuing_user();
    auto ticket_opt = ticketManager_.findTicketByChannel(event.command.channel_id);

    if (!ticket_opt) { event.reply(dpp::message("Não é um canal de ticket.").set_flags(dpp::m_ephemeral)); return; }
    Ticket t = *ticket_opt;

    cmdHandler_.replyAndDelete(event, dpp::message("Fechando..."));

    std::string log = ticketManager_.getAndRemoveLog(t.channel_id);
    std::string filename = "Ticket_" + std::to_string(t.ticket_id) + ".txt";
    std::ofstream(filename) << "Assunto: " << t.nome_ticket << "\n\n" << log;

    bot_.channel_delete(t.channel_id, [this, t, closer, filename](auto cb) {
        ticketManager_.arquivarTicket(t.ticket_id, filename);
        dpp::embed embed = dpp::embed().set_color(dpp::colors::red).set_title("Ticket Fechado: " + t.nome_ticket);
        embed.set_description("Fechado por " + closer.get_mention());
        bot_.message_create(dpp::message(config_.canal_logs, embed));
        });
}

void TicketCommands::handle_ver_log(const dpp::slashcommand_t& event) {
    int64_t id = std::get<int64_t>(event.get_parameter("codigo"));
    auto t = ticketManager_.findTicketById(id);
    if (t && std::filesystem::exists(t->log_filename)) {
        dpp::message msg; msg.add_file(t->log_filename, dpp::utility::read_file(t->log_filename));
        event.reply(msg);
    }
    else {
        event.reply(dpp::message("Log não encontrado."));
    }
}

void TicketCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id, const BotConfig& config) {
    dpp::slashcommand chamar("chamar", "Abre ticket.", bot_id);
    chamar.add_option(dpp::command_option(dpp::co_user, "usuario", "Usuário.", true));
    chamar.add_option(dpp::command_option(dpp::co_string, "assunto", "Motivo do ticket.", true));
    commands.push_back(chamar);

    dpp::slashcommand finalizar("finalizar_ticket", "Fecha ticket.", bot_id);
    commands.push_back(finalizar);

    dpp::slashcommand log("ver_log", "Vê logs antigos.", bot_id);
    log.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID.", true));
    commands.push_back(log);
}