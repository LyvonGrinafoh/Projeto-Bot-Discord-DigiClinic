#include "LeadCommands.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
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

LeadCommands::LeadCommands(dpp::cluster& bot, DatabaseManager& db, const BotConfig& config, CommandHandler& handler) :
    bot_(bot), db_(db), config_(config), cmdHandler_(handler) {
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
    if (std::holds_alternative<std::string>(contato_param)) { novo_lead.contato = std::get<std::string>(contato_param); }

    if (db_.addOrUpdateLead(novo_lead)) {
        dpp::embed embed = dpp::embed().set_color(dpp::colors::green).set_title("✅ Novo Lead Adicionado").add_field("Nome", novo_lead.nome, true).add_field("Origem", novo_lead.origem, true).add_field("Contato", novo_lead.contato.empty() ? "Nao informado" : novo_lead.contato, true).set_footer(dpp::embed_footer().set_text("Codigo do Lead: " + std::to_string(novo_lead.id)));
        cmdHandler_.replyAndDelete(event, dpp::message().add_embed(embed));
    }
    else { event.reply(dpp::message("❌ Erro ao salvar o novo lead no banco de dados.").set_flags(dpp::m_ephemeral)); }
}

void LeadCommands::handle_modificar_lead(const dpp::slashcommand_t& event) {
    dpp::user author = event.command.get_issuing_user(); int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo")); uint64_t codigo = static_cast<uint64_t>(codigo_int);
    Lead* lead_ptr = db_.getLeadPtr(codigo);

    if (lead_ptr) {
        Lead& lead_a_modificar = *lead_ptr; std::string observacao_motivo = std::get<std::string>(event.get_parameter("observacao")); std::string timestamp_obs = Utils::format_timestamp(std::time(nullptr)); std::string autor_obs = author.username; bool modificado = false;
        if (!lead_a_modificar.observacoes.empty()) { lead_a_modificar.observacoes += "\n"; }
        lead_a_modificar.observacoes += "[" + timestamp_obs + " | " + autor_obs + "]: " + observacao_motivo; modificado = true;

        auto check_update_string = [&](const std::string& param_name, std::string& field) { auto param = event.get_parameter(param_name); if (auto val_ptr = std::get_if<std::string>(&param)) { if (*val_ptr != field) { field = *val_ptr; return true; } } return false; };
        auto check_update_bool = [&](const std::string& param_name, bool& field) { auto param = event.get_parameter(param_name); if (auto val_ptr = std::get_if<std::string>(&param)) { bool new_value = (*val_ptr == "sim"); if (new_value != field) { field = new_value; return true; } } return false; };

        modificado |= check_update_string("nome", lead_a_modificar.nome); modificado |= check_update_string("contato", lead_a_modificar.contato); modificado |= check_update_string("area", lead_a_modificar.area_atuacao); modificado |= check_update_string("unidade", lead_a_modificar.unidade); modificado |= check_update_bool("conhece_formato", lead_a_modificar.conhece_formato); modificado |= check_update_bool("veio_campanha", lead_a_modificar.veio_campanha);
        auto respondido_param = event.get_parameter("respondido"); if (auto val_ptr = std::get_if<std::string>(&respondido_param)) { bool new_value = (*val_ptr == "sim"); if (new_value != lead_a_modificar.respondido) { lead_a_modificar.respondido = new_value; lead_a_modificar.data_hora_resposta = new_value ? Utils::format_timestamp(std::time(nullptr)) : ""; modificado = true; } }
        modificado |= check_update_string("status_conversa", lead_a_modificar.status_conversa); modificado |= check_update_string("status_followup", lead_a_modificar.status_followup); modificado |= check_update_bool("problema_contato", lead_a_modificar.problema_contato); modificado |= check_update_bool("convidou_visita", lead_a_modificar.convidou_visita); modificado |= check_update_bool("agendou_visita", lead_a_modificar.agendou_visita); modificado |= check_update_string("tipo_fechamento", lead_a_modificar.tipo_fechamento); modificado |= check_update_bool("teve_adicionais", lead_a_modificar.teve_adicionais);
        auto valor_fechamento_param = event.get_parameter("valor_fechamento"); if (auto val_ptr = std::get_if<double>(&valor_fechamento_param)) { if (*val_ptr != lead_a_modificar.valor_fechamento) { lead_a_modificar.valor_fechamento = *val_ptr; modificado = true; } }
        auto print_param = event.get_parameter("print_final"); if (auto val_ptr = std::get_if<dpp::snowflake>(&print_param)) { dpp::attachment anexo = event.command.get_resolved_attachment(*val_ptr); if (anexo.url != lead_a_modificar.print_final_conversa) { lead_a_modificar.print_final_conversa = anexo.url; modificado = true; } }

        if (modificado) {
            if (db_.saveLeads()) { cmdHandler_.replyAndDelete(event, dpp::message("✅ Lead `" + std::to_string(codigo) + "` modificado com sucesso!")); }
            else { event.reply(dpp::message("❌ Erro ao salvar as alterações do lead.").set_flags(dpp::m_ephemeral)); }
        }
        else { event.reply(dpp::message("⚠️ Nenhuma alteração detectada para o lead `" + std::to_string(codigo) + "` (além da observação obrigatória).").set_flags(dpp::m_ephemeral)); }
    }
    else { event.reply(dpp::message("❌ Codigo do lead nao encontrado.").set_flags(dpp::m_ephemeral)); }
}

