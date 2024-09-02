#include "GroupExecutable.h"
#include "Group.h"

void GroupExecutable::Execute(ULONG_PTR arg, DWORD transferred, void* iocp)
{
    UNREFERENCED_PARAMETER(arg);
    UNREFERENCED_PARAMETER(transferred);
    UNREFERENCED_PARAMETER(iocp);
    _owner->Update();
}
