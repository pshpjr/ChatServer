//deprecated
class WaitExecutable : public Executable
{
public:
    ~WaitExecutable() override = default;

    void Execute(const ULONG_PTR key, DWORD transferred, void* iocp) override
    {
        _execute(key, iocp);

        isDone = true;
        WakeByAddressSingle(&isDone);
    }

    void Join()
    {
        char expect = false;
        WaitOnAddress(&isDone, &expect, sizeof(long), INFINITE);
    }

protected:
    virtual void _execute(ULONG_PTR key, void* iocp) = 0;

private:
    char isDone = false;
};

//deprecated
template <typename T>
class ExecutableManager
{
    static_assert(std::is_base_of_v<WaitExecutable, T>);

public:
    ExecutableManager(int count, const HANDLE iocp) : _iocp(iocp)
    {
        _executables.resize(count);
    }

    void Run(const ULONG_PTR arg)
    {
        for (auto& exe : _executables)
        {
            int transfered = 1;
            PostQueuedCompletionStatus(_iocp, transfered, arg, exe.GetOverlapped());
        }
    }

    void Wait()
    {
        for (auto& exe : _executables)
        {
            exe.Join();
        }
    }

private:
    HANDLE _iocp = INVALID_HANDLE_VALUE;
    vector<T> _executables;
};
