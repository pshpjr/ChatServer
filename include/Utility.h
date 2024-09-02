//
// Created by Park on 24. 8. 22.
//

#ifndef UTILITY_H
#define UTILITY_H
#include <string>
#include <variant>

namespace psh::util
{
    std::string WToS(const std::wstring& wstr) ;

}

namespace psh::util
{
    template <typename T,typename E>
    class Result {
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

        operator bool()
        {
            return state_ == State::HasValue;
        }

        bool HasValue() const {
            return this;
        }

        valueType Value() {
            if (state_ == State::HasValue) {
                return std::get<valueType>(variant_);
            } else {
                throw std::exception("Error: Result does not contain a value");
            }
        }

        bool HasError() const {
            return state_ == State::HasError;
        }

        errorType Error() {
            if (state_ == State::HasError) {
                return std::get<errorType>(variant_);
            } else {
                throw std::exception("Error: Result does not contain an error");
            }
        }

    private:
        State state_;
        std::variant<valueType, errorType> variant_;
    };



}




#endif //UTILITY_H
