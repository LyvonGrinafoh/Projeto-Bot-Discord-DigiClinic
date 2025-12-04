#pragma once

#include <map>
#include <string>
#include <vector>
#include <optional>
#include "DataTypes.h"

class DatabaseManager {
private:
    std::map<uint64_t, Solicitacao> solicitacoes_;
    std::map<uint64_t, Lead> leads_;
    std::map<uint64_t, Compra> compras_;
    std::map<uint64_t, Visita> visitas_;
    std::map<uint64_t, Placa> placas_;
    std::map<uint64_t, EstoqueItem> estoque_;
    std::map<uint64_t, RelatorioDiario> relatorios_;

    template<typename T>
    bool loadFromFile(const std::string& filename, std::map<uint64_t, T>& database);

    template<typename T>
    bool saveToFile(const std::string& filename, const std::map<uint64_t, T>& database);

public:
    DatabaseManager() = default;

    bool loadAll();
    bool saveAll();

    // Solicitacao
    const std::map<uint64_t, Solicitacao>& getSolicitacoes() const;
    std::optional<Solicitacao> getSolicitacao(uint64_t id) const;
    bool addOrUpdateSolicitacao(const Solicitacao& s);
    bool removeSolicitacao(uint64_t id);
    void clearSolicitacoes();
    bool saveSolicitacoes();

    // Lead
    const std::map<uint64_t, Lead>& getLeads() const;
    std::optional<Lead> getLead(uint64_t id) const;
    Lead* getLeadPtr(uint64_t id);
    Lead* findLeadByContato(const std::string& contato);
    bool addOrUpdateLead(const Lead& l);
    bool removeLead(uint64_t id);
    void clearLeads();
    bool saveLeads();

    // Compra
    const std::map<uint64_t, Compra>& getCompras() const;
    std::optional<Compra> getCompra(uint64_t id) const;
    bool addOrUpdateCompra(const Compra& c);
    bool removeCompra(uint64_t id);
    void clearCompras();
    bool saveCompras();

    // Visita
    const std::map<uint64_t, Visita>& getVisitas() const;
    std::optional<Visita> getVisita(uint64_t id) const;
    Visita* getVisitaPtr(uint64_t id);
    bool addOrUpdateVisita(const Visita& v);
    bool removeVisita(uint64_t id);
    void clearVisitas();
    bool saveVisitas();

    // Placa
    const std::map<uint64_t, Placa>& getPlacas() const;
    std::optional<Placa> getPlaca(uint64_t id) const;
    Placa* getPlacaPtr(uint64_t id);
    bool addOrUpdatePlaca(const Placa& p);
    bool removePlaca(uint64_t id);
    void clearPlacas();
    bool savePlacas();

    // Estoque
    const std::map<uint64_t, EstoqueItem>& getEstoque() const;
    std::optional<EstoqueItem> getEstoqueItem(uint64_t id) const;
    EstoqueItem* getEstoqueItemPtr(uint64_t id);
    EstoqueItem* getEstoqueItemPorNome(const std::string& nome);
    bool addOrUpdateEstoqueItem(const EstoqueItem& item);
    bool removeEstoqueItem(uint64_t id);
    bool saveEstoque();

    // Relatorios Diarios
    const std::map<uint64_t, RelatorioDiario>& getRelatorios() const;
    bool addOrUpdateRelatorio(const RelatorioDiario& r);
    bool saveRelatorios();
};