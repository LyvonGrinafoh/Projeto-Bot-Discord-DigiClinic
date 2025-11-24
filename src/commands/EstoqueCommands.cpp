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
        + "**Qtd:** " + std::to_string(item.quantidade) + " " + item.unidade + "\n"
        + "**Mínimo:** " + std::to_string(item.quantidade_minima) + "\n"
        + "**Local:** " + item.local_estoque + "\n"
        + "**Última att:** " + item.data_ultima_att + " (" + item.atualizado_por_nome + ")\n";
}

void EstoqueCommands::handle_estoque_criar(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    std::string unidade = std::get<std::string>(event.get_parameter("unidade"));
    std::string local = std::get<std::string>(event.get_parameter("local"));

    int64_t min_int = std::get<int64_t>(event.get_parameter("minimo"));
    int quantidade_minima = static_cast<int>(min_int);

    int quantidade_inicial = 0;
    auto qtd_param = event.get_parameter("inicial");
    if (std::holds_alternative<int64_t>(qtd_param)) {
        quantidade_inicial = static_cast<int>(std::get<int64_t>(qtd_param));
    }

    dpp::user usuario = event.command.get_issuing_user();
    EstoqueItem* item_existente = db_.getEstoqueItemPorNome(nome_item);

    if (item_existente) {
        event.reply(dpp::message("❌ Já existe um item com o nome **" + item_existente->nome_item + "** no estoque. Use `/estoque_add` para adicionar quantidade ou edite o existente.").set_flags(dpp::m_ephemeral));
        return;
    }

    EstoqueItem novo_item;
    novo_item.id = Utils::gerar_codigo();
    novo_item.nome_item = nome_item;
    novo_item.unidade = unidade;
    novo_item.local_estoque = local;
    novo_item.quantidade_minima = quantidade_minima;
    novo_item.quantidade = quantidade_inicial;
    novo_item.atualizado_por_nome = usuario.username;
    novo_item.data_ultima_att = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateEstoqueItem(novo_item)) {
        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::green)
            .set_title("📦 Novo Item Cadastrado")
            .add_field("Item", novo_item.nome_item, true)
            .add_field("Estoque Inicial", std::to_string(novo_item.quantidade) + " " + novo_item.unidade, true)
            .add_field("Estoque Mínimo", std::to_string(novo_item.quantidade_minima) + " " + novo_item.unidade, true)
            .add_field("Local", novo_item.local_estoque, true)
            .set_footer(dpp::embed_footer().set_text("Cadastrado por: " + usuario.username));

        event.reply(dpp::message().add_embed(embed));
    }
    else {
        event.reply(dpp::message("❌ Erro ao salvar o novo item no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void EstoqueCommands::handle_estoque_add(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    int64_t quantidade_int = std::get<int64_t>(event.get_parameter("quantidade"));
    int quantidade = static_cast<int>(quantidade_int);
    dpp::user usuario = event.command.get_issuing_user();

    EstoqueItem* item_existente = db_.getEstoqueItemPorNome(nome_item);

    if (!item_existente) {
        event.reply(dpp::message("❌ Item `" + nome_item + "` não encontrado.\nUse `/estoque_criar` para cadastrar um novo item primeiro.").set_flags(dpp::m_ephemeral));
        return;
    }

    item_existente->quantidade += quantidade;
    item_existente->atualizado_por_nome = usuario.username;
    item_existente->data_ultima_att = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateEstoqueItem(*item_existente)) {
        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::green)
            .set_title("📦 Estoque Reposto")
            .add_field(item_existente->nome_item,
                "**Adicionado:** +" + std::to_string(quantidade) + "\n" +
                "**Total Atual:** " + std::to_string(item_existente->quantidade) + " " + item_existente->unidade, false)
            .set_footer(dpp::embed_footer().set_text("Atualizado por: " + usuario.username));

        event.reply(dpp::message().add_embed(embed));
    }
    else {
        event.reply(dpp::message("❌ Erro ao atualizar o item no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void EstoqueCommands::handle_estoque_remove(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    int64_t quantidade_int = std::get<int64_t>(event.get_parameter("quantidade"));
    int quantidade = static_cast<int>(quantidade_int);
    dpp::user usuario = event.command.get_issuing_user();

    EstoqueItem* item_existente = db_.getEstoqueItemPorNome(nome_item);

    if (!item_existente) {
        event.reply(dpp::message("❌ Item `" + nome_item + "` não encontrado.").set_flags(dpp::m_ephemeral));
        return;
    }

    if (item_existente->quantidade < quantidade) {
        event.reply(dpp::message("❌ Quantidade insuficiente. Só existem **" + std::to_string(item_existente->quantidade) + "** em estoque.").set_flags(dpp::m_ephemeral));
        return;
    }

    item_existente->quantidade -= quantidade;
    item_existente->atualizado_por_nome = usuario.username;
    item_existente->data_ultima_att = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateEstoqueItem(*item_existente)) {
        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::orange)
            .set_title("📦 Saída de Estoque")
            .add_field(item_existente->nome_item,
                "**Removido:** -" + std::to_string(quantidade) + "\n" +
                "**Restante:** " + std::to_string(item_existente->quantidade) + " " + item_existente->unidade, false);

        if (item_existente->quantidade <= item_existente->quantidade_minima) {
            embed.set_color(dpp::colors::red);
            embed.add_field("⚠️ ALERTA DE ESTOQUE BAIXO",
                "A quantidade atual (" + std::to_string(item_existente->quantidade) + ") atingiu ou está abaixo do mínimo (" + std::to_string(item_existente->quantidade_minima) + ").\n**Hora de comprar mais!**", false);
        }

        embed.set_footer(dpp::embed_footer().set_text("Atualizado por: " + usuario.username));
        event.reply(dpp::message().add_embed(embed));
    }
    else {
        event.reply(dpp::message("❌ Erro ao atualizar o item no banco de dados.").set_flags(dpp::m_ephemeral));
    }
}

void EstoqueCommands::handle_estoque_lista(const dpp::slashcommand_t& event) {
    const auto& estoque_map = db_.getEstoque();
    if (estoque_map.empty()) {
        event.reply(dpp::message("ℹ️ O estoque está vazio.").set_flags(dpp::m_ephemeral));
        return;
    }

    dpp::embed embed;
    embed.set_color(dpp::colors::blue)
        .set_title("📦 Relatório de Estoque");

    std::string descricao_total;
    bool tem_alertas = false;

    std::vector<EstoqueItem> itens_ordenados;
    for (const auto& [id, item] : estoque_map) {
        itens_ordenados.push_back(item);
    }
    std::sort(itens_ordenados.begin(), itens_ordenados.end(), [](const EstoqueItem& a, const EstoqueItem& b) {
        return a.nome_item < b.nome_item;
        });

    for (const auto& item : itens_ordenados) {
        std::string status_icon = "✅";
        if (item.quantidade <= item.quantidade_minima) {
            status_icon = "🔴";
            tem_alertas = true;
        }
        else if (item.quantidade <= item.quantidade_minima * 1.2) {
            status_icon = "⚠️";
        }

        std::string linha = status_icon + " **" + item.nome_item + ":** " +
            std::to_string(item.quantidade) + " " + item.unidade +
            " (Min: " + std::to_string(item.quantidade_minima) + ")\n";

        if (descricao_total.length() + linha.length() > 4000) {
            embed.set_description(descricao_total);
            event.reply(dpp::message(event.command.channel_id, embed));
            descricao_total = "";
            embed = dpp::embed().set_color(dpp::colors::blue).set_title("📦 Estoque (Continuação)");
        }
        descricao_total += linha;
    }

    if (tem_alertas) {
        embed.set_footer(dpp::embed_footer().set_text("🔴 = Abaixo do mínimo | ⚠️ = Próximo do mínimo"));
    }

    if (!descricao_total.empty()) {
        embed.set_description(descricao_total);
        event.reply(dpp::message(event.command.channel_id, embed));
    }
}


void EstoqueCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand criar_cmd("estoque_criar", "Cadastra um novo item no sistema de estoque.", bot_id);
    criar_cmd.add_option(dpp::command_option(dpp::co_string, "item", "Nome do item.", true));
    criar_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade (Cx, Un, Pct).", true));
    criar_cmd.add_option(dpp::command_option(dpp::co_string, "local", "Local de armazenamento.", true));
    criar_cmd.add_option(dpp::command_option(dpp::co_integer, "minimo", "Quantidade mínima (alerta de compra).", true));
    criar_cmd.add_option(dpp::command_option(dpp::co_integer, "inicial", "Quantidade inicial atual (opcional, padrão 0).", false));
    commands.push_back(criar_cmd);

    dpp::slashcommand add_cmd("estoque_add", "Adiciona quantidade a um item JÁ EXISTENTE.", bot_id);
    add_cmd.add_option(dpp::command_option(dpp::co_string, "item", "Nome do item.", true));
    add_cmd.add_option(dpp::command_option(dpp::co_integer, "quantidade", "Quantidade a adicionar.", true).set_min_value(1));
    commands.push_back(add_cmd);

    dpp::slashcommand remove_cmd("estoque_remove", "Remove quantidade (consumo) do estoque.", bot_id);
    remove_cmd.add_option(dpp::command_option(dpp::co_string, "item", "Nome do item.", true));
    remove_cmd.add_option(dpp::command_option(dpp::co_integer, "quantidade", "Quantidade a remover.", true).set_min_value(1));
    commands.push_back(remove_cmd);

    dpp::slashcommand lista_cmd("estoque_lista", "Lista todo o estoque e alerta itens baixos.", bot_id);
    commands.push_back(lista_cmd);
}