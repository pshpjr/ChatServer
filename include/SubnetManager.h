#pragma once
#include <optional>
#include<vector>

#include "SettingParser.h"

class SubnetManager
{
    struct SubnetInfo
    {
        char subnetSize;
        IN_ADDR address;
        String addressStr;
    };

public:
    SubnetManager();

    bool IsLan(const SockAddr_in& info);

    bool IsWan(const SockAddr_in& info);

    std::optional<SubnetInfo> FindMatchSubnet(const SockAddr_in info)
    {
        if (auto subnetResult = FindSubnet(_lanSubnet, info);
            subnetResult.has_value())
        {
            return subnetResult;
        }
        return FindSubnet(_wanSubnet, info);
    }

private:
    std::optional<SubnetInfo> FindSubnet(Vector<SubnetInfo>& group, SockAddr_in info);

    void LoadSubnets();
    void LoadSubnetGroup(String group, Vector<SubnetInfo>& container);

    SettingParser _subnetParser;
    Vector<SubnetInfo> _lanSubnet;
    Vector<SubnetInfo> _wanSubnet;
};
