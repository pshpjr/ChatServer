#include "Executable.h"

class Group;
class GroupExecutable : public Executable
{
public:
	GroupExecutable(Group* owner) :_owner(owner) {}

	void Execute(ULONG_PTR arg /*groupID*/, DWORD transferred, void* iocp) override;

private:
	Group* _owner;
};
