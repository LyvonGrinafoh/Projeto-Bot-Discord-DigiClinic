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
        + "**Categoria:** " + item.categoria + "\n"
        + "**Qtd:** " + std::to_string(item.quantidade) + " " + item.unidade + "\n"
        + "**Mínimo:** " + std::to_string(item.quantidade_minima) + "\n"
        + "**Local:** " + item.local_estoque + "\n"
        + "**Última att:** " + item.data_ultima_att + " (" + item.atualizado_por_nome + ")\n";
}

void EstoqueCommands::handle_estoque_criar(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    std::string unidade = std::get<std::string>(event.get_parameter("unidade"));
    std::string local = std::get<std::string>(event.get_parameter("local"));
    std::string categoria = std::get<std::string>(event.get_parameter("categoria"));
    int quantidade_minima = static_cast<int>(std::get<int64_t>(event.get_parameter("minimo")));

    int quantidade_inicial = 0;
    auto qtd_param = event.get_parameter("inicial");
    if (std::holds_alternative<int64_t>(qtd_param)) {
        quantidade_inicial = static_cast<int>(std::get<int64_t>(qtd_param));
    }

    if (db_.getEstoqueItemPorNome(nome_item)) {
        event.reply(dpp::message("❌ Já existe um item com esse nome.").set_flags(dpp::m_ephemeral));
        return;
    }

    EstoqueItem novo_item;
    novo_item.id = Utils::gerar_codigo();
    novo_item.nome_item = nome_item;
    novo_item.unidade = unidade;
    novo_item.local_estoque = local;
    novo_item.quantidade_minima = quantidade_minima;
    novo_item.quantidade = quantidade_inicial;
    novo_item.categoria = categoria;
    novo_item.atualizado_por_nome = event.command.get_issuing_user().username;
    novo_item.data_ultima_att = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateEstoqueItem(novo_item)) {
        dpp::embed embed = dpp::embed().set_color(dpp::colors::green).set_title("📦 Novo Item Cadastrado").add_field("Item", novo_item.nome_item, true).add_field("Estoque Inicial", std::to_string(novo_item.quantidade) + " " + novo_item.unidade, true).add_field("Estoque Mínimo", std::to_string(novo_item.quantidade_minima) + " " + novo_item.unidade, true).add_field("Categoria", novo_item.categoria, true).add_field("Local", novo_item.local_estoque, true);
        event.reply(dpp::message().add_embed(embed));
    }
    else { event.reply(dpp::message("❌ Erro ao salvar.").set_flags(dpp::m_ephemeral)); }
}

void EstoqueCommands::handle_estoque_add(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    int quantidade = static_cast<int>(std::get<int64_t>(event.get_parameter("quantidade")));

    EstoqueItem* item = db_.getEstoqueItemPorNome(nome_item);
    if (!item) { event.reply(dpp::message("❌ Item não encontrado.").set_flags(dpp::m_ephemeral)); return; }

    item->quantidade += quantidade;
    item->atualizado_por_nome = event.command.get_issuing_user().username;
    item->data_ultima_att = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateEstoqueItem(*item)) {
        dpp::embed embed = dpp::embed().set_color(dpp::colors::green).set_title("📦 Estoque Reposto").add_field(item->nome_item, "**Adicionado:** +" + std::to_string(quantidade) + "\n**Total:** " + std::to_string(item->quantidade) + " " + item->unidade, false);
        event.reply(dpp::message().add_embed(embed));
    }
    else { event.reply(dpp::message("❌ Erro ao atualizar.").set_flags(dpp::m_ephemeral)); }
}

void EstoqueCommands::handle_estoque_remove(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    int quantidade = static_cast<int>(std::get<int64_t>(event.get_parameter("quantidade")));

    EstoqueItem* item = db_.getEstoqueItemPorNome(nome_item);
    if (!item) { event.reply(dpp::message("❌ Item não encontrado.").set_flags(dpp::m_ephemeral)); return; }
    if (item->quantidade < quantidade) { event.reply(dpp::message("❌ Quantidade insuficiente.").set_flags(dpp::m_ephemeral)); return; }

    item->quantidade -= quantidade;
    item->atualizado_por_nome = event.command.get_issuing_user().username;
    item->data_ultima_att = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateEstoqueItem(*item)) {
        dpp::embed embed = dpp::embed().set_color(dpp::colors::orange).set_title("📦 Saída de Estoque").add_field(item->nome_item, "**Removido:** -" + std::to_string(quantidade) + "\n**Restante:** " + std::to_string(item->quantidade) + " " + item->unidade, false);
        if (item->quantidade <= item->quantidade_minima) { embed.set_color(dpp::colors::red).add_field("⚠️ ALERTA", "Estoque abaixo do mínimo!", false); }
        event.reply(dpp::message().add_embed(embed));
    }
    else { event.reply(dpp::message("❌ Erro ao atualizar.").set_flags(dpp::m_ephemeral)); }
}

