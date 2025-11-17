#include "PlacaCommands.h"
#include "Utils.h"
#include "CommandHandler.h"
#include "DatabaseManager.h"
#include "EventHandler.h"
#include <variant>
#include <string>
#include <vector>
#include <algorithm>

dpp::embed generatePageEmbed(const PaginationState& state);

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

    auto finish_placa_creation = [this, event, nova_placa, solicitante](const std::string& anexo_filename_for_embed) mutable {
        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::orange)
            .set_title("📢 Nova Solicitação de Placa")
            .add_field("Código", "`" + std::to_string(nova_placa.id) + "`", false)
            .add_field("Profissional", nova_placa.doutor, true)
            .add_field("Tipo", nova_placa.tipo_placa, true)
            .add_field("Solicitado por", solicitante.get_mention(), true)
            .set_footer(dpp::embed_footer().set_text("Status: Pendente"));

        dpp::message msg;
        msg.channel_id = config_.canal_solicitacao_placas;

        if (!nova_placa.imagem_referencia_path.empty()) {
            try {
                std::string file_content = dpp::utility::read_file(nova_placa.imagem_referencia_path);
                msg.add_file(anexo_filename_for_embed, file_content);
                embed.set_image("attachment://" + anexo_filename_for_embed);
            }
            catch (const dpp::exception& e) {
                Utils::log_to_file("Erro ao ler arquivo de imagem (placa ref): " + std::string(e.what()));
            }
        }

        msg.add_embed(embed);

        if (db_.addOrUpdatePlaca(nova_placa)) {
            bot_.message_create(msg);
            event.edit_response(dpp::message("✅ Solicitação de placa para **" + nova_placa.doutor + "** (" + nova_placa.tipo_placa + ") enviada! Código: `" + std::to_string(nova_placa.id) + "`"));
        }
        else {
            event.edit_response(dpp::message("❌ Erro ao salvar a solicitação de placa no banco de dados.").set_flags(dpp::m_ephemeral));
        }
        };

    auto param_anexo = event.get_parameter("imagem_referencia");
    if (auto attachment_id_ptr = std::get_if<dpp::snowflake>(&param_anexo)) {
        dpp::attachment anexo = event.command.get_resolved_attachment(*attachment_id_ptr);
        if (anexo.content_type.find("image/") == 0) {

            std::string anexo_filename = "ref_" + std::to_string(nova_placa.id) + "_" + anexo.filename;
            std::string caminho_salvar = "./uploads/" + anexo_filename;

            Utils::BaixarAnexo(&bot_, anexo.url, caminho_salvar,
                [this, event, nova_placa, caminho_salvar, anexo_filename, finish_placa_creation](bool sucesso) mutable {
                    if (sucesso) {
                        nova_placa.imagem_referencia_path = caminho_salvar;
                        finish_placa_creation(anexo_filename);
                    }
                    else {
                        event.edit_response(dpp::message("❌ Erro ao baixar a imagem de referência. A solicitação não foi salva.").set_flags(dpp::m_ephemeral));
                    }
                });
        }
        else {
            finish_placa_creation("");
        }
    }
    else {
        finish_placa_creation("");
    }
}

void PlacaCommands::handle_finalizar_placa(const dpp::slashcommand_t& event) {

    event.thinking(dpp::m_ephemeral);

    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo"));
    uint64_t codigo = static_cast<uint64_t>(codigo_int);
    dpp::snowflake anexo_id = std::get<dpp::snowflake>(event.get_parameter("arte_final"));
    dpp::attachment anexo = event.command.get_resolved_attachment(anexo_id);

    if (anexo.content_type.find("image/") != 0) {
        event.edit_response(dpp::message("❌ O anexo 'arte_final' precisa ser uma imagem.").set_flags(dpp::m_ephemeral));
        return;
    }

    Placa* placa_ptr = db_.getPlacaPtr(codigo);

    if (!placa_ptr) {
        event.edit_response(dpp::message("❌ Código de solicitação de placa não encontrado.").set_flags(dpp::m_ephemeral));
        return;
    }

    Placa& placa = *placa_ptr;

    if (placa.status != "pendente") {
        event.edit_response(dpp::message("⚠️ Esta solicitação de placa não está pendente (Status: " + placa.status + ").").set_flags(dpp::m_ephemeral));
        return;
    }

    std::string anexo_filename = "artefinal_" + std::to_string(placa.id) + "_" + anexo.filename;
    std::string caminho_salvar = "./uploads/" + anexo_filename;

    Utils::BaixarAnexo(&bot_, anexo.url, caminho_salvar,
        [this, event, placa, caminho_salvar, anexo_filename](bool sucesso) mutable {
            if (!sucesso) {
                event.edit_response(dpp::message("❌ Erro ao baixar a imagem da arte final.").set_flags(dpp::m_ephemeral));
                return;
            }

            placa.status = "finalizada";
            placa.arte_final_path = caminho_salvar;

            if (db_.addOrUpdatePlaca(placa)) {
                dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::green)
                    .set_title("✅ Placa Finalizada")
                    .add_field("Código", "`" + std::to_string(placa.id) + "`", false)
                    .add_field("Profissional", placa.doutor, true)
                    .add_field("Tipo", placa.tipo_placa, true)
                    .add_field("Solicitado por", placa.solicitado_por_nome, true)
                    .set_footer(dpp::embed_footer().set_text("Arte criada por: " + event.command.get_issuing_user().username));

                dpp::message msg;
                msg.channel_id = config_.canal_placas_finalizadas;

                try {
                    std::string file_content = dpp::utility::read_file(caminho_salvar);
                    msg.add_file(anexo_filename, file_content);
                    embed.set_image("attachment://" + anexo_filename);
                }
                catch (const dpp::exception& e) {
                    Utils::log_to_file("Erro ao ler arquivo de imagem (placa final): " + std::string(e.what()));
                }

                msg.add_embed(embed);
                bot_.message_create(msg);

                event.edit_response(dpp::message("🎉 Arte da placa para **" + placa.doutor + "** (Cód: `" + std::to_string(placa.id) + "`) foi enviada com sucesso!"));
            }
            else {
                event.edit_response(dpp::message("❌ Erro ao atualizar a placa no banco de dados.").set_flags(dpp::m_ephemeral));
            }
        });
}

