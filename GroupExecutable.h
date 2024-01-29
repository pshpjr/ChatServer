#include "Executable.h"

class Group;
class GroupExecutable : public Executable
{

	static constexpr auto EXECUTION_INTERVAL = 3ms;
public:
	GroupExecutable(Group* owner) :_owner(owner),nextRun(std::chrono::steady_clock::now()) {}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="arg"> : groupID</param>
	/// <param name="transferred"></param>
	/// <param name="iocp"></param>
	void Execute(ULONG_PTR arg , DWORD transferred, void* iocp) override;

private:
	Group* _owner;
	std::chrono::steady_clock::time_point nextRun {};
};
