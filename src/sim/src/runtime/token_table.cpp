#include "simrun/runtime/token_table.h"

#include <stdexcept>

namespace simrun {

TokenTable::TokenTable(const std::int32_t max_tokens) {
    Reset(max_tokens);
}

void TokenTable::Reset(const std::int32_t max_tokens) {
    if (max_tokens <= 0) {
        throw std::invalid_argument("max_tokens must be > 0");
    }
    token_state_.assign(static_cast<std::size_t>(max_tokens), TokenState::kInvalid);
    token_generation_.assign(static_cast<std::size_t>(max_tokens), 0);
    next_token_id_ = 0;
}

TokenId TokenTable::AcquireActiveToken() {
    if (next_token_id_ >= static_cast<TokenId>(token_state_.size())) {
        return kInvalidTokenId;
    }
    const TokenId token_id = next_token_id_;
    token_state_[static_cast<std::size_t>(token_id)] = TokenState::kActive;
    next_token_id_ += 1;
    return token_id;
}

void TokenTable::Invalidate(const TokenId token_id) {
    if (!IsValidToken(token_id)) {
        return;
    }
    token_state_[static_cast<std::size_t>(token_id)] = TokenState::kInvalid;
    token_generation_[static_cast<std::size_t>(token_id)] += 1;
}

TokenState TokenTable::State(const TokenId token_id) const {
    if (!IsValidToken(token_id)) {
        return TokenState::kInvalid;
    }
    return token_state_[static_cast<std::size_t>(token_id)];
}

Generation TokenTable::TokenGeneration(const TokenId token_id) const {
    if (!IsValidToken(token_id)) {
        return 0;
    }
    return token_generation_[static_cast<std::size_t>(token_id)];
}

bool TokenTable::IsEventTokenValid(const TokenId token_id) const {
    if (token_id == kInvalidTokenId) {
        return true;
    }
    return State(token_id) == TokenState::kActive;
}

std::int32_t TokenTable::Capacity() const noexcept {
    return static_cast<std::int32_t>(token_state_.size());
}

bool TokenTable::IsValidToken(const TokenId token_id) const noexcept {
    return token_id >= 0 && static_cast<std::size_t>(token_id) < token_state_.size();
}

}  // namespace simrun
