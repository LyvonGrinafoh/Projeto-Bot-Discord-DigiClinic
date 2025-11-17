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
                    embed.set_description("Canal privado <#" + std::to_string(new_channel.id) + "> criado para a conversa entre " + author.get_mention() + " e " + target_user.get_mention() + ".\n\nUse `/finalizar_ticket` dentro daquele canal para encerrar.");

                    event_copy.edit_original_response(dpp::message().add_embed(embed));

                    dpp::embed ticket_embed;
                    ticket_embed.set_color(dpp::colors::sti_blue).set_title("Ticket #" + std::to_string(ticket_real.ticket_id) + " Iniciado");
                    ticket_embed.set_description(author.get_mention() + " iniciou uma conversa com " + target_user.get_mention() + ".\n\nAmbos podem usar `/finalizar_ticket` aqui para fechar e arquivar este ticket.");
                    bot_.message_create(dpp::message(new_channel.id, ticket_embed));

                    // --- INÍCIO DA NOVA LÓGICA DE NOTIFICAÇÃO ---

                    // 1. Notificar no canal principal (temporário)
                    dpp::message public_msg(event_copy.command.channel_id,
                        target_user.get_mention() + ", " + author.get_mention() + " está te chamando no ticket <#" + std::to_string(new_channel.id) + ">."
                    );

                    bot_.message_create(public_msg, [this](const dpp::confirmation_callback_t& msg_cb) {
                        if (!msg_cb.is_error()) {
                            auto* msg_ptr = std::get_if<dpp::message>(&msg_cb.value);
                            if (msg_ptr) {
                                dpp::message msg_criada = *msg_ptr;
                                // Inicia timer de 10 minutos (600 segundos) para deletar a mensagem
                                bot_.start_timer([this, msg_criada](dpp::timer timer_handle) {
                                    bot_.message_delete(msg_criada.id, msg_criada.channel_id);
                                    bot_.stop_timer(timer_handle);
                                    }, 600);
                            }
                        }
                        });

                    // 2. Notificar no privado (DM)
                    dpp::message dm_msg("Olá " + target_user.username + "! " + author.get_mention() + " abriu um ticket privado com você aqui no servidor.\n\nPor favor, vá para o canal <#" + std::to_string(new_channel.id) + "> para conversar.");
                    bot_.direct_message_create(target_user.id, dm_msg, [this, target_user](const dpp::confirmation_callback_t& dm_cb) {
                        if (dm_cb.is_error()) {
                            Utils::log_to_file("AVISO: Falha ao enviar DM de notificação de ticket para " + target_user.username + ": " + dm_cb.get_error().message);
                        }
                        });

                    // --- FIM DA NOVA LÓGICA DE NOTIFICAÇÃO ---

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
    dpp::user closer = event.command.get_issuing_user();
    std::optional<Ticket> ticket_opt;
    uint64_t ticket_id_para_fechar = 0;

    auto param_codigo = event.get_parameter("codigo");

    if (auto val_ptr = std::get_if<int64_t>(&param_codigo)) {
        // O usuário forneceu um código
        ticket_id_para_fechar = static_cast<uint64_t>(*val_ptr);
        ticket_opt = ticketManager_.findTicketById(ticket_id_para_fechar);
    }
    else {
        // O usuário NÃO forneceu um código, tentar achar pelo canal
        ticket_opt = ticketManager_.findTicketByChannel(event.command.channel_id);
        if (ticket_opt) {
            ticket_id_para_fechar = ticket_opt->ticket_id;
        }
    }

    // --- Verificações ---
    if (!ticket_opt) {
        if (ticket_id_para_fechar != 0) {
            event.reply(dpp::message("❌ Código de ticket não encontrado.").set_flags(dpp::m_ephemeral));
        }
        else {
            event.reply(dpp::message("❌ Nenhum ticket aberto encontrado neste canal. Se quiser fechar um ticket específico, use `/finalizar_ticket codigo: <id>`.").set_flags(dpp::m_ephemeral));
        }
        return;
    }

    Ticket ticket = *ticket_opt;

    if (event.command.channel_id != ticket.channel_id) {
        event.reply(dpp::message("❌ Este comando só pode ser usado dentro do canal do ticket correspondente (`" + std::to_string(ticket.channel_id) + "`).").set_flags(dpp::m_ephemeral));
        return;
    }

    if (ticket.status != "aberto") {
        event.reply(dpp::message("❌ Este ticket não está mais aberto.").set_flags(dpp::m_ephemeral));
        return;
    }

    if (closer.id != ticket.user_a_id && closer.id != ticket.user_b_id) {
        // Verificação de permissão de admin/cargo
        dpp::permission user_perms = event.command.get_resolved_permission(closer.id);
        const auto& roles = event.command.member.get_roles();
        bool has_cargo_role = std::find(roles.begin(), roles.end(), config_.cargo_permitido) != roles.end();

        if (!user_perms.has(dpp::p_administrator) && !has_cargo_role) {
            event.reply(dpp::message("❌ Apenas os participantes do ticket ou a staff podem fechá-lo.").set_flags(dpp::m_ephemeral));
            return;
        }
    }

    // --- Lógica de Fechamento ---
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
        }

        ticketManager_.arquivarTicket(ticket.ticket_id, log_filename);

        dpp::embed log_embed;
        log_embed.set_color(dpp::colors::red).set_title("Ticket #" + std::to_string(ticket.ticket_id) + " Fechado");
        log_embed.set_description("O ticket entre <@" + std::to_string(ticket.user_a_id) + "> e <@" + std::to_string(ticket.user_b_id) + "> foi fechado por " + closer.get_mention() + ".");
        log_embed.add_field("Log Salvo", log_filename, false);
        bot_.message_create(dpp::message(config_.canal_logs, log_embed));
        });
}

