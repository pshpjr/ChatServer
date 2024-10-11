//
// Created by pshpj on 24. 9. 2.
//

#ifndef GROUPTYPES_H
#define GROUPTYPES_H
#include "Types.h"
#include "MyWindows.h"
#include <format>

class GroupID
{
public:
    explicit constexpr GroupID(const int id)
        : _id(id) {}

    static GroupID NextID()
    {
        return GroupID(InterlockedIncrement(&g_GroupID));
    }

    static consteval GroupID InvalidGroupID()
    {
        return GroupID{-1};
    }

    explicit operator long() const
    {
        return _id;
    }

    friend bool operator==(const GroupID& lhs, const GroupID& rhs)
    {
        return lhs._id == rhs._id;
    }

    friend bool operator!=(const GroupID& lhs, const GroupID& rhs)
    {
        return !(lhs == rhs);
    }

    friend std::hash<GroupID>;
    friend std::equal_to<GroupID>;

private:
    long _id;
    inline static long g_GroupID = 0;
};


namespace std
{
    template <>
    struct hash<GroupID>
    {
    public:
        size_t operator()(const GroupID& id) const noexcept
        {
            return std::hash<long>()(id._id);
        }
    };

    //GroupID 포멧터 사용 가능하게
    template <>
    struct std::formatter<GroupID, wchar_t>
    {
        std::formatter<long, wchar_t> long_formatter;

        constexpr auto parse(std::wformat_parse_context& ctx) -> decltype(ctx.begin())
        {
            return long_formatter.parse(ctx);
        }

        auto format(const GroupID& gid, std::wformat_context& ctx) const -> decltype(ctx.out())
        {
            long id = static_cast<long>(gid);
            return long_formatter.format(id, ctx);
        }
    };
}
#endif //GROUPTYPES_H
