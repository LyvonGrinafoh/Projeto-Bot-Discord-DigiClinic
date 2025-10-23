#include "EventHandler.h"
#include "ConfigManager.h"
#include "TicketManager.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <variant>
#include <cmath>

dpp::embed generatePageEmbed(const PaginationState& state);

EventHandler::EventHandler(dpp::cluster& bot, ConfigManager& configMgr, TicketManager& tm) :
    bot_(bot), configManager_(configMgr), ticketManager_(tm) {
}

void EventHandler::addPaginationState(dpp::snowflake messageId, PaginationState state) {
    std::lock_guard<std::mutex> lock(paginationMutex_);
    activePaginations_[messageId] = std::move(state);

    bot_.start_timer([this, messageId](dpp::timer timer_handle) {
        this->removePaginationState(messageId);
        bot_.stop_timer(timer_handle);
        }, 600);
}

void EventHandler::removePaginationState(dpp::snowflake messageId) {
    dpp::snowflake channelIdToClean = 0;
    {
        std::lock_guard<std::mutex> lock(paginationMutex_);
        auto it = activePaginations_.find(messageId);
        if (it != activePaginations_.end()) {
            channelIdToClean = it->second.channel_id;
            activePaginations_.erase(it);
            Utils::log_to_file("Estado de paginação removido para a mensagem ID: " + std::to_string(messageId));
        }
    }
    if (channelIdToClean != 0) {
        bot_.message_delete(messageId, channelIdToClean, [messageId](const dpp::confirmation_callback_t& cb) {
            if (cb.is_error() && cb.get_error().code != 10008) {
                Utils::log_to_file("AVISO: Falha ao auto-deletar mensagem de paginação " + std::to_string(messageId) + ": " + cb.get_error().message);
            }
            });
    }
}

void EventHandler::onMessageReactionAdd(const dpp::message_reaction_add_t& event) {
    const BotConfig& config_ = configManager_.getConfig();
    if (event.reacting_user.is_bot() || event.channel_id == config_.canal_logs) { return; }

    bool stateChanged = false;
    PaginationState newState;
    {
        std::lock_guard<std::mutex> lock(paginationMutex_);
        auto it = activePaginations_.find(event.message_id);
        if (it != activePaginations_.end()) {
            PaginationState& state = it->second;

            if (event.reacting_user.id != state.originalUserID) {
                bot_.message_delete_reaction(event.message_id, event.channel_id, event.reacting_user.id, event.reacting_emoji.name);
                return;
            }

            int totalPages = static_cast<int>(std::ceil(static_cast<double>(state.items.size()) / state.itemsPerPage));
            std::string emoji_name = event.reacting_emoji.name;

            if (emoji_name == "◀️") {
                if (state.currentPage > 1) {
                    state.currentPage--;
                    stateChanged = true;
                }
            }
            else if (emoji_name == "▶️") {
                if (state.currentPage < totalPages) {
                    state.currentPage++;
                    stateChanged = true;
                }
            }

            if (stateChanged) {
                newState = state;
            }
            bot_.message_delete_reaction(event.message_id, event.channel_id, event.reacting_user.id, event.reacting_emoji.name);
        }
    }

    if (stateChanged) {
        dpp::embed newEmbed = generatePageEmbed(newState);
        dpp::message updatedMsg(event.channel_id, newEmbed);
        updatedMsg.id = event.message_id;
        bot_.message_edit(updatedMsg);
    }

    dpp::channel* c = dpp::find_channel(event.channel_id); std::string channel_name = c ? c->name : std::to_string(event.channel_id); std::string emoji_mention = event.reacting_emoji.get_mention(); std::string log_message = "`[" + Utils::format_timestamp(std::time(nullptr)) + "]` `[REAÇÃO]` `(#" + channel_name + ")` `" + event.reacting_user.username + "` `(" + std::to_string(event.reacting_user.id) + ")` reagiu com " + emoji_mention + " à mensagem " + dpp::utility::message_url(event.reacting_guild.id, event.channel_id, event.message_id); bot_.message_create(dpp::message(config_.canal_logs, log_message));
}

