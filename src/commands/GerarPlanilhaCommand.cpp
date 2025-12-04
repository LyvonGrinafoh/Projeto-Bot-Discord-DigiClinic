#include "GerarPlanilhaCommand.h"
#include "ReportGenerator.h"
#include <variant>
#include <string>
#include <optional>

GerarPlanilhaCommand::GerarPlanilhaCommand(ReportGenerator& rg) :
    reportGenerator_(rg) {
}

void GerarPlanilhaCommand::handle_gerar_planilha(const dpp::slashcommand_t& event) {
    event.thinking(true);
    std::string tipo = std::get<std::string>(event.get_parameter("tipo"));

    std::optional<int64_t> mes;
    auto mes_param = event.get_parameter("mes");
    if (std::holds_alternative<int64_t>(mes_param)) {
        mes = std::get<int64_t>(mes_param);
    }

    std::optional<int64_t> ano;
    auto ano_param = event.get_parameter("ano");
    if (std::holds_alternative<int64_t>(ano_param)) {
        ano = std::get<int64_t>(ano_param);
    }

    std::optional<dpp::snowflake> usuario_filtro;
    auto user_param = event.get_parameter("usuario");
    if (std::holds_alternative<dpp::snowflake>(user_param)) {
        usuario_filtro = std::get<dpp::snowflake>(user_param);
    }

    if (tipo == "leads") {
        reportGenerator_.gerarPlanilhaLeads(event, mes, ano);
    }
    else if (tipo == "compras") {
        reportGenerator_.gerarPlanilhaCompras(event, mes, ano);
    }
    else if (tipo == "visitas") {
        reportGenerator_.gerarPlanilhaVisitas(event, mes, ano);
    }
    else if (tipo == "demandas") {
        reportGenerator_.gerarPlanilhaDemandas(event, mes, ano, usuario_filtro);
    }
    else if (tipo == "reposicao") {
        reportGenerator_.gerarPlanilhaReposicao(event);
    }
    else if (tipo == "relatorios") {
        if (!usuario_filtro.has_value()) {
            event.edit_original_response(dpp::message("⚠️ Para gerar planilha de relatórios diários, você DEVE selecionar um usuário no campo 'usuario'.").set_flags(dpp::m_ephemeral));
            return;
        }
        reportGenerator_.gerarPlanilhaRelatoriosDiarios(event, usuario_filtro.value(), mes, ano);
    }
    else {
        event.edit_original_response(dpp::message("Tipo de planilha desconhecido: " + tipo).set_flags(dpp::m_ephemeral));
    }
}

void GerarPlanilhaCommand::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand gerar_planilha_cmd("gerar_planilha", "Gera e envia uma planilha de dados.", bot_id);
    gerar_planilha_cmd.add_option(
        dpp::command_option(dpp::co_string, "tipo", "Qual planilha você deseja gerar.", true)
        .add_choice(dpp::command_option_choice("Leads", std::string("leads")))
        .add_choice(dpp::command_option_choice("Registros de Gastos", std::string("compras")))
        .add_choice(dpp::command_option_choice("Visitas", std::string("visitas")))
        .add_choice(dpp::command_option_choice("Demandas/Pedidos", std::string("demandas")))
        .add_choice(dpp::command_option_choice("Reposição de Estoque", std::string("reposicao")))
        .add_choice(dpp::command_option_choice("Relatórios Diários (Por Func)", std::string("relatorios")))
    );

    dpp::command_option mes_opt(dpp::co_integer, "mes", "Filtrar por mês (opcional).", false);
    for (int i = 1; i <= 12; ++i) mes_opt.add_choice(dpp::command_option_choice(std::to_string(i), (int64_t)i));
    gerar_planilha_cmd.add_option(mes_opt);

    gerar_planilha_cmd.add_option(
        dpp::command_option(dpp::co_integer, "ano", "Filtrar por ano (opcional, ex: 2024).", false)
    );

    gerar_planilha_cmd.add_option(dpp::command_option(dpp::co_user, "usuario", "Filtrar por usuário (Obrigatório para Relatórios).", false));

    commands.push_back(gerar_planilha_cmd);
}