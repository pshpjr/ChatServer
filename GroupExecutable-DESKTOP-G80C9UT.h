#include "Executable.h"

class Group;
class GroupExecutable : public Executable
{

public:
	GroupExecutable(Group* owner) : _owner(owner) {
		_type = GROUP;
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="arg"> : groupID</param>
	/// <param name="transferred"></param>
	/// <param name="iocp"></param>
	void Execute(ULONG_PTR arg , DWORD transferred, void* iocp) override;

private:
	Group* _owner;
};
