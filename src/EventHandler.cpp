#include "EventHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include "TicketManager.h"
#include "DatabaseManager.h"
#include <iostream>
#include <fstream>
#include <string>

EventHandler::EventHandler(dpp::cluster& bot, ConfigManager& configMgr, TicketManager& ticketMgr, DatabaseManager& db, AIHandler& ai) :
    bot_(bot), configManager_(configMgr), ticketManager_(ticketMgr), db_(db), ai_(ai) {
}

void EventHandler::updateMemory(dpp::snowflake channel_id, const std::string& user, const std::string& content) {
    auto& mem = memory_buffer_[channel_id];
    mem.push_back(user + ": " + content);
    if (mem.size() > 6) mem.pop_front(); // Mantém últimas 6 interações
}

std::string EventHandler::getMemoryContext(dpp::snowflake channel_id) {
    if (memory_buffer_.find(channel_id) == memory_buffer_.end()) return "";
    std::string ctx = "\n--- HISTÓRICO RECENTE ---\n";
    for (const auto& line : memory_buffer_[channel_id]) {
        ctx += line + "\n";
    }
    return ctx;
}

void EventHandler::addPaginationState(dpp::snowflake message_id, PaginationState state) {
    std::lock_guard<std::mutex> lock(pagination_mutex_);
    active_paginations_[message_id] = state;
}

void EventHandler::onMessageCreate(const dpp::message_create_t& event) {
    if (event.msg.author.is_bot()) return;

    // Logs de Ticket
    auto ticket = ticketManager_.findTicketByChannel(event.msg.channel_id);
    if (ticket) {
        ticketManager_.appendToLog(event.msg);
    }
    if (event.msg.content == "!ping") {
        bot_.message_create(dpp::message(event.msg.channel_id, "Pong!"));
    }

    // --- LÓGICA DE IA POR MENÇÃO ---
    bool mentioned = false;
    for (const auto& user : event.msg.mentions) {
        if (user.first.id == bot_.me.id) {
            mentioned = true;
            break;
        }
    }

    if (mentioned) {
        // 1. Limpa a menção da string para não confundir a IA
        std::string clean_content = event.msg.content;
        std::string mention_str = "<@" + std::to_string(bot_.me.id) + ">";
        size_t pos = clean_content.find(mention_str);
        if (pos != std::string::npos) clean_content.erase(pos, mention_str.length());

        // 2. Registra na memória antes de processar
        updateMemory(event.msg.channel_id, event.msg.author.username, clean_content);

        // 3. Constrói o contexto completo
        bot_.message_add_reaction(event.msg.id, event.msg.channel_id, "🧠"); // Feedback visual

        std::string manual = db_.getManualCompleto();
        std::string historico = getMemoryContext(event.msg.channel_id);

        std::string sistema = manual + "\n" + historico + "\nINSTRUÇÃO ATUAL: O usuário " + event.msg.author.username + " está falando com você. Responda diretamente a ele.";

        // 4. Chama a IA
        ai_.AskWithContext(sistema, clean_content, [this, event](std::string resposta) {
            bot_.message_delete_reaction(event.msg.id, event.msg.channel_id, event.msg.author.id, "🧠");

            // Registra resposta do bot na memória também
            updateMemory(event.msg.channel_id, "Bot", resposta);

            dpp::message reply(event.msg.channel_id, resposta);
            reply.set_reference(event.msg.id);
            bot_.message_create(reply);
            });
    }
}

void EventHandler::onMessageUpdate(const dpp::message_update_t& event) {}

void EventHandler::onMessageDelete(const dpp::message_delete_t& event) {
    std::lock_guard<std::mutex> lock(pagination_mutex_);
    if (active_paginations_.count(event.id)) {
        active_paginations_.erase(event.id);
    }
}

