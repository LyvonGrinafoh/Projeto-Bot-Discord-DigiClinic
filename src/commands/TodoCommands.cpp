#include "TodoCommands.h"
#include "CommandHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include <variant>
#include <string>

TodoCommands::TodoCommands(dpp::cluster& bot, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), config_(config), cmdHandler_(handler) {
}

void TodoCommands::handle_todo(const dpp::slashcommand_t& event) {
    if (event.command.get_issuing_user().id != seu_id_discord) {
        event.reply(dpp::message("❌ Este comando é restrito.").set_flags(dpp::m_ephemeral));
        return;
    }

    std::string anotacao = std::get<std::string>(event.get_parameter("anotacao"));

    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::yellow)
        .set_title("📝 Nova Anotação / TODO")
        .set_description(anotacao)
        .set_footer(dpp::embed_footer().set_text("Anotado por: " + event.command.get_issuing_user().username))
        .set_timestamp(std::time(nullptr));

    bot_.message_create(dpp::message(config_.canal_bugs, embed));

    event.reply(dpp::message("✅ Anotação registrada no canal <#" + std::to_string(config_.canal_bugs) + ">.").set_flags(dpp::m_ephemeral));
}

void TodoCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand cmd("todo", "Registra uma nova anotação/mudança (Restrito).", bot_id);
    cmd.add_option(dpp::command_option(dpp::co_string, "anotacao", "A nota a ser registrada.", true));

    cmd.set_default_permissions(0);
    cmd.add_permission(dpp::command_permission(187910708674035713, dpp::cpt_user, true));

    commands.push_back(cmd);
}