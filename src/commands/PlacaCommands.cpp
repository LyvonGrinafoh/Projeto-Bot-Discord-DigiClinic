#include "PlacaCommands.h"
#include "Utils.h"
#include <CommandHandler.h>
#include <variant>

PlacaCommands::PlacaCommands(dpp::cluster& bot, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), config_(config), cmdHandler_(handler) {

}

void PlacaCommands::handle_placa(const dpp::slashcommand_t& event) {
    std::string doutor = std::get<std::string>(event.get_parameter("doutor"));

    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::orange)
        .set_title("📢 Nova Solicitação de Placa")
        .add_field("Profissional", doutor, false)
        .add_field("Solicitado por", event.command.get_issuing_user().get_mention(), false)
        .set_footer(dpp::embed_footer().set_text("Aguardando criação da arte..."));

    auto param_anexo = event.get_parameter("imagem_referencia");
    if (auto attachment_id_ptr = std::get_if<dpp::snowflake>(&param_anexo)) {
        dpp::attachment anexo = event.command.get_resolved_attachment(*attachment_id_ptr);
        if (anexo.content_type.find("image/") == 0) {
            embed.set_image(anexo.url);
        }
    }

    dpp::message msg(config_.canal_solicitacao_placas, embed);
    bot_.message_create(msg);

    event.reply("✅ Solicitação de placa para **" + doutor + "** enviada com sucesso!");
}

void PlacaCommands::handle_finalizar_placa(const dpp::slashcommand_t& event) {
    std::string doutor = std::get<std::string>(event.get_parameter("doutor"));
    dpp::snowflake anexo_id = std::get<dpp::snowflake>(event.get_parameter("arte_final"));
    dpp::attachment anexo = event.command.get_resolved_attachment(anexo_id);

    if (anexo.content_type.find("image/") != 0) {
        event.reply(dpp::message("❌ O anexo 'arte_final' precisa ser uma imagem.").set_flags(dpp::m_ephemeral));
        return;
    }

    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::green)
        .set_title("✅ Placa Finalizada")
        .add_field("Profissional", doutor, false)
        .set_image(anexo.url)
        .set_footer(dpp::embed_footer().set_text("Arte criada por: " + event.command.get_issuing_user().username));

    dpp::message msg(config_.canal_placas_finalizadas, embed);
    bot_.message_create(msg);

    event.reply("🎉 Arte da placa para **" + doutor + "** foi enviada com sucesso!");
}

void PlacaCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand placa_cmd("placa", "Solicita a criação de uma placa para um profissional.", bot_id);
    placa_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome do profissional para a placa.", true));
    placa_cmd.add_option(dpp::command_option(dpp::co_attachment, "imagem_referencia", "Uma imagem de referência (opcional).", false));
    commands.push_back(placa_cmd);

    dpp::slashcommand finalizar_placa_cmd("finalizar_placa", "Envia a arte finalizada de uma placa.", bot_id);
    finalizar_placa_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome do profissional da placa finalizada.", true));
    finalizar_placa_cmd.add_option(dpp::command_option(dpp::co_attachment, "arte_final", "A imagem da placa finalizada.", true));
    commands.push_back(finalizar_placa_cmd);
}