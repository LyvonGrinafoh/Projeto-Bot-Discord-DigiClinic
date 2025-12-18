#include "AICommands.h"
#include "AIHandler.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "Utils.h"
#include <sstream>
#include <map>
#include <ctime>
#include <iomanip>

std::string getCargoFuncionario(const std::string& nome) {
    static std::map<std::string, std::string> cargos = {
        {"Matheus", "Marketing e Design"},
        {"Joao", "Vendas (SDR)"},
        {"Maria", "Atendimento/Recepção"},
        {"Ana", "Financeiro"}
    };
    for (const auto& [key, val] : cargos) {
        if (nome.find(key) != std::string::npos) return val;
    }
    return "Funcionário Geral";
}

std::pair<int, int> pegarMesAno(const std::string& data) {
    if (data.length() < 10) return { 0, 0 };
    try {
        int dia, mes, ano; char barra; std::stringstream ss(data);
        ss >> dia >> barra >> mes >> barra >> ano;
        return { mes, ano };
    }
    catch (...) { return { 0, 0 }; }
}

AICommands::AICommands(dpp::cluster& bot, AIHandler& ai, DatabaseManager& db, CommandHandler& handler) :
    bot_(bot), ai_(ai), db_(db), cmdHandler_(handler) {
}

void AICommands::handle_ensinar(const dpp::slashcommand_t& event) {
    if (!event.command.get_issuing_user().is_mfa_enabled() && event.command.get_issuing_user().id != 187910708674035713) {
    }

    std::string novo_conhecimento = std::get<std::string>(event.get_parameter("info"));
    if (db_.adicionarConhecimento(novo_conhecimento)) {
        dpp::embed embed = Utils::criarEmbedPadrao("🧠 Cérebro Atualizado", "Novo conhecimento adicionado com sucesso!", dpp::colors::green);
        embed.add_field("Info", novo_conhecimento, false);
        cmdHandler_.replyAndDelete(event, dpp::message().add_embed(embed));
    }
    else {
        cmdHandler_.replyAndDelete(event, dpp::message("❌ Erro ao salvar conhecimento."));
    }
}

void AICommands::handle_analise_leads(const dpp::slashcommand_t& event) {
    event.thinking(false);
    const auto& leads = db_.getLeads();
    if (leads.empty()) { cmdHandler_.editAndDelete(event, dpp::message("❌ Sem leads para analisar.")); return; }

    std::time_t t = std::time(nullptr); std::tm* now = std::localtime(&t);
    int mes_atual = now->tm_mon + 1; int ano_atual = now->tm_year + 1900;
    int mes_anterior = (mes_atual == 1) ? 12 : mes_atual - 1; int ano_anterior = (mes_atual == 1) ? ano_atual - 1 : ano_atual;
    struct Stats { int total = 0; int fechamentos = 0; std::map<std::string, int> origens; std::vector<std::string> amostra_obs; };
    Stats s_atual, s_anterior;

    for (const auto& [id, l] : leads) {
        std::pair<int, int> data_lead = pegarMesAno(l.data_contato);
        Stats* target = nullptr;
        if (data_lead.first == mes_atual && data_lead.second == ano_atual) target = &s_atual;
        else if (data_lead.first == mes_anterior && data_lead.second == ano_anterior) target = &s_anterior;
        if (target) {
            target->total++;
            target->origens[l.origem]++;
            if (l.status_conversa == "Fechamento" || l.valor_fechamento > 0) target->fechamentos++;
            if (target->amostra_obs.size() < 5) target->amostra_obs.push_back("Cliente: " + l.nome + " | Por: " + l.criado_por + " | Obs: " + l.observacoes);
        }
    }

    std::stringstream prompt;
    prompt << "Analise os dados comerciais da clínica. Mês " << mes_atual << " vs " << mes_anterior << ".\n";
    prompt << "ATUAL: " << s_atual.total << " leads, " << s_atual.fechamentos << " vendas.\n";
    prompt << "ANTERIOR: " << s_anterior.total << " leads, " << s_anterior.fechamentos << " vendas.\n";
    prompt << "DETALHES RECENTES:\n"; for (const auto& o : s_atual.amostra_obs) prompt << "- " << o << "\n";

    ai_.AskWithContext("Você é um Diretor Comercial.", prompt.str(), [this, event, mes_atual](std::string resposta) {
        dpp::embed embed = Utils::criarEmbedPadrao("📊 Relatório Comercial", resposta, dpp::colors::gold);
        cmdHandler_.editAndDelete(event, dpp::message().add_embed(embed), 600);
        }, 0.4f);
}

