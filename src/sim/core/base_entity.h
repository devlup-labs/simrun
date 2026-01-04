#pragma once
#include <string>

class BaseEntity {
public:
    explicit BaseEntity(std::string id_) : entity_id(std::move(id_)) {}
    virtual ~BaseEntity() = default;

    const std::string& id() const { return entity_id; }

private:
    std::string entity_id;
};
