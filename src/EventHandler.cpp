#include "EventHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <vector>

EventHandler::EventHandler(dpp::cluster& bot, ConfigManager& configMgr) :
    bot_(bot), configManager_(configMgr) {
}

void EventHandler::onMessageCreate(const dpp::message_create_t& event) {
    const BotConfig& config_ = configManager_.getConfig();
    if (event.msg.author.is_bot() || event.msg.channel_id == config_.canal_logs) { return; }

    if (event.msg.channel_id == config_.canal_sugestoes || event.msg.channel_id == config_.canal_bugs) {
        if (event.msg.content.empty()) { return; }
        bool is_sugestao = (event.msg.channel_id == config_.canal_sugestoes);
        dpp::embed embed = dpp::embed().set_author(event.msg.author.username, "", event.msg.author.get_avatar_url()).set_title(is_sugestao ? "💡 Nova Sugestão" : "🐞 Novo Bug Reportado").set_description(event.msg.content).set_color(is_sugestao ? dpp::colors::yellow : dpp::colors::red).set_timestamp(time(0));
        dpp::message msg(event.msg.channel_id, embed);
        bot_.message_create(msg, [this, event](const dpp::confirmation_callback_t& callback) {
            if (!callback.is_error()) {
                auto new_message = callback.get<dpp::message>();
                bot_.message_add_reaction(new_message.id, new_message.channel_id, "👍");
                bot_.message_add_reaction(new_message.id, new_message.channel_id, "👎");
                bot_.message_delete(event.msg.id, event.msg.channel_id);
            }
            else { Utils::log_to_file("ERRO ao criar embed de sugestao/bug: " + callback.get_error().message); }
            });
        return;
    }

    dpp::channel* c = dpp::find_channel(event.msg.channel_id); std::string channel_name = c ? c->name : std::to_string(event.msg.channel_id); std::string log_message = "`[" + Utils::format_timestamp(event.msg.sent) + "]` `(#" + channel_name + ")` `" + event.msg.author.username + "` `(" + std::to_string(event.msg.author.id) + ")`: " + event.msg.content;
    if (!event.msg.attachments.empty()) { for (const auto& att : event.msg.attachments) { log_message += " [Anexo: " + att.url + "]"; } }
    bot_.message_create(dpp::message(config_.canal_logs, log_message));
    dpp::snowflake channel_id = event.msg.channel_id;
    auto is_in_channel_list = [&](const std::vector<dpp::snowflake>& list) { return std::find(list.begin(), list.end(), channel_id) != list.end(); };

    if (is_in_channel_list(config_.canais_so_imagens)) { if (event.msg.attachments.empty() && event.msg.embeds.empty()) { bot_.message_delete(event.msg.id, event.msg.channel_id); bot_.direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de imagens foi removida. Por favor, envie apenas imagens ou GIFs nesse canal.")); } }
    else if (is_in_channel_list(config_.canais_so_links)) { if (event.msg.content.find("http://") == std::string::npos && event.msg.content.find("https://") == std::string::npos) { bot_.message_delete(event.msg.id, event.msg.channel_id); bot_.direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de links foi removida. Por favor, envie apenas links nesse canal.")); } }
    else if (is_in_channel_list(config_.canais_so_documentos)) { if (event.msg.attachments.empty()) { bot_.message_delete(event.msg.id, event.msg.channel_id); bot_.direct_message_create(event.msg.author.id, dpp::message("Sua mensagem no canal de documentos foi removida. Por favor, envie apenas arquivos nesse canal.")); } }
}

void EventHandler::onMessageUpdate(const dpp::message_update_t& event) {
    const BotConfig& config_ = configManager_.getConfig();
    if (event.msg.author.is_bot() || event.msg.channel_id == config_.canal_logs) { return; }
    dpp::channel* c = dpp::find_channel(event.msg.channel_id); std::string channel_name = c ? c->name : std::to_string(event.msg.channel_id); std::string log_message = "`[" + Utils::format_timestamp(event.msg.edited) + "]` `[EDITADA]` `(#" + channel_name + ")` `" + event.msg.author.username + "` `(" + std::to_string(event.msg.author.id) + ")`: " + event.msg.content; bot_.message_create(dpp::message(config_.canal_logs, log_message));
}

void EventHandler::onMessageDelete(const dpp::message_delete_t& event) {
    const BotConfig& config_ = configManager_.getConfig();
    if (event.channel_id == config_.canal_logs) { return; }
    dpp::channel* c = dpp::find_channel(event.channel_id); std::string channel_name = c ? c->name : std::to_string(event.channel_id); std::string log_message = "`[" + Utils::format_timestamp(std::time(nullptr)) + "]` `[DELETADA]` `(#" + channel_name + ")` `Mensagem ID: " + std::to_string(event.id) + "`"; bot_.message_create(dpp::message(config_.canal_logs, log_message));
}

void EventHandler::onMessageReactionAdd(const dpp::message_reaction_add_t& event) {
    const BotConfig& config_ = configManager_.getConfig();
    if (event.reacting_user.is_bot() || event.channel_id == config_.canal_logs) { return; }
    dpp::channel* c = dpp::find_channel(event.channel_id); std::string channel_name = c ? c->name : std::to_string(event.channel_id); std::string emoji_mention = event.reacting_emoji.get_mention(); std::string log_message = "`[" + Utils::format_timestamp(std::time(nullptr)) + "]` `[REAÇÃO]` `(#" + channel_name + ")` `" + event.reacting_user.username + "` `(" + std::to_string(event.reacting_user.id) + ")` reagiu com " + emoji_mention + " à mensagem " + dpp::utility::message_url(event.reacting_guild.id, event.channel_id, event.message_id); bot_.message_create(dpp::message(config_.canal_logs, log_message));
}