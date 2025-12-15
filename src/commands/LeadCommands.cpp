#include "LeadCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "EventHandler.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <optional>
#include <variant>
#include <ctime>
#include <filesystem>
#include <fstream>

LeadCommands::LeadCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler, EventHandler& eventHandler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler), eventHandler_(eventHandler) {
}

void LeadCommands::handle_adicionar_lead(const dpp::slashcommand_t& event) {
    dpp::user author = event.command.get_issuing_user();
    Lead novo_lead;
    novo_lead.id = Utils::gerar_codigo();
    novo_lead.origem = std::get<std::string>(event.get_parameter("origem"));
    novo_lead.data_contato = std::get<std::string>(event.get_parameter("data"));
    novo_lead.hora_contato = std::get<std::string>(event.get_parameter("hora"));
    novo_lead.nome = std::get<std::string>(event.get_parameter("nome"));
    novo_lead.area_atuacao = std::get<std::string>(event.get_parameter("area"));
    novo_lead.unidade = std::get<std::string>(event.get_parameter("unidade"));
    novo_lead.criado_por = author.username;
    novo_lead.conhece_formato = (std::get<std::string>(event.get_parameter("conhece_formato")) == "sim");
    novo_lead.veio_campanha = (std::get<std::string>(event.get_parameter("veio_campanha")) == "sim");

    auto contato_param = event.get_parameter("contato");
    if (std::holds_alternative<std::string>(contato_param)) novo_lead.contato = std::get<std::string>(contato_param);

    if (!novo_lead.contato.empty()) {
        Lead* lead_existente = db_.findLeadByContato(novo_lead.contato);
        if (lead_existente) {
            cmdHandler_.replyAndDelete(event, dpp::message("❌ Lead duplicado! Contato já existe: **" + lead_existente->nome + "** (`" + std::to_string(lead_existente->id) + "`)"));
            return;
        }
    }

    if (db_.addOrUpdateLead(novo_lead)) {
        dpp::embed embed = Utils::criarEmbedPadrao("✅ Novo Lead Adicionado", "", dpp::colors::green);
        embed.add_field("Nome", novo_lead.nome, true)
            .add_field("Origem", novo_lead.origem, true)
            .add_field("Contato", novo_lead.contato.empty() ? "N/A" : novo_lead.contato, true)
            .set_footer(dpp::embed_footer().set_text("Cód: " + std::to_string(novo_lead.id)));

        cmdHandler_.replyAndDelete(event, dpp::message().add_embed(embed));
    }
    else { cmdHandler_.replyAndDelete(event, dpp::message("❌ Erro ao salvar.")); }
}

