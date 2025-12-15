#include "AICommands.h"
#include "AIHandler.h"
#include "DatabaseManager.h"
#include "CommandHandler.h"
#include "Utils.h"
#include <sstream>
#include <map>
#include <ctime>
#include <iomanip>

const std::string MANUAL_DO_BOT = R"(
VOCÊ É O SUPORTE TÉCNICO E ANALISTA DA 'DIGITAL CLINIC'.
Responda em Português do Brasil profissional.

--- DOCUMENTAÇÃO DE COMANDOS ---
1. LEADS
   - /adicionar_lead: Cadastra novo cliente.
   - /listar_leads: Lista paginada (filtros: todos, nao_contatados).
   - /modificar_lead: Altera status ou dados.
   - /ver_lead: Mostra ficha completa.

2. VISITAS
   - /visitas: Marca visita (Doutor, Data, Unidade).
   - /finalizar_visita: Marca realizada + anexo.
   - /cancelar_visita: Cancela.
   - /lista_visitas: Vê agenda.

3. ESTOQUE & COMPRAS
   - /estoque_repor: Adiciona qtd.
   - /estoque_baixa: Remove qtd.
   - /registrar_gasto: Lança despesa.

4. TAREFAS
   - /demanda: Cria tarefa.
   - /pedido: Solicita compra.
   - /relatorio_do_dia: Envia o relatório diário de atividades.

--- REGRAS ---
- Explique qual comando usar para a dúvida do usuário.
- Não invente comandos.
)";

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
        int dia, mes, ano; char barra; std::stringstream ss(data); ss >> dia >> barra >> mes >> barra >> ano; return { mes, ano };
    }
    catch (...) { return { 0, 0 }; }
}

AICommands::AICommands(dpp::cluster& bot, AIHandler& ai, DatabaseManager& db, CommandHandler& handler) :
    bot_(bot), ai_(ai), db_(db), cmdHandler_(handler) {
}

void AICommands::handle_ia_duvida(const dpp::slashcommand_t& event) {
    event.thinking(false);
    std::string pergunta = std::get<std::string>(event.get_parameter("pergunta"));

    ai_.AskWithContext(MANUAL_DO_BOT, pergunta, [this, event](std::string resposta) {
        dpp::embed embed = Utils::criarEmbedPadrao("🤖 Suporte Inteligente", resposta, dpp::colors::purple);
        cmdHandler_.editAndDelete(event, dpp::message().add_embed(embed), 180);
        });
}

void AICommands::handle_analise_leads(const dpp::slashcommand_t& event) {
    event.thinking(false);

    const auto& leads = db_.getLeads();
    if (leads.empty()) {
        cmdHandler_.editAndDelete(event, dpp::message("❌ Sem leads para analisar."));
        return;
    }

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
            if (l.status_conversa == "Fechamento" || l.tipo_fechamento != "N/A" || l.valor_fechamento > 0) target->fechamentos++;
            if (target->amostra_obs.size() < 5 && l.observacoes.length() > 10 && l.observacoes != "Nenhuma") {
                target->amostra_obs.push_back(l.observacoes.substr(0, 100));
            }
        }
    }

    std::stringstream prompt;
    prompt << "Analise os dados de vendas da 'Digital Clinic'. Compare Mês " << mes_atual << " vs Mês " << mes_anterior << ".\n";
    prompt << "ATUAL: " << s_atual.total << " leads, " << s_atual.fechamentos << " vendas.\n";
    prompt << "ANTERIOR: " << s_anterior.total << " leads, " << s_anterior.fechamentos << " vendas.\n";
    prompt << "OBSERVAÇÕES RECENTES:\n";
    for (const auto& o : s_atual.amostra_obs) prompt << "- \"" << o << "\"\n";
    prompt << "\nDIRETRIZES: Seja direto. Aponte crescimento ou queda. Dê uma dica estratégica.";

    ai_.AskWithContext("Você é um Diretor Comercial Sênior. Fale em Português Formal.", prompt.str(), [this, event, mes_atual, mes_anterior](std::string resposta) {
        std::string titulo = "📊 Relatório Comercial (" + std::to_string(mes_atual) + "/" + std::to_string(mes_anterior) + ")";
        dpp::embed embed = Utils::criarEmbedPadrao(titulo, resposta, dpp::colors::gold);
        cmdHandler_.editAndDelete(event, dpp::message().add_embed(embed), 600);
        }, 0.4f);
}

void AICommands::handle_melhor_funcionario(const dpp::slashcommand_t& event) {
    event.thinking(false);

    auto relatorios = db_.getTodosRelatorios();
    if (relatorios.empty()) {
        cmdHandler_.editAndDelete(event, dpp::message("❌ Nenhum relatório encontrado."));
        return;
    }

    struct Performance { std::string cargo; int qtd_relatorios = 0; std::string resumo_atividades; };
    std::map<std::string, Performance> equipe;

    for (const auto& r : relatorios) {
        Performance& p = equipe[r.username];
        if (p.cargo.empty()) p.cargo = getCargoFuncionario(r.username);

        p.qtd_relatorios++;

        std::string trecho = r.conteudo;
        if (trecho.length() > 100) trecho = trecho.substr(0, 100) + "...";

        p.resumo_atividades += "- " + r.data_hora + ": " + trecho + "\n";
    }

    std::stringstream prompt;
    prompt << "Analise os relatórios diários da equipe e escolha o FUNCIONÁRIO DESTAQUE.\n\n";

    for (const auto& [nome, stats] : equipe) {
        prompt << "👤 " << nome << " (" << stats.cargo << ")\n";
        prompt << "📊 Entregas: " << stats.qtd_relatorios << "\n";
        std::string atv = stats.resumo_atividades;
        if (atv.length() > 800) atv = atv.substr(0, 800) + " [cortado]";
        prompt << atv << "\n-------------------\n";
    }

    prompt << "\nTAREFA: 1. Analise produtividade. 2. Escolha o Vencedor. 3. Justifique. 4. Menção honrosa para o 2º lugar.";

    ai_.AskWithContext("Você é um Gestor de RH rigoroso.", prompt.str(), [this, event](std::string resposta) {
        dpp::embed embed = Utils::criarEmbedPadrao("🏆 Veredito: Melhor Funcionário", resposta, dpp::colors::gold);
        embed.set_thumbnail("https://cdn-icons-png.flaticon.com/512/3112/3112946.png");
        event.edit_original_response(dpp::message(event.command.channel_id, embed));
        }, 0.2f);
}

void AICommands::addCommandDefinitions(std::vector<dpp::slashcommand>& commands, dpp::snowflake bot_id) {
    dpp::slashcommand ia("ia", "Dúvidas sobre o bot.", bot_id);
    ia.add_option(dpp::command_option(dpp::co_string, "pergunta", "Sua dúvida.", true));
    commands.push_back(ia);

    dpp::slashcommand analise("analise_leads", "Relatório de performance.", bot_id);
    commands.push_back(analise);

    dpp::slashcommand best("melhor_funcionario", "IA elege o destaque.", bot_id);
    best.default_member_permissions = dpp::p_administrator;
    commands.push_back(best);
}