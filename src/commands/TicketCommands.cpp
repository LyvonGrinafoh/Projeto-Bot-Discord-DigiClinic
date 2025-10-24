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

TicketCommands::TicketCommands(dpp::cluster& bot, TicketManager& tm, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), ticketManager_(tm), config_(config), cmdHandler_(handler) {
}

void TicketCommands::handle_chamar(const dpp::slashcommand_t& event) {
    dpp::user author = event.command.get_issuing_user();
    dpp::user target_user = event.command.get_resolved_user(std::get<dpp::snowflake>(event.get_parameter("usuario")));

    if (target_user.is_bot()) {
        event.reply(dpp::message("Você não pode chamar um bot para um ticket.").set_flags(dpp::m_ephemeral));
        return;
    }
    if (target_user.id == author.id) {
        event.reply(dpp::message("Você não pode abrir um ticket com você mesmo.").set_flags(dpp::m_ephemeral));
        return;
    }

    event.thinking(true);
    dpp::slashcommand_t event_copy = event;

    try {
        uint64_t temp_id = Utils::gerar_codigo();
        std::string channel_name_temp = "ticket-" + std::to_string(temp_id);

        dpp::channel ticket_channel;
        ticket_channel.set_name(channel_name_temp);
        ticket_channel.set_guild_id(event.command.guild_id);

        std::vector<dpp::permission_overwrite> overwrites;

        dpp::permission_overwrite everyone_deny;
        everyone_deny.id = event.command.guild_id;
        everyone_deny.type = dpp::ot_role;
        everyone_deny.allow = 0;
        everyone_deny.deny = dpp::p_view_channel;
        overwrites.push_back(everyone_deny);

        dpp::permission_overwrite author_allow;
        author_allow.id = author.id;
        author_allow.type = dpp::ot_member;
        author_allow.allow = dpp::p_view_channel | dpp::p_send_messages | dpp::p_attach_files | dpp::p_embed_links | dpp::p_read_message_history;
        author_allow.deny = 0;
        overwrites.push_back(author_allow);

        dpp::permission_overwrite target_allow;
        target_allow.id = target_user.id;
        target_allow.type = dpp::ot_member;
        target_allow.allow = dpp::p_view_channel | dpp::p_send_messages | dpp::p_attach_files | dpp::p_embed_links | dpp::p_read_message_history;
        target_allow.deny = 0;
        overwrites.push_back(target_allow);

        dpp::permission_overwrite bot_allow;
        bot_allow.id = bot_.me.id;
        bot_allow.type = dpp::ot_member;
        bot_allow.allow = dpp::p_view_channel | dpp::p_send_messages | dpp::p_embed_links | dpp::p_read_message_history | dpp::p_manage_messages | dpp::p_manage_channels;
        bot_allow.deny = 0;
        overwrites.push_back(bot_allow);

        ticket_channel.permission_overwrites = overwrites;

        bot_.channel_create(ticket_channel, [this, event_copy, author, target_user](const dpp::confirmation_callback_t& cb) {

            if (cb.is_error()) {
                Utils::log_to_file("ERRO: Falha ao criar canal do ticket (passo 1): " + cb.get_error().message);
                event_copy.edit_original_response(dpp::message("❌ Falha ao criar o canal do ticket."));
                return;
            }

            try {
                dpp::channel new_channel = std::get<dpp::channel>(cb.value);

                Ticket ticket_real = ticketManager_.createTicket(author.id, target_user.id, new_channel.id);

                dpp::channel canal_para_renomear = new_channel;
                canal_para_renomear.set_name("ticket-" + std::to_string(ticket_real.ticket_id));

                bot_.channel_edit(canal_para_renomear, [this, event_copy, ticket_real, new_channel, author, target_user](const dpp::confirmation_callback_t& edit_cb) {

                    if (edit_cb.is_error()) {
                        Utils::log_to_file("AVISO: Falha ao *renomear* o canal (ticket-" + std::to_string(ticket_real.ticket_id) + "): " + edit_cb.get_error().message);
                    }

                    dpp::embed embed;
                    embed.set_color(dpp::colors::green).set_title("✅ Ticket Criado: #" + std::to_string(ticket_real.ticket_id));
                    embed.set_description("Canal privado <#" + std::to_string(new_channel.id) + "> criado para a conversa entre " + author.get_mention() + " e " + target_user.get_mention() + ".\n\nUse `/finalizar_ticket " + std::to_string(ticket_real.ticket_id) + "` dentro daquele canal para encerrar.");

                    event_copy.edit_original_response(dpp::message().add_embed(embed));

                    dpp::embed ticket_embed;
                    ticket_embed.set_color(dpp::colors::sti_blue).set_title("Ticket #" + std::to_string(ticket_real.ticket_id) + " Iniciado");
                    ticket_embed.set_description(author.get_mention() + " iniciou uma conversa com " + target_user.get_mention() + ".\n\nAmbos podem usar `/finalizar_ticket " + std::to_string(ticket_real.ticket_id) + "` aqui para fechar e arquivar este ticket.");
                    bot_.message_create(dpp::message(new_channel.id, ticket_embed));
                    });

            }
            catch (const std::exception& e) {
                std::string erro_msg = e.what();
                Utils::log_to_file("ERRO CRITICO (STD) DENTRO DA LAMBDA: " + erro_msg);
                event_copy.edit_original_response(dpp::message("❌ Ocorreu um erro interno grave (lambda): " + erro_msg));
            }
            catch (...) {
                Utils::log_to_file("ERRO CRITICO (DESCONHECIDO) DENTRO DA LAMBDA");
                event_copy.edit_original_response(dpp::message("❌ Ocorreu um erro interno desconhecido (lambda)."));
            }
            });
    }
    catch (const std::exception& e) {
        std::string erro_msg = e.what();
        Utils::log_to_file("ERRO CRITICO (STD) em handle_chamar (try/catch): " + erro_msg);
        event_copy.edit_original_response(dpp::message("❌ Ocorreu um erro interno grave: " + erro_msg));
    }
    catch (...) {
        Utils::log_to_file("ERRO CRITICO (DESCONHECIDO) em handle_chamar (try/catch)");
        event_copy.edit_original_response(dpp::message("❌ Ocorreu um erro interno desconhecido."));
    }
}


