#include "GerarPlanilhaCommand.h"
#include "ReportGenerator.h"
#include <variant>
#include <string>

GerarPlanilhaCommand::GerarPlanilhaCommand(ReportGenerator& rg) :
    reportGenerator_(rg) {
}

void GerarPlanilhaCommand::handle_gerar_planilha(const dpp::slashcommand_t& event) {
    event.thinking(true);
    std::string tipo = std::get<std::string>(event.get_parameter("tipo"));

    if (tipo == "leads") {
        reportGenerator_.gerarPlanilhaLeads(event);
    }
    else if (tipo == "compras") {
        reportGenerator_.gerarPlanilhaCompras(event);
    }
    else if (tipo == "visitas") {
        reportGenerator_.gerarPlanilhaVisitas(event);
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
        .add_choice(dpp::command_option_choice("Lista de Compras", std::string("compras")))
        .add_choice(dpp::command_option_choice("Visitas", std::string("visitas")))
    );
    commands.push_back(gerar_planilha_cmd);
}