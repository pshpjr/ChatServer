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

	optional<SubnetInfo> FindMatchSubnet(const SockAddr_in info)
	{
		if ( auto subnetResult = FindSubnet(_lanSubnet, info); subnetResult.has_value() )
		{
			return subnetResult;
		}
		else
		{
			return FindSubnet(_wanSubnet, info);
		}
	}

private:
	optional<SubnetInfo> FindSubnet(vector<SubnetInfo>& group, const SockAddr_in info);

private:
	void LoadSubnets();
	void LoadSubnetGroup(String group, vector<SubnetInfo>& container);

private:
	SettingParser _subnetParser;
	vector<SubnetInfo> _lanSubnet;
	vector<SubnetInfo> _wanSubnet;
};