void EventHandler::onMessageCreate(const dpp::message_create_t& event) {
    const BotConfig& config_ = configManager_.getConfig();
    if (event.msg.author.is_bot() || event.msg.channel_id == config_.canal_logs) { return; }

    auto ticket_opt = ticketManager_.findTicketByChannel(event.msg.channel_id);
    if (ticket_opt) {
        std::string log_line = "[" + Utils::format_timestamp(event.msg.sent) + "] " + event.msg.author.username + ": " + event.msg.content;
        if (!event.msg.attachments.empty()) {
            for (const auto& att : event.msg.attachments) {
                log_line += " [Anexo: " + att.url + "]";
            }
        }
        ticketManager_.appendToLog(event.msg.channel_id, log_line);
    }

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

    if (!ticket_opt) {
        bot_.message_create(dpp::message(config_.canal_logs, log_message));
    }

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

dpp::embed generatePageEmbed(const PaginationState& state) {
    dpp::embed embed;
    embed.set_color(dpp::colors::blue);
    int totalItems = state.items.size();
    if (totalItems == 0) { embed.set_title("Nenhum item encontrado"); embed.set_description("A lista está vazia."); return embed; }
    int totalPages = static_cast<int>(std::ceil(static_cast<double>(totalItems) / state.itemsPerPage));
    if (totalPages == 0) totalPages = 1;
    int startIdx = (state.currentPage - 1) * state.itemsPerPage;
    int endIdx = std::min(startIdx + state.itemsPerPage, totalItems);
    std::string description; std::string title;
    if (state.listType == "leads") { title = "📋 Lista de Leads"; }
    else if (state.listType == "visitas") { title = "📅 Visitas Agendadas"; }
    else if (state.listType == "demandas") { title = "📋 Solicitações Pendentes"; }
    else { title = "Lista Paginada"; }
    embed.set_title(title);
    if (startIdx >= totalItems) { description = "Nenhum item nesta página."; }
    else {
        for (int i = startIdx; i < endIdx; ++i) {
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Lead>) { description += "**Nome:** " + arg.nome + "\n"; description += "**Codigo:** `" + std::to_string(arg.id) + "`\n"; description += "**Data Contato:** " + arg.data_contato + "\n"; description += "**Contato:** " + (arg.contato.empty() ? "N/A" : arg.contato) + "\n"; description += "**Status:** " + arg.status_conversa + "\n"; }
                else if constexpr (std::is_same_v<T, Visita>) { bool cancelada = (arg.observacoes.find("Visita Cancelada") != std::string::npos); if (cancelada) description += "**[CANCELADA]** "; description += "**Dr(a):** " + arg.doutor + "\n"; description += "**Código:** `" + std::to_string(arg.id) + "`\n"; description += "**Data:** " + arg.data + " às " + arg.horario + "\n"; description += "**Unidade:** " + arg.unidade + "\n"; }
                else if constexpr (std::is_same_v<T, Solicitacao>) { std::string tipo_str; switch (arg.tipo) { case DEMANDA: tipo_str = "Demanda"; break; case PEDIDO: tipo_str = "Pedido"; break; case LEMBRETE:tipo_str = "Lembrete"; break; } description += "**" + tipo_str + "**\n"; description += "**Código:** `" + std::to_string(arg.id) + "`\n"; description += "**Descrição:** " + arg.texto + "\n"; if (arg.tipo != PEDIDO) { description += "**Prazo:** " + arg.prazo + "\n"; } }
                description += "--------------------\n";
                }, state.items[i]);
        }
    }
    if (description.length() > 4000) { description = description.substr(0, 4000) + "\n... (descrição muito longa)"; }
    embed.set_description(description); embed.set_footer(dpp::embed_footer().set_text("Página " + std::to_string(state.currentPage) + " / " + std::to_string(totalPages)));
    return embed;
}