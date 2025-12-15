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

void EstoqueCommands::handle_estoque_criar(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    std::string unidade = std::get<std::string>(event.get_parameter("unidade"));
    std::string local = std::get<std::string>(event.get_parameter("local"));
    std::string categoria = std::get<std::string>(event.get_parameter("categoria"));
    int quantidade_minima = static_cast<int>(std::get<int64_t>(event.get_parameter("minimo")));

    int quantidade_inicial = 0;
    auto qtd_param = event.get_parameter("inicial");
    if (std::holds_alternative<int64_t>(qtd_param)) quantidade_inicial = static_cast<int>(std::get<int64_t>(qtd_param));

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
        dpp::embed embed = Utils::criarEmbedPadrao("📦 Novo Item Cadastrado", "", dpp::colors::green);
        embed.add_field("Item", novo_item.nome_item, true)
            .add_field("Estoque Inicial", std::to_string(novo_item.quantidade) + " " + novo_item.unidade, true)
            .add_field("Mínimo", std::to_string(novo_item.quantidade_minima) + " " + novo_item.unidade, true)
            .add_field("Local", novo_item.local_estoque, true)
            .add_field("Categoria", novo_item.categoria, true);
        cmdHandler_.replyAndDelete(event, dpp::message().add_embed(embed));
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
        std::string desc = "**Adicionado:** +" + std::to_string(quantidade) + "\n**Novo Total:** " + std::to_string(item->quantidade) + " " + item->unidade;
        dpp::embed embed = Utils::criarEmbedPadrao("📦 Estoque Reposto", desc, dpp::colors::green);
        embed.add_field("Item", item->nome_item, true);
        cmdHandler_.replyAndDelete(event, dpp::message().add_embed(embed));
    }
    else { event.reply(dpp::message("❌ Erro ao atualizar.").set_flags(dpp::m_ephemeral)); }
}

void EstoqueCommands::handle_estoque_remove(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    int quantidade = static_cast<int>(std::get<int64_t>(event.get_parameter("quantidade")));

    EstoqueItem* item = db_.getEstoqueItemPorNome(nome_item);
    if (!item) { event.reply(dpp::message("❌ Item não encontrado.").set_flags(dpp::m_ephemeral)); return; }
    if (item->quantidade < quantidade) { event.reply(dpp::message("❌ Quantidade insuficiente no estoque.").set_flags(dpp::m_ephemeral)); return; }

    item->quantidade -= quantidade;
    item->atualizado_por_nome = event.command.get_issuing_user().username;
    item->data_ultima_att = Utils::format_timestamp(std::time(nullptr));

    if (db_.addOrUpdateEstoqueItem(*item)) {
        std::string desc = "**Baixa:** -" + std::to_string(quantidade) + "\n**Restante:** " + std::to_string(item->quantidade) + " " + item->unidade;
        uint32_t color = dpp::colors::orange;
        if (item->quantidade <= item->quantidade_minima) {
            desc += "\n⚠️ **ALERTA: Estoque Baixo!**";
            color = dpp::colors::red;
        }
        dpp::embed embed = Utils::criarEmbedPadrao("📦 Baixa de Estoque", desc, color);
        embed.add_field("Item", item->nome_item, true);
        cmdHandler_.replyAndDelete(event, dpp::message().add_embed(embed));
    }
    else { event.reply(dpp::message("❌ Erro ao atualizar.").set_flags(dpp::m_ephemeral)); }
}

void EstoqueCommands::handle_estoque_delete(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    EstoqueItem* item = db_.getEstoqueItemPorNome(nome_item);
    if (!item) { event.reply(dpp::message("❌ Item não encontrado.").set_flags(dpp::m_ephemeral)); return; }

    if (db_.removeEstoqueItem(item->id)) {
        cmdHandler_.replyAndDelete(event, dpp::message("🗑️ Item **" + nome_item + "** removido do sistema."));
    }
    else { event.reply(dpp::message("❌ Erro ao remover.").set_flags(dpp::m_ephemeral)); }
}

void EstoqueCommands::handle_estoque_modificar(const dpp::slashcommand_t& event) {
    std::string nome_item = std::get<std::string>(event.get_parameter("item"));
    EstoqueItem* item = db_.getEstoqueItemPorNome(nome_item);
    if (!item) { event.reply(dpp::message("❌ Item não encontrado.").set_flags(dpp::m_ephemeral)); return; }

    bool alterado = false;
    std::string mudancas_log = "";

    auto cat_param = event.get_parameter("categoria");
    if (auto val = std::get_if<std::string>(&cat_param)) { if (item->categoria != *val) { item->categoria = *val; mudancas_log += "• Categoria\n"; alterado = true; } }
    auto uni_param = event.get_parameter("unidade");
    if (auto val = std::get_if<std::string>(&uni_param)) { if (item->unidade != *val) { item->unidade = *val; mudancas_log += "• Unidade\n"; alterado = true; } }
    auto loc_param = event.get_parameter("local");
    if (auto val = std::get_if<std::string>(&loc_param)) { if (item->local_estoque != *val) { item->local_estoque = *val; mudancas_log += "• Local\n"; alterado = true; } }
    auto min_param = event.get_parameter("minimo");
    if (std::holds_alternative<int64_t>(min_param)) { int novo = (int)std::get<int64_t>(min_param); if (item->quantidade_minima != novo) { item->quantidade_minima = novo; mudancas_log += "• Mínimo\n"; alterado = true; } }
    auto nome_param = event.get_parameter("novo_nome");
    if (auto val = std::get_if<std::string>(&nome_param)) {
        if (*val != item->nome_item && !db_.getEstoqueItemPorNome(*val)) { item->nome_item = *val; mudancas_log += "• Nome\n"; alterado = true; }
    }

    if (alterado) {
        item->atualizado_por_nome = event.command.get_issuing_user().username;
        item->data_ultima_att = Utils::format_timestamp(std::time(nullptr));
        db_.addOrUpdateEstoqueItem(*item);
        dpp::embed embed = Utils::criarEmbedPadrao("✏️ Item Modificado", mudancas_log, dpp::colors::blue);
        embed.add_field("Item", item->nome_item, true);
        cmdHandler_.replyAndDelete(event, dpp::message().add_embed(embed));
    }
    else {
        event.reply(dpp::message("⚠️ Nenhuma alteração.").set_flags(dpp::m_ephemeral));
    }
}

