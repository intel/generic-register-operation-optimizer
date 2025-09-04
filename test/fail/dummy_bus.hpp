#pragma once

#include <async/concepts.hpp>

#include <stdx/ct_string.hpp>

struct dummy_bus {
    struct dummy_sender {
        using is_sender = void;
    };
    template <stdx::ct_string, auto>
    static auto read(auto...) -> async::sender auto {
        return dummy_sender{};
    }
    template <stdx::ct_string, auto...>
    static auto write(auto...) -> async::sender auto {
        return dummy_sender{};
    }
};
