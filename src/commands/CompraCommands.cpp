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
    nova_compra.categoria = std::get<std::string>(event.get_parameter("categoria"));
    nova_compra.unidade = std::get<std::string>(event.get_parameter("unidade"));

    auto obs_param = event.get_parameter("observacao");
    if (auto val_ptr = std::get_if<std::string>(&obs_param)) nova_compra.observacao = *val_ptr;
    else nova_compra.observacao = "Nenhuma.";

    nova_compra.registrado_por = event.command.get_issuing_user().username;
    nova_compra.data_registro = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateCompra(nova_compra)) {
        std::stringstream ss; ss << "R$ " << std::fixed << std::setprecision(2) << nova_compra.valor;

        dpp::embed embed = Utils::criarEmbedPadrao("💳 Novo Gasto Registrado", "", dpp::colors::light_sea_green);
        embed.add_field("Descrição", nova_compra.descricao, false)
            .add_field("Categoria", nova_compra.categoria, true)
            .add_field("Unidade", nova_compra.unidade, true)
            .add_field("Local", nova_compra.local_compra, true)
            .add_field("Valor", ss.str(), true)
            .add_field("Obs", nova_compra.observacao, false)
            .set_footer(dpp::embed_footer().set_text("Registrado por: " + nova_compra.registrado_por));

        cmdHandler_.replyAndDelete(event, dpp::message().add_embed(embed));
    }
    else {
        cmdHandler_.replyAndDelete(event, dpp::message("❌ Erro ao salvar."));
    }
}

void CompraCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand cmd("registrar_gasto", "Registra gasto.", bot_id);
    cmd.add_option(dpp::command_option(dpp::co_string, "descricao", "Desc.", true));
    cmd.add_option(dpp::command_option(dpp::co_string, "local", "Local.", true));
    cmd.add_option(dpp::command_option(dpp::co_number, "valor", "R$", true).set_min_value(0.01));
    cmd.add_option(dpp::command_option(dpp::co_string, "categoria", "Cat.", true).add_choice(dpp::command_option_choice("Café", "Café")).add_choice(dpp::command_option_choice("ASB", "ASB")).add_choice(dpp::command_option_choice("Faxina", "Faxina")).add_choice(dpp::command_option_choice("Papelaria", "Papelaria")).add_choice(dpp::command_option_choice("Outros", "Outros")));
    cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Local.", true).add_choice(dpp::command_option_choice("Tatuapé", "Tatuapé")).add_choice(dpp::command_option_choice("Campo Belo", "Campo Belo")).add_choice(dpp::command_option_choice("Ambas", "Ambas")));
    cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "Obs.", false));
    commands.push_back(cmd);
}