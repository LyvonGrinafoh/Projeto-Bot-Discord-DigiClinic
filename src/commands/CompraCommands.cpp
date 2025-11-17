#include "CompraCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include <iostream>
#include <variant>
#include <sstream>
#include <iomanip>

CompraCommands::CompraCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler) {
}

void CompraCommands::handle_adicionar_compra(const dpp::slashcommand_t& event) {
    Compra nova_compra;
    nova_compra.id = Utils::gerar_codigo();
    nova_compra.descricao = std::get<std::string>(event.get_parameter("descricao"));
    nova_compra.local_compra = std::get<std::string>(event.get_parameter("local"));
    nova_compra.valor = std::get<double>(event.get_parameter("valor"));

    auto obs_param = event.get_parameter("observacao");
    if (auto val_ptr = std::get_if<std::string>(&obs_param)) {
        nova_compra.observacao = *val_ptr;
    }
    else {
        nova_compra.observacao = "Nenhuma.";
    }

    nova_compra.registrado_por = event.command.get_issuing_user().username;
    nova_compra.data_registro = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateCompra(nova_compra)) {

        std::stringstream ss;
        ss << "R$ " << std::fixed << std::setprecision(2) << nova_compra.valor;
        std::string valor_formatado = ss.str();

        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::light_sea_green)
            .set_title("💳 Novo Gasto Registrado")
            .add_field("Descrição", nova_compra.descricao, false)
            .add_field("Local", nova_compra.local_compra, true)
            .add_field("Valor", valor_formatado, true)
            .add_field("Observação", nova_compra.observacao, false)
            .set_footer(dpp::embed_footer().set_text("Registrado por: " + nova_compra.registrado_por));

        event.reply(dpp::message().add_embed(embed));
    }
    else {
        event.reply(dpp::message("❌ Erro ao salvar o registro de gasto no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void CompraCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand cmd("registrar_gasto", "Registra um novo gasto no cartão de crédito.", bot_id);
    cmd.add_option(dpp::command_option(dpp::co_string, "descricao", "Descrição do gasto (ex: Compra Papelaria).", true));
    cmd.add_option(dpp::command_option(dpp::co_string, "local", "Local da compra (ex: Kalunga, Mercado Livre).", true));
    cmd.add_option(dpp::command_option(dpp::co_number, "valor", "O valor do gasto (ex: 123.45).", true).set_min_value(0.01));
    cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "Observação adicional (opcional).", false));
    commands.push_back(cmd);
}