void LeadCommands::handle_modificar_lead(const dpp::slashcommand_t& event) {
    dpp::user author = event.command.get_issuing_user();
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    Lead* lead_ptr = db_.getLeadPtr(codigo);

    if (lead_ptr) {
        Lead& lead = *lead_ptr;
        std::string obs = std::get<std::string>(event.get_parameter("observacao"));
        lead.observacoes += "\n[" + Utils::format_timestamp(std::time(nullptr)) + " | " + author.username + "]: " + obs;
        bool modificado = true;

        auto check_s = [&](const std::string& p, std::string& f) { auto par = event.get_parameter(p); if (auto v = std::get_if<std::string>(&par)) { if (*v != f) { f = *v; return true; } } return false; };
        auto check_b = [&](const std::string& p, bool& f) { auto par = event.get_parameter(p); if (auto v = std::get_if<std::string>(&par)) { bool n = (*v == "sim"); if (n != f) { f = n; return true; } } return false; };

        modificado |= check_s("nome", lead.nome);

        auto c_par = event.get_parameter("contato");
        if (auto v = std::get_if<std::string>(&c_par)) {
            if (*v != lead.contato && !v->empty()) {
                if (db_.findLeadByContato(*v)) { cmdHandler_.replyAndDelete(event, dpp::message("❌ Contato já existe em outro lead.")); return; }
                lead.contato = *v; modificado = true;
            }
        }

        modificado |= check_s("area", lead.area_atuacao);
        modificado |= check_s("unidade", lead.unidade);
        modificado |= check_b("conhece_formato", lead.conhece_formato);
        modificado |= check_b("veio_campanha", lead.veio_campanha);

        auto r_par = event.get_parameter("respondido");
        if (auto v = std::get_if<std::string>(&r_par)) {
            bool n = (*v == "sim");
            if (n != lead.respondido) { lead.respondido = n; lead.data_hora_resposta = n ? Utils::format_timestamp(std::time(nullptr)) : ""; modificado = true; }
        }

        modificado |= check_s("status_conversa", lead.status_conversa);
        modificado |= check_s("status_followup", lead.status_followup);
        modificado |= check_b("problema_contato", lead.problema_contato);
        modificado |= check_b("convidou_visita", lead.convidou_visita);
        modificado |= check_b("agendou_visita", lead.agendou_visita);
        modificado |= check_s("tipo_fechamento", lead.tipo_fechamento);
        modificado |= check_b("teve_adicionais", lead.teve_adicionais);
        auto val_par = event.get_parameter("valor_fechamento");
        if (std::holds_alternative<double>(val_par)) { lead.valor_fechamento = std::get<double>(val_par); modificado = true; }

        auto print_par = event.get_parameter("print_final");
        if (std::holds_alternative<dpp::snowflake>(print_par)) {
            dpp::attachment att = event.command.get_resolved_attachment(std::get<dpp::snowflake>(print_par));
            std::string path = "lead_prints/" + std::to_string(codigo) + (att.filename.find('.') != std::string::npos ? att.filename.substr(att.filename.find_last_of('.')) : ".png");

            Utils::BaixarAnexo(&bot_, att.url, path, [this, event, &lead, path](bool ok, std::string c) {
                if (ok) {
                    lead.print_final_conversa = path;
                    db_.saveLeads();
                    cmdHandler_.replyAndDelete(event, dpp::message("✅ Lead modificado e print salvo!"));
                }
                else cmdHandler_.replyAndDelete(event, dpp::message("❌ Erro ao baixar print."));
                });
            return;
        }

        if (modificado) {
            db_.saveLeads();
            cmdHandler_.replyAndDelete(event, dpp::message("✅ Lead `" + std::to_string(codigo) + "` modificado!"));
        }
        else cmdHandler_.replyAndDelete(event, dpp::message("⚠️ Nenhuma alteração."));
    }
    else cmdHandler_.replyAndDelete(event, dpp::message("❌ Lead não encontrado."));
}

void LeadCommands::handle_listar_leads(const dpp::slashcommand_t& event) {
    std::string tipo = std::get<std::string>(event.get_parameter("tipo"));
    const auto& leads = db_.getLeads();
    std::vector<PaginatedItem> items;

    for (const auto& [id, lead] : leads) {
        bool add = false;
        if (tipo == "todos") add = true;
        else if (tipo == "nao_contatados" && lead.status_conversa == "Nao Contatado") add = true;
        else if (tipo == "com_problema" && lead.problema_contato) add = true;
        else if (tipo == "nao_respondidos" && !lead.respondido) add = true;
        if (add) items.push_back(lead);
    }

    if (items.empty()) { cmdHandler_.replyAndDelete(event, dpp::message("Nenhum lead encontrado.")); return; }

    PaginationState state;
    state.channel_id = event.command.channel_id;
    state.items = std::move(items);
    state.originalUserID = event.command.get_issuing_user().id;
    state.listType = "leads";
    state.currentPage = 1;
    state.itemsPerPage = 5;

    dpp::embed firstPage = Utils::generatePageEmbed(state);

    event.reply(dpp::message(event.command.channel_id, firstPage), [this, state, event](const auto& cb) {
        if (!cb.is_error()) {
            event.get_original_response([this, state](const auto& msg_cb) {
                if (!msg_cb.is_error()) {
                    auto m = std::get<dpp::message>(msg_cb.value);
                    eventHandler_.addPaginationState(m.id, state);

                    bot_.message_add_reaction(m.id, m.channel_id, "🗑️", [this, m](auto cb_trash) {
                        if (!cb_trash.is_error()) {
                            bot_.message_add_reaction(m.id, m.channel_id, "◀️", [this, m](auto cb_left) {
                                if (!cb_left.is_error()) {
                                    bot_.message_add_reaction(m.id, m.channel_id, "▶️");
                                }
                                });
                        }
                        });

                    bot_.start_timer([this, m](auto t) { bot_.message_delete(m.id, m.channel_id); bot_.stop_timer(t); }, 60);
                }
                });
        }
        });
}

