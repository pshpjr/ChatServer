#include <variant>

#pragma once

template <typename T,typename E>
class Result
{
public:
    using valueType = T;
    using errorType = E;  // Assuming error codes are represented by integers

    // Enum to track the current state of the Result
    enum class State {
        HasValue,
        HasError
    };

    Result(valueType value)
        : state_(State::HasValue), variant_(std::move(value)) {
    }

    Result(errorType error)
        : state_(State::HasError), variant_(error) {
    }

    bool HasValue() const {
        return state_ == State::HasValue;
    }

    valueType Value() {
        if (state_ == State::HasValue) {
            return std::get<valueType>(variant_);
        } else {
            throw std::runtime_error("Error: Result does not contain a value");
        }
    }

    bool HasError() const {
        return state_ == State::HasError;
    }

    errorType Error() {
        if (state_ == State::HasError) {
            return std::get<errorType>(variant_);
        } else {
            throw std::runtime_error("Error: Result does not contain an error");
        }
    }

private:
    State state_;
    std::variant<valueType, errorType> variant_;
};