void TicketCommands::handle_finalizar_ticket(const dpp::slashcommand_t& event) {
    int64_t ticket_id_int = std::get<int64_t>(event.get_parameter("codigo"));
    uint64_t ticket_id = static_cast<uint64_t>(ticket_id_int);
    dpp::user closer = event.command.get_issuing_user();

    std::optional<Ticket> ticket_opt = ticketManager_.findTicketById(ticket_id);

    if (!ticket_opt) {
        event.reply(dpp::message("❌ Código de ticket não encontrado.").set_flags(dpp::m_ephemeral));
        return;
    }

    Ticket ticket = *ticket_opt;

    if (event.command.channel_id != ticket.channel_id) {
        event.reply(dpp::message("❌ Este comando só pode ser usado dentro do canal do ticket correspondente.").set_flags(dpp::m_ephemeral));
        return;
    }

    if (closer.id != ticket.user_a_id && closer.id != ticket.user_b_id) {
        event.reply(dpp::message("❌ Apenas os participantes do ticket (" + dpp::user::get_mention(ticket.user_a_id) + " e " + dpp::user::get_mention(ticket.user_b_id) + ") podem fechá-lo.").set_flags(dpp::m_ephemeral));
        return;
    }

    cmdHandler_.replyAndDelete(event, dpp::message("Fechando e arquivando ticket... ⏳"));

    std::string log_content = ticketManager_.getAndRemoveLog(ticket.channel_id);
    std::string log_filename = "Ticket_" + std::to_string(ticket.ticket_id) + ".txt";

    try {
        std::ofstream log_file(log_filename);
        log_file << log_content;
        log_file.close();
    }
    catch (const std::exception& e) {
        Utils::log_to_file("ERRO: Falha ao salvar log do ticket " + std::to_string(ticket.ticket_id) + ": " + e.what());
    }

    bot_.channel_delete(ticket.channel_id, [this, ticket, closer, log_filename](const dpp::confirmation_callback_t& cb) {
        if (cb.is_error()) {
            Utils::log_to_file("ERRO: Falha ao deletar canal do ticket " + std::to_string(ticket.ticket_id) + ": " + cb.get_error().message);
            return;
        }

        ticketManager_.removeTicket(ticket.ticket_id);

        dpp::embed log_embed;
        log_embed.set_color(dpp::colors::red).set_title("Ticket #" + std::to_string(ticket.ticket_id) + " Fechado");
        log_embed.set_description("O ticket entre <@" + std::to_string(ticket.user_a_id) + "> e <@" + std::to_string(ticket.user_b_id) + "> foi fechado por " + closer.get_mention() + ".");
        log_embed.add_field("Log Salvo", log_filename, false);
        bot_.message_create(dpp::message(config_.canal_logs, log_embed));
        });
}

void TicketCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand chamar_cmd("chamar", "Inicia um ticket de conversa privada com um usuário.", bot_id);
    chamar_cmd.add_option(dpp::command_option(dpp::co_user, "usuario", "O usuário com quem você quer falar.", true));
    commands.push_back(chamar_cmd);

    dpp::slashcommand finalizar_cmd("finalizar_ticket", "Fecha e arquiva um ticket de conversa.", bot_id);
    finalizar_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O número do ticket a ser fechado.", true));
    commands.push_back(finalizar_cmd);
}