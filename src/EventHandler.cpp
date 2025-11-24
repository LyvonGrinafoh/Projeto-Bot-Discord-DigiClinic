#include "EventHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include "TicketManager.h"
#include <iostream>
#include <fstream>

EventHandler::EventHandler(dpp::cluster& bot, ConfigManager& configMgr, TicketManager& ticketMgr) :
    bot_(bot), configManager_(configMgr), ticketManager_(ticketMgr) {
}

void EventHandler::addPaginationState(dpp::snowflake message_id, PaginationState state) {
    std::lock_guard<std::mutex> lock(pagination_mutex_);
    active_paginations_[message_id] = state;
}

void EventHandler::onMessageCreate(const dpp::message_create_t& event) {
    auto ticket = ticketManager_.findTicketByChannel(event.msg.channel_id);
    if (ticket) {
        if (!event.msg.author.is_bot()) { 
            ticketManager_.appendToLog(event.msg);
        }
    }

    if (event.msg.content == "!ping") {
        bot_.message_create(dpp::message(event.msg.channel_id, "Pong!"));
    }
}

void EventHandler::onMessageUpdate(const dpp::message_update_t& event) {
}

void EventHandler::onMessageDelete(const dpp::message_delete_t& event) {
    std::lock_guard<std::mutex> lock(pagination_mutex_);
    if (active_paginations_.count(event.id)) {
        active_paginations_.erase(event.id);
    }
}

void EventHandler::onMessageReactionAdd(const dpp::message_reaction_add_t& event) {
    if (event.reacting_user.is_bot()) return;

    std::lock_guard<std::mutex> lock(pagination_mutex_);
    auto it = active_paginations_.find(event.message_id);

    if (it != active_paginations_.end()) {
        PaginationState& state = it->second;

        if (event.reacting_user.id != state.originalUserID) {
            return;
        }

        if (event.reacting_emoji.name == "🗑️") {
            bot_.message_delete(event.message_id, event.channel_id);
            active_paginations_.erase(it);
            return;
        }

        int totalPages = (state.items.size() + state.itemsPerPage - 1) / state.itemsPerPage;
        bool changed = false;

        if (event.reacting_emoji.name == "◀️") {
            if (state.currentPage > 1) {
                state.currentPage--;
                changed = true;
            }
        }
        else if (event.reacting_emoji.name == "▶️") {
            if (state.currentPage < totalPages) {
                state.currentPage++;
                changed = true;
            }
        }

        if (changed) {
            dpp::embed newEmbed = Utils::generatePageEmbed(state);
            dpp::message m(event.channel_id, newEmbed);
            m.id = event.message_id;
            bot_.message_edit(m);

            bot_.message_delete_reaction(m, event.reacting_user.id, event.reacting_emoji.name);
        }
    }
}