void EstoqueCommands::handle_estoque_delete(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    EstoqueItem* item = db_.getEstoqueItemPorNome(nome_item);

    if (!item) { event.reply(dpp::message("❌ Item não encontrado.").set_flags(dpp::m_ephemeral)); return; }

    if (db_.removeEstoqueItem(item->id)) {
        event.reply(dpp::message("🗑️ Item **" + nome_item + "** foi removido permanentemente do sistema."));
    }
    else {
        event.reply(dpp::message("❌ Erro ao remover o item.").set_flags(dpp::m_ephemeral));
    }
}

void EstoqueCommands::handle_estoque_lista(const dpp::slashcommand_t& event) {
    const auto& estoque_map = db_.getEstoque();
    if (estoque_map.empty()) { event.reply(dpp::message("ℹ️ Estoque vazio.").set_flags(dpp::m_ephemeral)); return; }

    dpp::embed embed;
    embed.set_color(dpp::colors::blue).set_title("📦 Relatório de Estoque");
    std::string descricao;

    std::vector<EstoqueItem> itens;
    for (const auto& [id, item] : estoque_map) itens.push_back(item);
    std::sort(itens.begin(), itens.end(), [](auto& a, auto& b) { return a.nome_item < b.nome_item; });

    for (const auto& item : itens) {
        std::string icon = (item.quantidade <= item.quantidade_minima) ? "🔴" : "✅";
        descricao += icon + " **" + item.nome_item + "** (" + item.categoria + "): " + std::to_string(item.quantidade) + " " + item.unidade + " (Min: " + std::to_string(item.quantidade_minima) + ")\n";
    }

    if (descricao.length() > 4000) descricao = descricao.substr(0, 4000) + "...";
    embed.set_description(descricao);
    event.reply(dpp::message(event.command.channel_id, embed));
}

void EstoqueCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand criar_cmd("estoque_criar", "Cadastra item.", bot_id);
    criar_cmd.add_option(dpp::command_option(dpp::co_string, "item", "Nome.", true));
    criar_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade.", true));
    criar_cmd.add_option(dpp::command_option(dpp::co_string, "local", "Local.", true));
    criar_cmd.add_option(dpp::command_option(dpp::co_string, "categoria", "Categoria.", true)
        .add_choice(dpp::command_option_choice("Café", std::string("Café")))
        .add_choice(dpp::command_option_choice("ASB", std::string("ASB")))
        .add_choice(dpp::command_option_choice("Faxina", std::string("Faxina")))
        .add_choice(dpp::command_option_choice("Papelaria", std::string("Papelaria")))
        .add_choice(dpp::command_option_choice("Outros", std::string("Outros")))
    );
    criar_cmd.add_option(dpp::command_option(dpp::co_integer, "minimo", "Mínimo.", true));
    criar_cmd.add_option(dpp::command_option(dpp::co_integer, "inicial", "Qtd inicial.", false));
    commands.push_back(criar_cmd);

    dpp::slashcommand add_cmd("estoque_add", "Adiciona qtd.", bot_id);
    add_cmd.add_option(dpp::command_option(dpp::co_string, "item", "Nome.", true));
    add_cmd.add_option(dpp::command_option(dpp::co_integer, "quantidade", "Qtd.", true));
    commands.push_back(add_cmd);

    dpp::slashcommand remove_cmd("estoque_remove", "Remove qtd.", bot_id);
    remove_cmd.add_option(dpp::command_option(dpp::co_string, "item", "Nome.", true));
    remove_cmd.add_option(dpp::command_option(dpp::co_integer, "quantidade", "Qtd.", true));
    commands.push_back(remove_cmd);

    dpp::slashcommand delete_cmd("estoque_delete", "Deleta item permanentemente.", bot_id);
    delete_cmd.add_option(dpp::command_option(dpp::co_string, "item", "Nome do item a deletar.", true));
    commands.push_back(delete_cmd);

    dpp::slashcommand lista_cmd("estoque_lista", "Lista estoque.", bot_id);
    commands.push_back(lista_cmd);
}