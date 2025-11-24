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
    );

    dpp::command_option mes_opt(dpp::co_integer, "mes", "Filtrar por mês (opcional).", false);
    mes_opt.add_choice(dpp::command_option_choice("Janeiro", (int64_t)1));
    mes_opt.add_choice(dpp::command_option_choice("Fevereiro", (int64_t)2));
    mes_opt.add_choice(dpp::command_option_choice("Março", (int64_t)3));
    mes_opt.add_choice(dpp::command_option_choice("Abril", (int64_t)4));
    mes_opt.add_choice(dpp::command_option_choice("Maio", (int64_t)5));
    mes_opt.add_choice(dpp::command_option_choice("Junho", (int64_t)6));
    mes_opt.add_choice(dpp::command_option_choice("Julho", (int64_t)7));
    mes_opt.add_choice(dpp::command_option_choice("Agosto", (int64_t)8));
    mes_opt.add_choice(dpp::command_option_choice("Setembro", (int64_t)9));
    mes_opt.add_choice(dpp::command_option_choice("Outubro", (int64_t)10));
    mes_opt.add_choice(dpp::command_option_choice("Novembro", (int64_t)11));
    mes_opt.add_choice(dpp::command_option_choice("Dezembro", (int64_t)12));
    gerar_planilha_cmd.add_option(mes_opt);

    gerar_planilha_cmd.add_option(
        dpp::command_option(dpp::co_integer, "ano", "Filtrar por ano (opcional, ex: 2024).", false)
        .set_min_value(2023)
        .set_max_value(2050)
    );

    gerar_planilha_cmd.add_option(dpp::command_option(dpp::co_user, "usuario", "Filtrar por usuário (apenas para Demandas).", false));

    commands.push_back(gerar_planilha_cmd);
}