void AICommands::handle_melhor_funcionario(const dpp::slashcommand_t& event) {
    event.thinking(false);
    auto relatorios = db_.getTodosRelatorios();
    const auto& solicitacoes = db_.getSolicitacoes();
    std::time_t t = std::time(nullptr); std::tm* now = std::localtime(&t);
    int mes_atual = now->tm_mon + 1; int ano_atual = now->tm_year + 1900;

    struct Perf { std::string cargo; int relatorios = 0; std::vector<Solicitacao> tarefas; std::string logs; };
    std::map<std::string, Perf> equipe;

    for (const auto& r : relatorios) {
        auto d = pegarMesAno(r.data_hora);
        if (d.first == mes_atual && d.second == ano_atual) {
            Perf& p = equipe[r.username];
            if (p.cargo.empty()) p.cargo = getCargoFuncionario(r.username);
            p.relatorios++;
            p.logs += "- " + r.conteudo.substr(0, 80) + "...\n";
        }
    }
    for (const auto& [id, s] : solicitacoes) {
        std::string ref = s.data_finalizacao.empty() ? s.data_criacao : s.data_finalizacao;
        auto d = pegarMesAno(ref);
        if (d.first == mes_atual && d.second == ano_atual) { equipe[s.nome_usuario_responsavel].tarefas.push_back(s); }
    }

    if (equipe.empty()) { cmdHandler_.editAndDelete(event, dpp::message("❌ Sem dados este mês.")); return; }

    std::stringstream prompt;
    prompt << "ELEIÇÃO MELHOR FUNCIONÁRIO DO MÊS " << mes_atual << "/" << ano_atual << "\n\n";
    for (const auto& [nome, stats] : equipe) {
        prompt << "👤 " << nome << " (" << stats.cargo << ")\n" << "- Relatórios: " << stats.relatorios << "\n";
        int fin = 0, atraso = 0, pend = 0;
        for (const auto& s : stats.tarefas) {
            if (s.status == "finalizada") { fin++; if (!s.prazo.empty() && s.prazo != "N/A" && Utils::compararDatas(s.data_finalizacao, s.prazo) == 1) atraso++; }
            else if (s.status == "pendente") pend++;
        }
        prompt << "- Tarefas: " << fin << " OK, " << atraso << " Atraso, " << pend << " Pendentes.\n- Atividades:\n" << stats.logs << "\n-------------------\n";
    }
    prompt << "\nEscolha o MELHOR e justifique.";

    ai_.AskWithContext("Você é um Gestor de RH.", prompt.str(), [this, event](std::string resposta) {
        dpp::embed embed = Utils::criarEmbedPadrao("🏆 Melhor Funcionário do Mês", resposta, dpp::colors::gold);
        embed.set_thumbnail("https://cdn-icons-png.flaticon.com/512/3112/3112946.png");
        event.edit_original_response(dpp::message(event.command.channel_id, embed));
        }, 0.2f);
}

void AICommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand ensinar("ensinar", "Ensina algo novo ao bot (ADM).", bot_id);
    ensinar.add_option(dpp::command_option(dpp::co_string, "info", "A informação nova.", true));
    ensinar.default_member_permissions = dpp::p_administrator;
    commands.push_back(ensinar);

    dpp::slashcommand analise("analise_leads", "Relatório de performance.", bot_id);
    commands.push_back(analise);

    dpp::slashcommand best("melhor_funcionario", "IA elege o destaque do mês.", bot_id);
    best.default_member_permissions = dpp::p_administrator;
    commands.push_back(best);
}