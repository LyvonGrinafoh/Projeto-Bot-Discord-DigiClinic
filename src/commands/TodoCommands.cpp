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
        cmdHandler_.replyAndDelete(event, dpp::message("❌ Restrito."));
        return;
    }

    std::string anotacao = std::get<std::string>(event.get_parameter("anotacao"));

    dpp::embed embed = Utils::criarEmbedPadrao("📝 Nova Anotação / TODO", anotacao, dpp::colors::yellow);
    embed.set_footer(dpp::embed_footer().set_text("Autor: " + event.command.get_issuing_user().username));

    bot_.message_create(dpp::message(config_.canal_bugs, embed));
    cmdHandler_.replyAndDelete(event, dpp::message("✅ Anotação registrada em <#" + std::to_string(config_.canal_bugs) + ">."));
}

void TodoCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand cmd("todo", "Nota (Restrito).", bot_id);
    cmd.add_option(dpp::command_option(dpp::co_string, "anotacao", "Texto.", true));
    cmd.set_default_permissions(0);
    commands.push_back(cmd);
}