void PlacaCommands::handle_lista_placas(const dpp::slashcommand_t& event) {
    std::string status_filtro = "pendente";
    auto param = event.get_parameter("status");
    if (auto val_ptr = std::get_if<std::string>(&param)) {
        status_filtro = *val_ptr;
    }

    const auto& placas_map = db_.getPlacas();
    std::vector<PaginatedItem> filtered_items;

    for (const auto& [id, placa] : placas_map) {
        if (status_filtro == "todas") {
            filtered_items.push_back(placa);
        }
        else if (placa.status == status_filtro) {
            filtered_items.push_back(placa);
        }
    }

    if (filtered_items.empty()) {
        event.reply(dpp::message("Nenhuma placa encontrada com o status `" + status_filtro + "`.").set_flags(dpp::m_ephemeral));
        return;
    }

    std::sort(filtered_items.begin(), filtered_items.end(), [](const PaginatedItem& a, const PaginatedItem& b) {
        if (std::holds_alternative<Placa>(a) && std::holds_alternative<Placa>(b)) {
            return std::get<Placa>(a).id < std::get<Placa>(b).id;
        }
        return false;
        });

    PaginationState state;
    state.channel_id = event.command.channel_id;
    state.items = std::move(filtered_items);
    state.originalUserID = event.command.get_issuing_user().id;
    state.listType = "placas";
    state.currentPage = 1;
    state.itemsPerPage = 5;

    dpp::embed firstPageEmbed = generatePageEmbed(state);
    bool needsPagination = state.items.size() > state.itemsPerPage;

    event.reply(dpp::message(event.command.channel_id, firstPageEmbed),
        [this, state, needsPagination, event](const dpp::confirmation_callback_t& cb) mutable {
            if (!cb.is_error()) {
                if (needsPagination) {
                    event.get_original_response([this, state](const dpp::confirmation_callback_t& msg_cb) mutable {
                        if (!msg_cb.is_error()) {
                            auto* msg_ptr = std::get_if<dpp::message>(&msg_cb.value);
                            if (msg_ptr) {
                                auto& msg = *msg_ptr;
                                eventHandler_.addPaginationState(msg.id, std::move(state));
                                bot_.message_add_reaction(msg.id, msg.channel_id, "◀️", [this, msg](const dpp::confirmation_callback_t& reaction_cb) {
                                    if (!reaction_cb.is_error()) {
                                        bot_.message_add_reaction(msg.id, msg.channel_id, "▶️");
                                    }
                                    });
                            }
                            else { Utils::log_to_file("Erro: get_original_response não retornou um dpp::message."); }
                        }
                        else { Utils::log_to_file("Erro ao *buscar* a resposta original: " + msg_cb.get_error().message); }
                        });
                }
            }
            else {
                Utils::log_to_file("Erro ao *enviar* a resposta inicial paginada para /lista_placas: " + cb.get_error().message);
            }
        });
}


void PlacaCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand placa_cmd("placa", "Solicita a criação de uma placa para um profissional.", bot_id);
    placa_cmd.add_option(dpp::command_option(dpp::co_string, "doutor", "Nome do profissional para a placa.", true));

    placa_cmd.add_option(
        dpp::command_option(dpp::co_string, "tipo", "O tipo de placa (porta ou armário).", true)
        .add_choice(dpp::command_option_choice("Porta", std::string("Porta")))
        .add_choice(dpp::command_option_choice("Armário", std::string("Armário")))
    );

    placa_cmd.add_option(dpp::command_option(dpp::co_attachment, "imagem_referencia", "Uma imagem de referência (opcional).", false));
    commands.push_back(placa_cmd);

    dpp::slashcommand finalizar_placa_cmd("finalizar_placa", "Envia a arte finalizada de uma placa.", bot_id);
    finalizar_placa_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O código da solicitação de placa.", true));
    finalizar_placa_cmd.add_option(dpp::command_option(dpp::co_attachment, "arte_final", "A imagem da placa finalizada.", true));
    commands.push_back(finalizar_placa_cmd);

    dpp::slashcommand lista_placas_cmd("lista_placas", "Lista as solicitações de placas.", bot_id);
    lista_placas_cmd.add_option(
        dpp::command_option(dpp::co_string, "status", "Filtrar por status (padrão: pendente).", false)
        .add_choice(dpp::command_option_choice("Pendentes", std::string("pendente")))
        .add_choice(dpp::command_option_choice("Finalizadas", std::string("finalizada")))
        .add_choice(dpp::command_option_choice("Todas", std::string("todas")))
    );
    commands.push_back(lista_placas_cmd);
}