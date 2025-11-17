#include "EstoqueCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "ConfigManager.h"
#include "Utils.h"
#include <iostream>
#include <variant>
#include <string>
#include <algorithm>

EstoqueCommands::EstoqueCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler) {
}

std::string EstoqueCommands::formatarItemEstoque(const EstoqueItem& item) {
    return "**Item:** " + item.nome_item + "\n"
        + "**Quantidade:** " + std::to_string(item.quantidade) + " " + item.unidade + "\n"
        + "**Local:** " + item.local_estoque + "\n"
        + "**Última att:** " + item.data_ultima_att + " (por: " + item.atualizado_por_nome + ")\n"
        + "`ID: " + std::to_string(item.id) + "`\n";
}

void EstoqueCommands::handle_estoque_add(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    int64_t quantidade_int = std::get<int64_t>(event.get_parameter("quantidade"));
    int quantidade = static_cast<int>(quantidade_int);
    std::string unidade = std::get<std::string>(event.get_parameter("unidade"));
    std::string local = std::get<std::string>(event.get_parameter("local"));
    dpp::user usuario = event.command.get_issuing_user();

    EstoqueItem* item_existente = db_.getEstoqueItemPorNome(nome_item);
    std::string timestamp = Utils::format_timestamp(std::time(nullptr));

    dpp::embed embed;
    std::string resposta;

    if (item_existente) {
        // Atualiza item existente
        item_existente->quantidade += quantidade;
        item_existente->unidade = unidade;
        item_existente->local_estoque = local;
        item_existente->atualizado_por_nome = usuario.username;
        item_existente->data_ultima_att = timestamp;

        if (db_.addOrUpdateEstoqueItem(*item_existente)) {
            resposta = "✅ Item de estoque atualizado com sucesso!";
            embed = dpp::embed()
                .set_color(dpp::colors::green)
                .set_title("📦 Estoque Atualizado (Adicionado)")
                .add_field(item_existente->nome_item,
                    "**Nova Quantidade:** " + std::to_string(item_existente->quantidade) + " " + item_existente->unidade + "\n" +
                    "**Local:** " + item_existente->local_estoque, false)
                .set_footer(dpp::embed_footer().set_text("ID: " + std::to_string(item_existente->id) + " | " + timestamp));
        }
        else {
            event.reply(dpp::message("❌ Erro ao atualizar o item no banco de dados.").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    else {
        // Adiciona novo item
        EstoqueItem novo_item;
        novo_item.id = Utils::gerar_codigo();
        novo_item.nome_item = nome_item;
        novo_item.quantidade = quantidade;
        novo_item.unidade = unidade;
        novo_item.local_estoque = local;
        novo_item.atualizado_por_nome = usuario.username;
        novo_item.data_ultima_att = timestamp;

        if (db_.addOrUpdateEstoqueItem(novo_item)) {
            resposta = "✅ Novo item adicionado ao estoque com sucesso!";
            embed = dpp::embed()
                .set_color(dpp::colors::green)
                .set_title("📦 Novo Item no Estoque")
                .add_field(novo_item.nome_item,
                    "**Quantidade:** " + std::to_string(novo_item.quantidade) + " " + novo_item.unidade + "\n" +
                    "**Local:** " + novo_item.local_estoque, false)
                .set_footer(dpp::embed_footer().set_text("ID: " + std::to_string(novo_item.id) + " | " + timestamp));
        }
        else {
            event.reply(dpp::message("❌ Erro ao salvar o novo item no banco de dados.").set_flags(dpp::m_ephemeral));
            return;
        }
    }

    event.reply(dpp::message().add_embed(embed));
}

void EstoqueCommands::handle_estoque_remove(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    int64_t quantidade_int = std::get<int64_t>(event.get_parameter("quantidade"));
    int quantidade = static_cast<int>(quantidade_int);
    dpp::user usuario = event.command.get_issuing_user();
    std::string timestamp = Utils::format_timestamp(std::time(nullptr));

    EstoqueItem* item_existente = db_.getEstoqueItemPorNome(nome_item);

    if (!item_existente) {
        event.reply(dpp::message("❌ Item `" + nome_item + "` não encontrado no estoque.").set_flags(dpp::m_ephemeral));
        return;
    }

    if (item_existente->quantidade < quantidade) {
        event.reply(dpp::message("❌ Você não pode remover **" + std::to_string(quantidade) + "** " + item_existente->unidade +
            ". Só existem **" + std::to_string(item_existente->quantidade) + "** em estoque.").set_flags(dpp::m_ephemeral));
        return;
    }

    item_existente->quantidade -= quantidade;
    item_existente->atualizado_por_nome = usuario.username;
    item_existente->data_ultima_att = timestamp;

    if (db_.addOrUpdateEstoqueItem(*item_existente)) {
        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::orange)
            .set_title("📦 Estoque Atualizado (Removido)")
            .add_field(item_existente->nome_item,
                "**Nova Quantidade:** " + std::to_string(item_existente->quantidade) + " " + item_existente->unidade + "\n" +
                "**Local:** " + item_existente->local_estoque, false)
            .set_footer(dpp::embed_footer().set_text("ID: " + std::to_string(item_existente->id) + " | " + timestamp));

        event.reply(dpp::message().add_embed(embed));
    }
    else {
        event.reply(dpp::message("❌ Erro ao atualizar o item no banco de dados após a remoção.").set_flags(dpp::m_ephemeral));
    }
}

void EstoqueCommands::handle_estoque_lista(const dpp::slashcommand_t& event) {
    const auto& estoque_map = db_.getEstoque();
    if (estoque_map.empty()) {
        event.reply(dpp::message("ℹ️ O estoque está vazio no momento.").set_flags(dpp::m_ephemeral));
        return;
    }

    dpp::embed embed;
    embed.set_color(dpp::colors::blue)
        .set_title("📦 Itens Atuais no Estoque");

    std::string descricao_total;

    for (const auto& [id, item] : estoque_map) {
        if (item.quantidade > 0) { // Só lista itens que têm quantidade
            std::string linha = "**" + item.nome_item + ":** " +
                std::to_string(item.quantidade) + " " + item.unidade +
                " (" + item.local_estoque + ")\n";

            if (descricao_total.length() + linha.length() > 4000) { // Limite do embed
                embed.set_description(descricao_total);
                event.reply(dpp::message(event.command.channel_id, embed));

                descricao_total = "";
                embed = dpp::embed()
                    .set_color(dpp::colors::blue)
                    .set_title("📦 Itens Atuais no Estoque (Continuação)");
            }
            descricao_total += linha;
        }
    }

    if (descricao_total.empty()) {
        event.reply(dpp::message("ℹ️ Nenhum item com quantidade positiva encontrado no estoque.").set_flags(dpp::m_ephemeral));
        return;
    }

    embed.set_description(descricao_total);
    event.reply(dpp::message(event.command.channel_id, embed));
}


void EstoqueCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand add_cmd("estoque_add", "Adiciona (ou soma) um item ao estoque.", bot_id);
    add_cmd.add_option(dpp::command_option(dpp::co_string, "item", "O nome do item (ex: Resma A4).", true));
    add_cmd.add_option(dpp::command_option(dpp::co_integer, "quantidade", "Quantidade para adicionar.", true).set_min_value(1));
    add_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Tipo de unidade (ex: Unidades, Caixas, Pacotes).", true));
    add_cmd.add_option(dpp::command_option(dpp::co_string, "local", "Onde o item está (ex: Almoxarifado, Tatuapé).", true));
    commands.push_back(add_cmd);

    dpp::slashcommand remove_cmd("estoque_remove", "Remove uma quantidade de um item do estoque.", bot_id);
    remove_cmd.add_option(dpp::command_option(dpp::co_string, "item", "O nome do item (ex: Resma A4).", true));
    remove_cmd.add_option(dpp::command_option(dpp::co_integer, "quantidade", "Quantidade para remover.", true).set_min_value(1));
    commands.push_back(remove_cmd);

    dpp::slashcommand lista_cmd("estoque_lista", "Lista todos os itens no estoque.", bot_id);
    commands.push_back(lista_cmd);
}