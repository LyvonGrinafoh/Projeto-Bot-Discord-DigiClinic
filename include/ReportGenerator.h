#pragma once
#include <string>
#include <dpp/dpp.h>
#include <optional>

class DatabaseManager;

class ReportGenerator {
private:
    DatabaseManager& db_;
    dpp::cluster& bot_;

public:
    ReportGenerator(DatabaseManager& db, dpp::cluster& bot);
    void gerarPlanilhaLeads(const dpp::slashcommand_t& event, std::optional<int64_t> mes, std::optional<int64_t> ano);
    void gerarPlanilhaCompras(const dpp::slashcommand_t& event, std::optional<int64_t> mes, std::optional<int64_t> ano);
    void gerarPlanilhaVisitas(const dpp::slashcommand_t& event, std::optional<int64_t> mes, std::optional<int64_t> ano);

    void gerarPlanilhaDemandas(const dpp::slashcommand_t& event, std::optional<int64_t> mes, std::optional<int64_t> ano, std::optional<dpp::snowflake> usuario_filtro);

    void gerarPlanilhaReposicao(const dpp::slashcommand_t& event);
};