#pragma once

#include <vector>

#include "simrun/core/enums.h"
#include "simrun/util/types.h"

namespace simrun {

class TokenTable {
public:
    TokenTable() = default;
    explicit TokenTable(std::int32_t max_tokens);

    void Reset(std::int32_t max_tokens);

    TokenId AcquireActiveToken();
    void Invalidate(TokenId token_id);

    [[nodiscard]] TokenState State(TokenId token_id) const;
    [[nodiscard]] Generation TokenGeneration(TokenId token_id) const;
    [[nodiscard]] bool IsEventTokenValid(TokenId token_id) const;

    [[nodiscard]] std::int32_t Capacity() const noexcept;

private:
    bool IsValidToken(TokenId token_id) const noexcept;

    std::vector<TokenState> token_state_{};
    std::vector<Generation> token_generation_{};
    TokenId next_token_id_ = 0;
};

}  // namespace simrun