void EventHandler::onMessageReactionAdd(const dpp::message_reaction_add_t& event) {
    if (event.reacting_user.is_bot()) return;

    if (event.reacting_emoji.name == "🗑️") {
        bot_.message_delete(event.message_id, event.channel_id);
        std::lock_guard<std::mutex> lock(pagination_mutex_);
        if (active_paginations_.count(event.message_id)) {
            active_paginations_.erase(event.message_id);
        }
        return;
    }

    std::lock_guard<std::mutex> lock(pagination_mutex_);
    auto it = active_paginations_.find(event.message_id);

    if (it != active_paginations_.end()) {
        PaginationState& state = it->second;
        if (event.reacting_user.id != state.originalUserID) return;

        int totalPages = (state.items.size() + state.itemsPerPage - 1) / state.itemsPerPage;
        bool changed = false;

        if (event.reacting_emoji.name == "◀️") {
            if (state.currentPage > 1) { state.currentPage--; changed = true; }
        }
        else if (event.reacting_emoji.name == "▶️") {
            if (state.currentPage < totalPages) { state.currentPage++; changed = true; }
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

void EventHandler::onButtonClickListener(const dpp::button_click_t& event) {
    if (event.custom_id.rfind("btn_fin_", 0) == 0) {
        std::string codigo_str = event.custom_id.substr(8);
        dpp::interaction_modal_response modal("modal_fin_" + codigo_str, "Finalizar Solicitação #" + codigo_str);
        modal.add_component(dpp::component().set_label("Observação (Opcional)").set_id("obs_field").set_type(dpp::cot_text).set_placeholder("Descreva como foi resolvido...").set_min_length(0).set_max_length(1000).set_text_style(dpp::text_paragraph).set_required(false));
        event.dialog(modal);
    }
}

void EventHandler::onFormSubmitListener(const dpp::form_submit_t& event) {
    if (event.custom_id == "modal_relatorio_dia") {
        std::string conteudo = std::get<std::string>(event.components[0].components[0].value);
        dpp::user u = event.command.get_issuing_user();
        RelatorioDiario rel; rel.id = Utils::gerar_codigo(); rel.user_id = u.id; rel.username = u.username; rel.data_hora = Utils::format_timestamp(std::time(nullptr)); rel.conteudo = conteudo;
        if (db_.addOrUpdateRelatorio(rel)) {
            event.reply(dpp::message("✅ Relatório diário registrado com sucesso!").set_flags(dpp::m_ephemeral));
            dpp::embed log_embed = Utils::criarEmbedPadrao("📄 Relatório Diário Enviado", conteudo, dpp::colors::blue);
            log_embed.add_field("Funcionário", u.get_mention(), true);
            log_embed.add_field("Data", rel.data_hora, true);
            bot_.message_create(dpp::message(configManager_.getConfig().canal_logs, log_embed));
        }
        else { event.reply(dpp::message("❌ Erro ao salvar relatório.").set_flags(dpp::m_ephemeral)); }
        return;
    }
    if (event.custom_id.rfind("modal_fin_", 0) == 0) {
        std::string codigo_str = event.custom_id.substr(10);
        uint64_t codigo = 0; try { codigo = std::stoull(codigo_str); }
        catch (...) { return; }
        std::string observacao = std::get<std::string>(event.components[0].components[0].value);
        if (observacao.empty()) observacao = "Nenhuma.";
        auto sol_opt = db_.getSolicitacao(codigo);
        if (!sol_opt) { event.reply(dpp::message("Solicitação não encontrada.").set_flags(dpp::m_ephemeral)); return; }
        Solicitacao sol = *sol_opt;
        if (sol.status != "pendente") { event.reply(dpp::message("Esta solicitação não está pendente.").set_flags(dpp::m_ephemeral)); return; }
        sol.status = "finalizada"; sol.data_finalizacao = Utils::format_timestamp(std::time(nullptr)); sol.observacao_finalizacao = observacao;
        if (db_.addOrUpdateSolicitacao(sol)) {
            dpp::snowflake channel_id = 0; std::string title = "";
            if (sol.tipo == DEMANDA) { channel_id = configManager_.getConfig().canal_finalizadas; title = "✅ Demanda Finalizada"; }
            else if (sol.tipo == PEDIDO) { channel_id = configManager_.getConfig().canal_pedidos_concluidos; title = "✅ Pedido Finalizado"; }
            if (channel_id != 0) {
                dpp::embed embed = Utils::criarEmbedPadrao(title, sol.texto, dpp::colors::dark_green);
                embed.add_field("Código", std::to_string(sol.id), false).add_field("Responsável", "<@" + std::to_string(sol.id_usuario_responsavel) + ">", true).add_field("Finalizado por", event.command.get_issuing_user().get_mention(), true);
                if (sol.tipo == DEMANDA) { std::string prazo_desc = sol.prazo; if (Utils::compararDatas(sol.data_finalizacao, sol.prazo) == 1) { prazo_desc += " (ENTREGUE COM ATRASO ⚠️)"; } embed.add_field("Prazo", prazo_desc, true); }
                if (!observacao.empty()) { embed.add_field("Observação", observacao, false); }
                bot_.message_create(dpp::message(channel_id, embed));
            }
            event.reply(dpp::message("Solicitação finalizada com sucesso!"));
        }
        else { event.reply(dpp::message("Erro ao salvar.").set_flags(dpp::m_ephemeral)); }
    }
}