void LeadCommands::handle_ver_lead(const dpp::slashcommand_t& event) {
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    std::optional<Lead> opt = db_.getLead(codigo);
    if (opt) {
        const Lead& l = *opt;
        dpp::embed embed = Utils::criarEmbedPadrao("Ficha: " + l.nome, "", dpp::colors::blue);
        embed.add_field("Código", std::to_string(l.id), true)
            .add_field("Contato", l.contato.empty() ? "N/A" : l.contato, true)
            .add_field("Origem", l.origem, true)
            .add_field("Área", l.area_atuacao, true)
            .add_field("Status", l.status_conversa, true)
            .add_field("Observações", l.observacoes.empty() ? "N/A" : l.observacoes.substr(0, 1000), false);

        if (!l.print_final_conversa.empty()) {
            if (l.print_final_conversa.find("lead_prints") == 0 && std::filesystem::exists(l.print_final_conversa)) {
                embed.set_image("attachment://" + l.print_final_conversa);
                dpp::message msg(event.command.channel_id, embed);
                msg.add_file(l.print_final_conversa, dpp::utility::read_file(l.print_final_conversa));

                event.reply(msg, [this, event](const auto& cb) {
                    if (!cb.is_error()) {
                        event.get_original_response([this](const auto& m_cb) {
                            if (!m_cb.is_error()) {
                                auto m = std::get<dpp::message>(m_cb.value);
                                bot_.message_add_reaction(m.id, m.channel_id, "🗑️");
                                bot_.start_timer([this, m](auto t) { bot_.message_delete(m.id, m.channel_id); bot_.stop_timer(t); }, 60);
                            }
                            });
                    }
                    });
                return;
            }
            else {
                embed.set_image(l.print_final_conversa);
            }
        }

        dpp::message msg(event.command.channel_id, embed);
        cmdHandler_.replyAndDelete(event, msg);
    }
    else { cmdHandler_.replyAndDelete(event, dpp::message("❌ Não encontrado.")); }
}

void LeadCommands::handle_limpar_leads(const dpp::slashcommand_t& event) {
    db_.clearLeads();
    cmdHandler_.replyAndDelete(event, dpp::message("🧹 Leads limpos."));
}

void LeadCommands::handle_deletar_lead(const dpp::slashcommand_t& event) {
    int64_t codigo = std::get<int64_t>(event.get_parameter("codigo"));
    if (db_.removeLead(codigo)) cmdHandler_.replyAndDelete(event, dpp::message("✅ Lead deletado."));
    else cmdHandler_.replyAndDelete(event, dpp::message("❌ Erro ao deletar."));
}

void LeadCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand add("adicionar_lead", "Novo lead.", bot_id);
    add.add_option(dpp::command_option(dpp::co_string, "origem", "Origem", true).add_choice(dpp::command_option_choice("Whatsapp", "Whatsapp")).add_choice(dpp::command_option_choice("Instagram", "Instagram")).add_choice(dpp::command_option_choice("Facebook", "Facebook")).add_choice(dpp::command_option_choice("Site", "Site")));
    add.add_option(dpp::command_option(dpp::co_string, "data", "DD/MM/AAAA", true));
    add.add_option(dpp::command_option(dpp::co_string, "hora", "HH:MM", true));
    add.add_option(dpp::command_option(dpp::co_string, "nome", "Nome", true));
    add.add_option(dpp::command_option(dpp::co_string, "area", "Area", true));
    add.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade", true).add_choice(dpp::command_option_choice("Campo Belo", "Campo Belo")).add_choice(dpp::command_option_choice("Tatuapé", "Tatuape")).add_choice(dpp::command_option_choice("Ambas", "Ambas")));
    add.add_option(dpp::command_option(dpp::co_string, "conhece_formato", "Conhece?", true).add_choice(dpp::command_option_choice("Sim", "sim")).add_choice(dpp::command_option_choice("Não", "nao")));
    add.add_option(dpp::command_option(dpp::co_string, "veio_campanha", "Campanha?", true).add_choice(dpp::command_option_choice("Sim", "sim")).add_choice(dpp::command_option_choice("Não", "nao")));
    add.add_option(dpp::command_option(dpp::co_string, "contato", "Tel/Email", false));
    commands.push_back(add);

    dpp::slashcommand mod("modificar_lead", "Edita lead.", bot_id);
    mod.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID", true));
    mod.add_option(dpp::command_option(dpp::co_string, "observacao", "Motivo", true));
    mod.add_option(dpp::command_option(dpp::co_string, "nome", "Nome", false));
    mod.add_option(dpp::command_option(dpp::co_string, "contato", "Contato", false));
    mod.add_option(dpp::command_option(dpp::co_string, "area", "Area", false));
    mod.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade", false).add_choice(dpp::command_option_choice("Campo Belo", "Campo Belo")).add_choice(dpp::command_option_choice("Tatuapé", "Tatuape")).add_choice(dpp::command_option_choice("Ambas", "Ambas")));
    mod.add_option(dpp::command_option(dpp::co_string, "conhece_formato", "Conhece?", false).add_choice(dpp::command_option_choice("Sim", "sim")).add_choice(dpp::command_option_choice("Não", "nao")));
    mod.add_option(dpp::command_option(dpp::co_string, "veio_campanha", "Campanha?", false).add_choice(dpp::command_option_choice("Sim", "sim")).add_choice(dpp::command_option_choice("Não", "nao")));
    mod.add_option(dpp::command_option(dpp::co_string, "respondido", "Respondido?", false).add_choice(dpp::command_option_choice("Sim", "sim")).add_choice(dpp::command_option_choice("Não", "nao")));
    mod.add_option(dpp::command_option(dpp::co_string, "status_conversa", "Status", false).add_choice(dpp::command_option_choice("Orcamento enviado", "Orcamento enviado")).add_choice(dpp::command_option_choice("Desqualificado", "Desqualificado")).add_choice(dpp::command_option_choice("Em negociação", "Em negociacao")).add_choice(dpp::command_option_choice("Fechamento", "Fechamento")).add_choice(dpp::command_option_choice("Sem Resposta", "Sem Resposta")).add_choice(dpp::command_option_choice("Não Contatado", "Nao Contatado")));
    mod.add_option(dpp::command_option(dpp::co_string, "status_followup", "FollowUp", false).add_choice(dpp::command_option_choice("Primeiro contato", "Primeiro contato")).add_choice(dpp::command_option_choice("Follow Up 1", "Follow Up 1")).add_choice(dpp::command_option_choice("Follow Up 2", "Follow Up 2")).add_choice(dpp::command_option_choice("Follow Up 3", "Follow Up 3")).add_choice(dpp::command_option_choice("Follow Up 4", "Follow Up 4")).add_choice(dpp::command_option_choice("Contato encerrado", "Contato encerrado")).add_choice(dpp::command_option_choice("Retornar contato", "Retornar contato")));
    mod.add_option(dpp::command_option(dpp::co_string, "problema_contato", "Problema?", false).add_choice(dpp::command_option_choice("Sim", "sim")).add_choice(dpp::command_option_choice("Não", "nao")));
    mod.add_option(dpp::command_option(dpp::co_string, "convidou_visita", "Convidou?", false).add_choice(dpp::command_option_choice("Sim", "sim")).add_choice(dpp::command_option_choice("Não", "nao")));
    mod.add_option(dpp::command_option(dpp::co_string, "agendou_visita", "Agendou?", false).add_choice(dpp::command_option_choice("Sim", "sim")).add_choice(dpp::command_option_choice("Não", "nao")));
    mod.add_option(dpp::command_option(dpp::co_string, "tipo_fechamento", "Tipo Fecha.", false).add_choice(dpp::command_option_choice("Diária", "Diaria")).add_choice(dpp::command_option_choice("Pacote de horas", "Pacote de horas")).add_choice(dpp::command_option_choice("Mensal", "Mensal")).add_choice(dpp::command_option_choice("Hora avulsa", "Hora avulsa")).add_choice(dpp::command_option_choice("Outro", "Outro")));
    mod.add_option(dpp::command_option(dpp::co_string, "teve_adicionais", "Adicionais?", false).add_choice(dpp::command_option_choice("Sim", "sim")).add_choice(dpp::command_option_choice("Não", "nao")));
    mod.add_option(dpp::command_option(dpp::co_number, "valor_fechamento", "Valor", false));
    mod.add_option(dpp::command_option(dpp::co_attachment, "print_final", "Print", false));
    commands.push_back(mod);

    dpp::slashcommand list("listar_leads", "Lista leads.", bot_id);
    list.add_option(dpp::command_option(dpp::co_string, "tipo", "Filtro", true).add_choice(dpp::command_option_choice("Todos", "todos")).add_choice(dpp::command_option_choice("Não Contatados", "nao_contatados")).add_choice(dpp::command_option_choice("Com Problema", "com_problema")).add_choice(dpp::command_option_choice("Não Respondidos", "nao_respondidos")));
    commands.push_back(list);

    dpp::slashcommand ver("ver_lead", "Ver ficha.", bot_id);
    ver.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID", true));
    commands.push_back(ver);

    dpp::slashcommand limp("limpar_leads", "Apaga tudo.", bot_id);
    limp.default_member_permissions = dpp::p_administrator;
    commands.push_back(limp);

    dpp::slashcommand del("deletar_lead", "Apaga um.", bot_id);
    del.add_option(dpp::command_option(dpp::co_integer, "codigo", "ID", true));
    del.default_member_permissions = dpp::p_administrator;
    commands.push_back(del);
}