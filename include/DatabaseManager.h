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

    template<typename T>
    bool loadFromFile(const std::string& filename, std::map<uint64_t, T>& database);

    template<typename T>
    bool saveToFile(const std::string& filename, const std::map<uint64_t, T>& database);

public:
    DatabaseManager() = default;

    bool loadAll();

    bool saveAll();

    const std::map<uint64_t, Solicitacao>& getSolicitacoes() const;
    std::optional<Solicitacao> getSolicitacao(uint64_t id) const;
    bool addOrUpdateSolicitacao(const Solicitacao& s);
    bool removeSolicitacao(uint64_t id);
    void clearSolicitacoes();
    bool saveSolicitacoes();

    const std::map<uint64_t, Lead>& getLeads() const;
    std::optional<Lead> getLead(uint64_t id) const;
    Lead* getLeadPtr(uint64_t id);
    bool addOrUpdateLead(const Lead& l);
    bool removeLead(uint64_t id);
    void clearLeads();
    bool saveLeads();

    const std::map<uint64_t, Compra>& getCompras() const;
    std::optional<Compra> getCompra(uint64_t id) const;
    bool addOrUpdateCompra(const Compra& c);
    bool removeCompra(uint64_t id);
    void clearCompras();
    bool saveCompras();

    const std::map<uint64_t, Visita>& getVisitas() const;
    std::optional<Visita> getVisita(uint64_t id) const;
    Visita* getVisitaPtr(uint64_t id);
    bool addOrUpdateVisita(const Visita& v);
    bool removeVisita(uint64_t id);
    void clearVisitas();
    bool saveVisitas();
};