void EstoqueCommands::handle_estoque_lista(const dpp::slashcommand_t& event) {
    const auto& estoque_map = db_.getEstoque();
    if (estoque_map.empty()) { event.reply(dpp::message("ℹ️ Estoque vazio.").set_flags(dpp::m_ephemeral)); return; }

    std::vector<PaginatedItem> items;
    for (const auto& [id, item] : estoque_map) items.push_back(item);
    std::sort(items.begin(), items.end(), [](auto& a, auto& b) { return std::get<EstoqueItem>(a).nome_item < std::get<EstoqueItem>(b).nome_item; });

    PaginationState state;
    state.channel_id = event.command.channel_id;
    state.items = std::move(items);
    state.originalUserID = event.command.get_issuing_user().id;
    state.listType = "estoque";
    state.currentPage = 1;
    state.itemsPerPage = 10;

    dpp::embed firstPage = Utils::generatePageEmbed(state);

    event.reply(dpp::message(event.command.channel_id, firstPage), [this, state](const dpp::confirmation_callback_t& cb) mutable {
        if (!cb.is_error()) {
        }
        });

    event.get_original_response([this, state](const dpp::confirmation_callback_t& msg_cb) {
        if (!msg_cb.is_error()) {
            auto m = std::get<dpp::message>(msg_cb.value);

            bot_.message_add_reaction(m.id, m.channel_id, "🗑️", [this, m, state](auto cb) {
                if (!cb.is_error()) {
                    if (state.items.size() > state.itemsPerPage) {
                        bot_.message_add_reaction(m.id, m.channel_id, "◀️", [this, m](auto cb2) {
                            if (!cb2.is_error()) bot_.message_add_reaction(m.id, m.channel_id, "▶️");
                            });
                    }
                }
                });

            bot_.start_timer([this, m](dpp::timer t) {
                bot_.message_delete(m.id, m.channel_id);
                bot_.stop_timer(t);
                }, 60);
        }
        });
}

void EstoqueCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand criar("estoque_criar", "Cadastra item.", bot_id);
    criar.add_option(dpp::command_option(dpp::co_string, "item", "Nome.", true));
    criar.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade.", true));
    criar.add_option(dpp::command_option(dpp::co_string, "local", "Local.", true));
    criar.add_option(dpp::command_option(dpp::co_string, "categoria", "Categoria.", true)
        .add_choice(dpp::command_option_choice("Café", std::string("Café")))
        .add_choice(dpp::command_option_choice("ASB", std::string("ASB")))
        .add_choice(dpp::command_option_choice("Faxina", std::string("Faxina")))
        .add_choice(dpp::command_option_choice("Papelaria", std::string("Papelaria")))
        .add_choice(dpp::command_option_choice("Outros", std::string("Outros")))
    );
    criar.add_option(dpp::command_option(dpp::co_integer, "minimo", "Qtd Mínima.", true));
    criar.add_option(dpp::command_option(dpp::co_integer, "inicial", "Qtd Inicial.", false));
    commands.push_back(criar);

    dpp::slashcommand repor("estoque_repor", "Adiciona quantidade.", bot_id);
    repor.add_option(dpp::command_option(dpp::co_string, "item", "Nome.", true));
    repor.add_option(dpp::command_option(dpp::co_integer, "quantidade", "Qtd.", true));
    commands.push_back(repor);

    dpp::slashcommand baixa("estoque_baixa", "Remove quantidade (uso/perda).", bot_id);
    baixa.add_option(dpp::command_option(dpp::co_string, "item", "Nome.", true));
    baixa.add_option(dpp::command_option(dpp::co_integer, "quantidade", "Qtd.", true));
    commands.push_back(baixa);

    dpp::slashcommand mod("estoque_modificar", "Altera cadastro.", bot_id);
    mod.add_option(dpp::command_option(dpp::co_string, "item", "Item atual.", true));
    mod.add_option(dpp::command_option(dpp::co_string, "novo_nome", "Renomear.", false));
    mod.add_option(dpp::command_option(dpp::co_integer, "minimo", "Novo mínimo.", false));
    mod.add_option(dpp::command_option(dpp::co_string, "categoria", "Nova categoria.", false).add_choice(dpp::command_option_choice("Café", std::string("Café"))).add_choice(dpp::command_option_choice("ASB", std::string("ASB"))).add_choice(dpp::command_option_choice("Faxina", std::string("Faxina"))).add_choice(dpp::command_option_choice("Papelaria", std::string("Papelaria"))).add_choice(dpp::command_option_choice("Outros", std::string("Outros"))));
    mod.add_option(dpp::command_option(dpp::co_string, "unidade", "Nova unidade.", false));
    mod.add_option(dpp::command_option(dpp::co_string, "local", "Novo local.", false));
    commands.push_back(mod);

    dpp::slashcommand del("estoque_delete", "Apaga item do sistema.", bot_id);
    del.add_option(dpp::command_option(dpp::co_string, "item", "Nome.", true));
    commands.push_back(del);

    dpp::slashcommand lista("estoque_lista", "Ver estoque.", bot_id);
    commands.push_back(lista);
}