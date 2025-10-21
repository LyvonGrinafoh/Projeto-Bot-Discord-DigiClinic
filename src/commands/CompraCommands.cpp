#include "CompraCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include <iostream>
#include <variant>

CompraCommands::CompraCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler){
}

void CompraCommands::handle_adicionar_compra(const dpp::slashcommand_t& event) {
    Compra nova_compra;
    nova_compra.id = Utils::gerar_codigo();
    nova_compra.item = std::get<std::string>(event.get_parameter("item"));
    nova_compra.local_compra = std::get<std::string>(event.get_parameter("local"));
    nova_compra.unidade_destino = std::get<std::string>(event.get_parameter("unidade"));

    auto qtd_param = event.get_parameter("quantidade");
    if (auto val_ptr = std::get_if<int64_t>(&qtd_param)) {
        nova_compra.quantidade = static_cast<int>(*val_ptr);
        if (nova_compra.quantidade < 1) nova_compra.quantidade = 1;
    }
    else {
        nova_compra.quantidade = 0;
    }

    auto obs_param = event.get_parameter("observacao");
    if (auto val_ptr = std::get_if<std::string>(&obs_param)) {
        nova_compra.observacao = *val_ptr;
    }
    else {
        nova_compra.observacao = "Nenhuma.";
    }

    nova_compra.solicitado_por = event.command.get_issuing_user().username;
    nova_compra.data_solicitacao = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateCompra(nova_compra)) {
        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::light_sea_green)
            .set_title("🛒 Item Adicionado à Lista de Compras")
            .add_field("Item", nova_compra.item, false)
            .add_field("Local Sugerido", nova_compra.local_compra, true)
            .add_field("Unidade Destino", nova_compra.unidade_destino, true);

        if (nova_compra.quantidade > 0) {
            embed.add_field("Quantidade", std::to_string(nova_compra.quantidade), true);
        }
        else {
            embed.add_field("Quantidade", "N/A", true);
        }

        embed.add_field("Observação", nova_compra.observacao, false)
            .set_footer(dpp::embed_footer().set_text("Adicionado por: " + nova_compra.solicitado_por));

        event.reply(dpp::message().add_embed(embed));
    }
    else {
        event.reply(dpp::message("❌ Erro ao salvar o item de compra no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void CompraCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand adicionar_compra_cmd("adicionar_compra", "Adiciona um item à lista de compras.", bot_id);
    adicionar_compra_cmd.add_option(dpp::command_option(dpp::co_string, "item", "O que precisa ser comprado.", true));
    adicionar_compra_cmd.add_option(
        dpp::command_option(dpp::co_string, "local", "Onde o item deve ser comprado.", true)
        .add_choice(dpp::command_option_choice("Papelaria", std::string("Papelaria")))
        .add_choice(dpp::command_option_choice("Mercado", std::string("Mercado")))
        .add_choice(dpp::command_option_choice("Dental Cremer", std::string("Dental Cremer")))
        .add_choice(dpp::command_option_choice("Outro", std::string("Outro")))
    );
    adicionar_compra_cmd.add_option(
        dpp::command_option(dpp::co_string, "unidade", "Para qual unidade o item se destina.", true)
        .add_choice(dpp::command_option_choice("Tatuapé", std::string("Tatuapé")))
        .add_choice(dpp::command_option_choice("Campo Belo", std::string("Campo Belo")))
        .add_choice(dpp::command_option_choice("Ambas", std::string("Ambas")))
    );
    adicionar_compra_cmd.add_option(dpp::command_option(dpp::co_integer, "quantidade", "Quantas unidades (opcional, mínimo 1).", false).set_min_value(1));
    adicionar_compra_cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "Alguma observação sobre o item (marca, etc).", false));
    commands.push_back(adicionar_compra_cmd);
}