void LeadCommands::handle_listar_leads(const dpp::slashcommand_t& event) {
    std::string tipo_lista = std::get<std::string>(event.get_parameter("tipo")); dpp::embed embed = dpp::embed().set_color(dpp::colors::blue); std::string description; int count = 0; const auto& leads_map = db_.getLeads();
    std::string title; if (tipo_lista == "todos") title = "📋 Lista de Todos os Leads"; else if (tipo_lista == "nao_contatados") title = "📋 Lista de Leads Nao Contatados"; else if (tipo_lista == "com_problema") title = "📋 Lista de Leads com Problema de Contato"; else if (tipo_lista == "nao_respondidos") title = "📋 Lista de Leads Nao Respondidos"; else title = "📋 Lista Desconhecida"; embed.set_title(title);
    for (const auto& [id, lead] : leads_map) {
        bool adicionar = false; if (tipo_lista == "todos") { adicionar = true; }
        else if (tipo_lista == "nao_contatados" && lead.status_conversa == "Nao Contatado") { adicionar = true; }
        else if (tipo_lista == "com_problema" && lead.problema_contato) { adicionar = true; }
        else if (tipo_lista == "nao_respondidos" && !lead.respondido) { adicionar = true; }
        if (adicionar) { description += "**Nome:** " + lead.nome + "\n"; description += "**Codigo:** `" + std::to_string(id) + "`\n"; description += "**Data Contato:** " + lead.data_contato + "\n"; description += "**Contato:** " + (lead.contato.empty() ? "Nao informado" : lead.contato) + "\n"; description += "**Status:** " + lead.status_conversa + "\n"; description += "--------------------\n"; count++; }
    }
    if (count == 0) { description = "Nenhum lead encontrado para este filtro."; }
    else { embed.set_title(embed.title + " (" + std::to_string(count) + ")"); }
    if (description.length() > 4000) { description = description.substr(0, 4000) + "\n... (lista muito longa para exibir)"; }
    embed.set_description(description); event.reply(dpp::message().add_embed(embed));
}

void LeadCommands::handle_ver_lead(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo")); uint64_t codigo = static_cast<uint64_t>(codigo_int);
    std::optional<Lead> lead_opt = db_.getLead(codigo);
    if (lead_opt) {
        const Lead& lead = *lead_opt; dpp::embed embed = dpp::embed().set_color(dpp::colors::blue).set_title("Ficha do Lead: " + lead.nome);
        embed.add_field("Codigo", "`" + std::to_string(lead.id) + "`", true); embed.add_field("Criado Por", lead.criado_por.empty() ? "N/A" : lead.criado_por, true); embed.add_field("Origem", lead.origem, true); embed.add_field("Data/Hora Contato", lead.data_contato + " as " + lead.hora_contato, true); embed.add_field("Contato", lead.contato.empty() ? "Nao informado" : lead.contato, true); embed.add_field("Area", lead.area_atuacao, true); embed.add_field("Unidade", lead.unidade, true); embed.add_field("Conhece o Formato?", lead.conhece_formato ? "Sim" : "Nao", true); embed.add_field("Veio de Campanha?", lead.veio_campanha ? "Sim" : "Nao", true); embed.add_field("Respondido?", lead.respondido ? "Sim" : "Nao", true); embed.add_field("Data/Hora Resposta", lead.data_hora_resposta.empty() ? "N/A" : lead.data_hora_resposta, true); embed.add_field("Problema no Contato?", lead.problema_contato ? "Sim" : "Nao", true); embed.add_field("Status da Conversa", lead.status_conversa, true); embed.add_field("Status do FollowUp", lead.status_followup, true); embed.add_field("Convidou Visita?", lead.convidou_visita ? "Sim" : "Nao", true); embed.add_field("Agendou Visita?", lead.agendou_visita ? "Sim" : "Nao", true); embed.add_field("Tipo de Fechamento", lead.tipo_fechamento.empty() ? "N/A" : lead.tipo_fechamento, true); std::stringstream ss_valor; ss_valor << std::fixed << std::setprecision(2) << lead.valor_fechamento; embed.add_field("Valor do Fechamento", "R$ " + ss_valor.str(), true); embed.add_field("Teve Adicionais?", lead.teve_adicionais ? "Sim" : "Nao", true);
        std::string obs = lead.observacoes.empty() ? "Nenhuma observacao." : lead.observacoes; if (obs.length() > 1000) { obs = obs.substr(0, 1000) + "..."; } embed.add_field("Observacoes", obs, false);
        if (!lead.print_final_conversa.empty()) { embed.set_image(lead.print_final_conversa); }
        event.reply(dpp::message().add_embed(embed));
    }
    else { event.reply(dpp::message("❌ Codigo do lead nao encontrado.").set_flags(dpp::m_ephemeral)); }
}

