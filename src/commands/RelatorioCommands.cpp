#include "RelatorioCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include <iostream>

RelatorioCommands::RelatorioCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler) {
}

void RelatorioCommands::handle_relatorio_do_dia(const dpp::slashcommand_t& event) {
    // Abre um formulário (Modal) para o funcionário escrever
    dpp::interaction_modal_response modal("modal_relatorio_dia", "Relatório do Dia");

    modal.add_component(
        dpp::component().set_label("Descreva suas atividades de hoje")
        .set_id("conteudo_relatorio")
        .set_type(dpp::cot_text)
        .set_placeholder("Hoje eu fiz...")
        .set_min_length(10)
        .set_max_length(4000)
        .set_text_style(dpp::text_paragraph)
        .set_required(true)
    );

    event.dialog(modal);
}

void RelatorioCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand cmd("relatorio_do_dia", "Envia o seu relatório diário de atividades.", bot_id);
    commands.push_back(cmd);
}