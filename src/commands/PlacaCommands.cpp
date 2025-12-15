#include "PlacaCommands.h"
#include "Utils.h"
#include "CommandHandler.h"
#include "DatabaseManager.h"
#include "EventHandler.h"
#include <variant>
#include <string>
#include <vector>
#include <algorithm>

PlacaCommands::PlacaCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler, EventHandler& eventHandler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler), eventHandler_(eventHandler) {
}

void PlacaCommands::handle_placa(const dpp::slashcommand_t& event) {
    event.thinking(dpp::m_ephemeral);

    std::string doutor = std::get<std::string>(event.get_parameter("doutor"));
    std::string tipo_placa = std::get<std::string>(event.get_parameter("tipo"));
    dpp::user solicitante = event.command.get_issuing_user();

    Placa nova_placa;
    nova_placa.id = Utils::gerar_codigo();
    nova_placa.doutor = doutor;
    nova_placa.tipo_placa = tipo_placa;
    nova_placa.solicitado_por_id = solicitante.id;
    nova_placa.solicitado_por_nome = solicitante.username;
    nova_placa.status = "pendente";

    auto finish = [this, event, nova_placa](const std::string& filename, const std::string& file_content) mutable {
        if (db_.addOrUpdatePlaca(nova_placa)) {
            dpp::message msg(config_.canal_solicitacao_placas, "");

            dpp::embed embed = Utils::criarEmbedPadrao("📢 Nova Placa Solicitada", "", dpp::colors::yellow);
            embed.add_field("Dr(a)", nova_placa.doutor, true)
                .add_field("Tipo", nova_placa.tipo_placa, true)
                .add_field("Solicitante", nova_placa.solicitado_por_nome, true)
                .set_footer(dpp::embed_footer().set_text("Cód: " + std::to_string(nova_placa.id)));

            if (!filename.empty() && !file_content.empty()) {
                msg.add_file(filename, file_content);
                embed.set_image("attachment://" + filename);
            }
            msg.add_embed(embed);
            bot_.message_create(msg);

            event.edit_response("✅ Solicitação enviada com sucesso!");
        }
        else { event.edit_response("❌ Erro ao salvar."); }
        };

    auto param_anexo = event.get_parameter("imagem_referencia");
    if (auto id_ptr = std::get_if<dpp::snowflake>(&param_anexo)) {
        dpp::attachment anexo = event.command.get_resolved_attachment(*id_ptr);
        std::string filename = "ref_" + std::to_string(nova_placa.id) + "_" + anexo.filename;
        std::string path = "./uploads/" + filename;

        Utils::BaixarAnexo(&bot_, anexo.url, path, [this, event, nova_placa, path, filename, finish](bool ok, std::string content) mutable {
            if (ok) {
                nova_placa.imagem_referencia_path = path;
                finish(filename, content);
            }
            else { event.edit_response("❌ Erro ao baixar imagem."); }
            });
    }
    else { finish("", ""); }
}

void PlacaCommands::handle_finalizar_placa(const dpp::slashcommand_t& event) {
    event.thinking(dpp::m_ephemeral);
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    dpp::attachment anexo = event.command.get_resolved_attachment(std::get<dpp::snowflake>(event.get_parameter("arte_final")));

    auto placa_opt = db_.getPlaca(codigo);
    if (!placa_opt) { event.edit_response("Placa não encontrada."); return; }
    Placa p = *placa_opt;

    std::string filename = "arte_" + std::to_string(p.id) + "_" + anexo.filename;
    std::string path = "./uploads/" + filename;

    Utils::BaixarAnexo(&bot_, anexo.url, path, [this, event, p, path, filename](bool ok, std::string content) mutable {
        if (!ok) { event.edit_response("Erro ao baixar arte."); return; }

        p.status = "finalizada";
        p.arte_final_path = path;
        db_.addOrUpdatePlaca(p);

        dpp::message msg(config_.canal_placas_finalizadas, "");
        dpp::embed embed = Utils::criarEmbedPadrao("✅ Placa Finalizada", "", dpp::colors::green);
        embed.add_field("Dr(a)", p.doutor, true);

        if (!content.empty()) {
            msg.add_file(filename, content);
            embed.set_image("attachment://" + filename);
        }

        msg.add_embed(embed);
        bot_.message_create(msg);
        event.edit_response("Placa finalizada!");
        });
}

void PlacaCommands::handle_lista_placas(const dpp::slashcommand_t& event) {
    const auto& map = db_.getPlacas();
    std::vector<PaginatedItem> items;
    for (const auto& [id, p] : map) {
        if (p.status == "pendente") items.push_back(p);
    }

    if (items.empty()) { event.reply(dpp::message("Nenhuma placa pendente.").set_flags(dpp::m_ephemeral)); return; }

    PaginationState state;
    state.channel_id = event.command.channel_id;
    state.items = std::move(items);
    state.originalUserID = event.command.get_issuing_user().id;
    state.listType = "placas";
    state.currentPage = 1;
    state.itemsPerPage = 5;

    dpp::embed firstPage = Utils::generatePageEmbed(state);

    event.reply(dpp::message(event.command.channel_id, firstPage), [this, state](const dpp::confirmation_callback_t& cb) {
        if (!cb.is_error()) {
        }
        });

    event.get_original_response([this, state](const auto& msg_cb) {
        if (!msg_cb.is_error()) {
            auto m = std::get<dpp::message>(msg_cb.value);
            eventHandler_.addPaginationState(m.id, state);

            bot_.message_add_reaction(m.id, m.channel_id, "🗑️", [this, m](auto cb) {
                if (!cb.is_error()) {
                    bot_.message_add_reaction(m.id, m.channel_id, "◀️", [this, m](auto cb2) {
                        if (!cb2.is_error()) bot_.message_add_reaction(m.id, m.channel_id, "▶️");
                        });
                }
                });

            bot_.start_timer([this, m](dpp::timer t) {
                bot_.message_delete(m.id, m.channel_id);
                bot_.stop_timer(t);
                }, 60);
        }
        });
}

void PlacaCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand p("placa", "Solicita placa.", bot_id);
    p.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome.", true));
    p.add_option(dpp::command_option(dpp::co_string, "tipo", "Tipo.", true).add_choice(dpp::command_option_choice("Porta", std::string("Porta"))).add_choice(dpp::command_option_choice("Armário", std::string("Armário"))));
    p.add_option(dpp::command_option(dpp::co_attachment, "imagem_referencia", "Ref.", false));
    commands.push_back(p);

    dpp::slashcommand f("finalizar_placa", "Finaliza placa.", bot_id);
    f.add_option(dpp::command_option(dpp::co_integer, "codigo", "Cód.", true));
    f.add_option(dpp::command_option(dpp::co_attachment, "arte_final", "Arte.", true));
    commands.push_back(f);

    dpp::slashcommand l("lista_placas", "Lista placas.", bot_id);
    commands.push_back(l);
}