void LeadCommands::handle_limpar_leads(const dpp::slashcommand_t& event) {
    db_.clearLeads(); event.reply(dpp::message("🧹 Todos os leads foram limpos do banco de dados.").set_flags(dpp::m_ephemeral));
}

void LeadCommands::handle_deletar_lead(const dpp::slashcommand_t& event) {
    int64_t codigo_int = std::get<int64_t>(event.get_parameter("codigo")); uint64_t codigo = static_cast<uint64_t>(codigo_int);
    std::optional<Lead> lead_opt = db_.getLead(codigo);
    if (lead_opt) {
        std::string nome_do_lead = lead_opt->nome;
        if (db_.removeLead(codigo)) { cmdHandler_.replyAndDelete(event, dpp::message("✅ O lead '" + nome_do_lead + "' (codigo `" + std::to_string(codigo) + "`) foi deletado com sucesso.")); }
        else { event.reply(dpp::message("❌ Erro ao deletar o lead (não foi possível salvar o banco de dados após remoção).").set_flags(dpp::m_ephemeral)); }
    }
    else { event.reply(dpp::message("❌ Codigo do lead nao encontrado.").set_flags(dpp::m_ephemeral)); }
}

void LeadCommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand adicionar_lead_cmd("adicionar_lead", "Adiciona um novo lead ao banco de dados.", bot_id);
    adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "origem", "De onde o lead veio.", true).add_choice(dpp::command_option_choice("Whatsapp", std::string("Whatsapp"))).add_choice(dpp::command_option_choice("Instagram", std::string("Instagram"))).add_choice(dpp::command_option_choice("Facebook", std::string("Facebook"))).add_choice(dpp::command_option_choice("Site", std::string("Site"))));
    adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "data", "Data do primeiro contato (DD/MM/AAAA).", true));
    adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "hora", "Hora do primeiro contato (HH:MM).", true));
    adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "nome", "Nome do lead.", true));
    adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Area de atuacao do lead.", true));
    adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Unidade de interesse.", true).add_choice(dpp::command_option_choice("Campo Belo", std::string("Campo Belo"))).add_choice(dpp::command_option_choice("Tatuape", std::string("Tatuape"))).add_choice(dpp::command_option_choice("Ambas", std::string("Ambas"))));
    adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "conhece_formato", "O lead ja conhece o formato de coworking?", true).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
    adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "veio_campanha", "O lead veio de alguma campanha paga?", true).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
    adicionar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "contato", "Telefone ou e-mail do lead.", false));
    commands.push_back(adicionar_lead_cmd);
    dpp::slashcommand modificar_lead_cmd("modificar_lead", "Modifica ou adiciona informacoes a um lead existente.", bot_id);
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O codigo unico do lead a ser modificado.", true));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "observacao", "MOTIVO da modificacao (obrigatorio).", true));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "nome", "Altera o nome do lead.", false));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "contato", "Altera o numero ou email de contato do lead.", false));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "area", "Altera a area de atuacao do lead.", false));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "unidade", "Altera a unidade de interesse.", false).add_choice(dpp::command_option_choice("Campo Belo", std::string("Campo Belo"))).add_choice(dpp::command_option_choice("Tatuape", std::string("Tatuape"))).add_choice(dpp::command_option_choice("Ambas", std::string("Ambas"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "conhece_formato", "Altera se o lead conhece o formato.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "veio_campanha", "Altera se o lead veio de campanha.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "respondido", "Marcar/desmarcar se o lead foi respondido.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "status_conversa", "Altera o status da conversa.", false).add_choice(dpp::command_option_choice("Orcamento enviado", std::string("Orcamento enviado"))).add_choice(dpp::command_option_choice("Desqualificado", std::string("Desqualificado"))).add_choice(dpp::command_option_choice("Em negociacao", std::string("Em negociacao"))).add_choice(dpp::command_option_choice("Fechamento", std::string("Fechamento"))).add_choice(dpp::command_option_choice("Sem Resposta", std::string("Sem Resposta"))).add_choice(dpp::command_option_choice("Nao Contatado", std::string("Nao Contatado"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "status_followup", "Altera o status do followup.", false).add_choice(dpp::command_option_choice("Primeiro contato", std::string("Primeiro contato"))).add_choice(dpp::command_option_choice("Follow Up 1", std::string("Follow Up 1"))).add_choice(dpp::command_option_choice("Follow Up 2", std::string("Follow Up 2"))).add_choice(dpp::command_option_choice("Follow Up 3", std::string("Follow Up 3"))).add_choice(dpp::command_option_choice("Follow Up 4", std::string("Follow Up 4"))).add_choice(dpp::command_option_choice("Contato encerrado", std::string("Contato encerrado"))).add_choice(dpp::command_option_choice("Retornar contato", std::string("Retornar contato"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "problema_contato", "Marcar/desmarcar problema no contato.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "convidou_visita", "Marcar se convidou para visitar.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "agendou_visita", "Marcar se agendou visita.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "tipo_fechamento", "Define o tipo de fechamento.", false).add_choice(dpp::command_option_choice("Diaria", std::string("Diaria"))).add_choice(dpp::command_option_choice("Pacote de horas", std::string("Pacote de horas"))).add_choice(dpp::command_option_choice("Mensal", std::string("Mensal"))).add_choice(dpp::command_option_choice("Hora avulsa", std::string("Hora avulsa"))).add_choice(dpp::command_option_choice("Outro", std::string("Outro"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_string, "teve_adicionais", "Marcar se teve adicionais no fechamento.", false).add_choice(dpp::command_option_choice("Sim", std::string("sim"))).add_choice(dpp::command_option_choice("Nao", std::string("nao"))));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_number, "valor_fechamento", "Define o valor final do fechamento.", false));
    modificar_lead_cmd.add_option(dpp::command_option(dpp::co_attachment, "print_final", "Anexa o print do final da conversa.", false));
    commands.push_back(modificar_lead_cmd);
    dpp::slashcommand listar_leads_cmd("listar_leads", "Lista os leads com base em um filtro.", bot_id);
    listar_leads_cmd.add_option(dpp::command_option(dpp::co_string, "tipo", "O tipo de lista que voce quer ver.", true).add_choice(dpp::command_option_choice("Todos", std::string("todos"))).add_choice(dpp::command_option_choice("Nao Contatados", std::string("nao_contatados"))).add_choice(dpp::command_option_choice("Com Problema", std::string("com_problema"))).add_choice(dpp::command_option_choice("Nao Respondidos", std::string("nao_respondidos"))));
    commands.push_back(listar_leads_cmd);
    dpp::slashcommand ver_lead_cmd("ver_lead", "Mostra todas as informacoes de um lead especifico.", bot_id);
    ver_lead_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O codigo do lead que voce quer ver.", true));
    commands.push_back(ver_lead_cmd);
    dpp::slashcommand limpar_leads_cmd("limpar_leads", "Limpa TODOS os leads do banco de dados.", bot_id);
    limpar_leads_cmd.default_member_permissions = dpp::p_administrator; commands.push_back(limpar_leads_cmd);
    dpp::slashcommand deletar_lead_cmd("deletar_lead", "Deleta um lead permanentemente do banco de dados.", bot_id);
    deletar_lead_cmd.add_option(dpp::command_option(dpp::co_integer, "codigo", "O codigo do lead que voce deseja deletar.", true));
    deletar_lead_cmd.default_member_permissions = dpp::p_administrator; commands.push_back(deletar_lead_cmd);
}