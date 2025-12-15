#include "TicketCommands.h"
#include "TicketManager.h"
#include "CommandHandler.h"
#include "Utils.h"
#include <sstream>

TicketCommands::TicketCommands(dpp::cluster& bot, TicketManager& tm, const BotConfig& config, CommandHandler& handler)
    : bot_(bot), ticketManager_(tm), config_(config), cmdHandler_(handler) {
}

void TicketCommands::handle_chamar(const dpp::slashcommand_t& event) {
    // 1. Verificar canal (opcional)
    if (event.command.channel_id != config_.canal_sugestoes && event.command.channel_id != config_.canal_bugs) {
        // Logica opcional de restricao
    }

    dpp::user user = event.command.get_issuing_user();
    std::string assunto = std::get<std::string>(event.get_parameter("motivo"));

    event.thinking(true);

    // 2. Criar canal de texto privado
    dpp::channel new_channel;
    new_channel.set_name("ticket-" + user.username);
    new_channel.set_guild_id(event.command.guild_id);
    new_channel.set_type(dpp::CHANNEL_TEXT);


    // @everyone: Negar ver canal
    new_channel.permission_overwrites.push_back(
        dpp::permission_overwrite(event.command.guild_id, 0, dpp::p_view_channel, dpp::ot_role)
    );

    // Bot: Permitir tudo
    new_channel.permission_overwrites.push_back(
        dpp::permission_overwrite(bot_.me.id, dpp::p_view_channel | dpp::p_send_messages | dpp::p_manage_channels, 0, dpp::ot_member)
    );

    // Usuário que abriu: Permitir ver e enviar
    new_channel.permission_overwrites.push_back(
        dpp::permission_overwrite(user.id, dpp::p_view_channel | dpp::p_send_messages, 0, dpp::ot_member)
    );

    // Cargo ADM: Permitir ver e enviar
    if (config_.cargo_adm) {
        new_channel.permission_overwrites.push_back(
            dpp::permission_overwrite(config_.cargo_adm, dpp::p_view_channel | dpp::p_send_messages, 0, dpp::ot_role)
        );
    }
    // -------------------------------------------------------------

    bot_.channel_create(new_channel, [this, event, user, assunto](const dpp::confirmation_callback_t& cb) {
        if (cb.is_error()) {
            event.edit_response("❌ Erro ao criar canal do ticket: " + cb.get_error().message);
            return;
        }

        dpp::channel created_channel = std::get<dpp::channel>(cb.value);

        // Criar ticket no DB
        Ticket t = ticketManager_.createTicket(user.id, 0, created_channel.id, assunto);

        // Mensagem de boas-vindas
        dpp::embed embed = Utils::criarEmbedPadrao("🎫 Ticket #" + std::to_string(t.ticket_id),
            "Olá " + user.get_mention() + "!\n\nDescreva seu problema aqui. Um administrador irá atendê-lo em breve.\n**Assunto:** " + assunto,
            dpp::colors::green);

        dpp::message msg(created_channel.id, embed);
        msg.add_component(dpp::component().add_component(
            dpp::component().set_type(dpp::cot_button).set_label("🔒 Finalizar Ticket").set_style(dpp::cos_danger).set_id("btn_finalizar_ticket")
        ));

        bot_.message_create(msg);
        event.edit_response("✅ Ticket criado: " + created_channel.get_mention());
        });
}

void TicketCommands::handle_finalizar_ticket(const dpp::slashcommand_t& event) {
    auto ticket_opt = ticketManager_.findTicketByChannel(event.command.channel_id);

    if (!ticket_opt.has_value()) {
        event.reply(dpp::message("❌ Este comando só funciona em canais de ticket abertos.").set_flags(dpp::m_ephemeral));
        return;
    }

    event.thinking(false);
    Ticket t = ticket_opt.value();

    std::string log_content = ticketManager_.getAndRemoveLog(t.channel_id);
    std::string filename = "logs/ticket_" + std::to_string(t.ticket_id) + ".txt";


    ticketManager_.arquivarTicket(t.ticket_id, filename);

    dpp::embed embed = Utils::criarEmbedPadrao("🔒 Ticket Fechado", "Este ticket será excluído em 5 segundos.", dpp::colors::red);
    event.edit_response(dpp::message(event.command.channel_id, embed));

    bot_.start_timer([this, channel_id = t.channel_id](dpp::timer h) {
        bot_.channel_delete(channel_id);
        bot_.stop_timer(h);
        }, 5);
}

void TicketCommands::handle_ver_log(const dpp::slashcommand_t& event) {
    event.reply(dpp::message("📄 Funcionalidade em desenvolvimento.").set_flags(dpp::m_ephemeral));
}

void TicketCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id, const BotConfig& config) {
    dpp::slashcommand chamar("chamar", "Abre um ticket de suporte.", bot_id);
    chamar.add_option(dpp::command_option(dpp::co_string, "motivo", "Motivo do contato", true));
    commands.push_back(chamar);

    dpp::slashcommand finalizar("finalizar_ticket", "Fecha o ticket atual.", bot_id);
    commands.push_back(finalizar);
}