void TicketCommands::handle_ver_log(const dpp::slashcommand_t& event) {
    int64_t ticket_id_int = std::get<int64_t>(event.get_parameter("codigo"));
    uint64_t ticket_id = static_cast<uint64_t>(ticket_id_int);

    dpp::permission user_perms = event.command.get_resolved_permission(event.command.get_issuing_user().id);
    bool has_admin_perm = user_perms.has(dpp::p_administrator);

    const auto& roles = event.command.member.get_roles();

    bool has_cargo_role = std::find(roles.begin(), roles.end(), config_.cargo_permitido) != roles.end();

    if (!has_admin_perm && !has_cargo_role) {
        event.reply(dpp::message("❌ Você não tem permissão para usar este comando.").set_flags(dpp::m_ephemeral));
        return;
    }

    std::optional<Ticket> ticket_opt = ticketManager_.findTicketById(ticket_id);

    if (!ticket_opt) {
        event.reply(dpp::message("❌ Código de ticket não encontrado no banco de dados.").set_flags(dpp::m_ephemeral));
        return;
    }

    Ticket ticket = *ticket_opt;

    if (ticket.status == "aberto") {
        event.reply(dpp::message("❌ Este ticket ainda está aberto. Só é possível baixar logs de tickets fechados.").set_flags(dpp::m_ephemeral));
        return;
    }

    if (ticket.log_filename.empty()) {
        event.reply(dpp::message("❌ Este ticket fechado não possui um arquivo de log registrado.").set_flags(dpp::m_ephemeral));
        return;
    }

    if (!std::filesystem::exists(ticket.log_filename)) {
        event.reply(dpp::message("❌ O arquivo de log (`" + ticket.log_filename + "`) não foi encontrado no servidor. Pode ter sido apagado.").set_flags(dpp::m_ephemeral));
        return;
    }

    try {
        dpp::message msg;
        msg.set_flags(dpp::m_ephemeral);
        msg.set_content("Aqui está o log do Ticket #" + std::to_string(ticket.ticket_id));
        msg.add_file(ticket.log_filename, dpp::utility::read_file(ticket.log_filename));
        event.reply(msg);
    }
    catch (const dpp::exception& e) {
        event.reply(dpp::message("❌ Erro ao ler ou anexar o arquivo de log: " + std::string(e.what())).set_flags(dpp::m_ephemeral));
        Utils::log_to_file("ERRO: Falha ao ler/anexar log " + ticket.log_filename + ": " + e.what());
    }
    catch (const std::exception& e) {
        event.reply(dpp::message("❌ Erro de sistema ao ler o arquivo de log: " + std::string(e.what())).set_flags(dpp::m_ephemeral));
        Utils::log_to_file("ERRO: Falha ao ler/anexar log " + ticket.log_filename + ": " + e.what());
    }
}


void TicketCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id, const BotConfig& config) {
    dpp::slashcommand chamar_cmd("chamar", "Inicia um ticket de conversa privada com um usuário.", bot_id);
    chamar_cmd.add_option(dpp::command_option(dpp::co_user, "usuario", "O usuário com quem você quer falar.", true));
    commands.push_back(chamar_cmd);

    dpp::slashcommand finalizar_cmd("finalizar_ticket", "Fecha e arquiva um ticket de conversa.", bot_id);
    // Alteração: "codigo" agora é opcional (false)
    finalizar_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O número do ticket (opcional se usado no canal).", false));
    commands.push_back(finalizar_cmd);

    dpp::slashcommand ver_log_cmd("ver_log", "Baixa o arquivo de log de um ticket fechado (Admin).", bot_id);
    ver_log_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O número do ticket fechado.", true));
    ver_log_cmd.set_default_permissions(0);
    ver_log_cmd.add_permission(dpp::command_permission(config.cargo_permitido, dpp::cpt_role, true));
    commands.push_back(ver_log_cmd);
}