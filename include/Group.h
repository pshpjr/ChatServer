#pragma once

#include <chrono>
#include <queue>
#include "Types.h"
#include "Container.h"
#include "TLSLockFreeQueue.h"
#include "BuildOption.h"
#include "Session.h"
class SendBuffer;
class GroupManager;
class GroupExecutable;
class CRecvBuffer;
class IOCP;

namespace GroupInfo
{
    struct GroupIDHash
    {
        std::size_t operator()(const GroupID& s) const
        {
            return std::hash<unsigned long long>()(static_cast<long>(s));
        }
    };

    // struct GroupIDEqual
    // {
    //     bool operator()(const SessionID& lhs, const SessionID& rhs) const
    //     {
    //         return lhs.id == rhs.id;
    //     }
    // };
}


/// <summary>
/// 구현상의 한계로 인해 생성자에서 할 동작들 중 그룹 관련된 기능(groupID 가지고 오는 등)은
/// OnCreate에서 설정해야 함. 
/// </summary>
class Group
{
    struct GroupJob
    {
        enum type
        {
            Recv
            , Enter
            , Move
            , Leave
        };

        type type{};
        SessionID sessionId{};
        int errCode = -1;
        GroupID dst = GroupID(0);
    };


    friend class GroupExecutable;
    friend class GroupManager;

public:
    /// <summary>
    /// OnCreate를 리턴한 이후부터 해당 그룹으로 이동 가능
    /// </summary>
    virtual void OnCreate()
    {
    };

    virtual void OnUpdate(int milli)
    {
        UNREFERENCED_PARAMETER(milli);
    };

    virtual void OnEnter(SessionID id)
    {
        UNREFERENCED_PARAMETER(id);
    };

    virtual void OnLeave(SessionID id, int wsaErrCode)
    {
        UNREFERENCED_PARAMETER(id);
    };

    virtual void OnRecv(SessionID id, CRecvBuffer& recvBuffer)
    {
        UNREFERENCED_PARAMETER(id);
        UNREFERENCED_PARAMETER(recvBuffer);
    };

    virtual ~Group() = default;

    [[nodiscard]] GroupID GetGroupID() const
    {
        return _groupId;
    }

    [[nodiscard]] int GetQueued() const
    {
        return jobQSize;
    }


    //static constexpr int debugSize = 2001;


    //enum jobType {
    //	Enter,
    //	Leave,
    //	Packet,
    //	Other
    //};
    //struct stDebugData
    //{
    //	SessionID id;
    //	jobType type;
    //	GroupID dst = InvalidGroupID();
    //	String cause;
    //};

    //struct stDebug
    //{
    //	long arrIndex;
    //	stDebugData arr[1000];
    //};
    ////세션 id, 종류
    //stDebug debugArr[debugSize];
    ////tuple<SessionID, String, thread::id> _debugLeave[debugSize];

    //void Write(SessionID id, jobType type,GroupID dst, String cause)
    //{

    //	auto index = InterlockedIncrement(&debugArr[id.index].arrIndex);

    //	if (type == Leave && debugArr[id.index].arr[index - 1].id != id)
    //		__debugbreak();

    //	debugArr[id.index].arr[index].id = id;
    //	debugArr[id.index].arr[index].type = type;
    //	debugArr[id.index].arr[index].dst = dst;
    //	debugArr[id.index].arr[index].cause = cause;

    //}
    void SendPacket(SessionID id, SendBuffer& buffer) const;
    void SendPackets(SessionID id, Vector<SendBuffer>& buffer);

protected:
    void MoveSession(SessionID id, GroupID dst) const;
    void LeaveSession(SessionID id, bool update, int wsaErrCode);
    void EnterSession(SessionID id, bool update);
    void SetLoopMs(int loopMS);

    [[nodiscard]] size_t Sessions() const
    {
        return _sessions.size();
    }

    Group();
    IOCP* _iocp;
    int _loopMs = 10;

    [[nodiscard]] int64 GetWorkTime() const
    {
        return oldWorkTime;
    }

    [[nodiscard]] int64 GetJobTps() const
    {
        return oldHandledJob;
    }

    [[nodiscard]] int32 GetEnterTps() const
    {
        return _enterTps;
    }

    [[nodiscard]] int32 GetLeaveTps() const
    {
        return _leaveTps;
    }
private:
    bool Enqueue(GroupJob job, bool update = true);

    bool NeedUpdate();
    void Update();
    void Execute(IOCP* iocp) const;
    [[nodiscard]] bool hasJob() const;

    bool HandleJob();

    template <typename Header>
    void RecvHandler(Session& session, void* iocp);


    void onRecvPacket(const Session& session, CRecvBuffer& buffer);



private:
    GroupID _groupId = GroupID::InvalidGroupID();
    GroupExecutable& _executable;
    std::chrono::steady_clock::time_point _prevUpdate;
    std::chrono::steady_clock::time_point _nextUpdate;

    SessionSet _sessions;

    LockFreeFixedQueue<GroupJob, 8192> _jobs;
    //TlsLockFreeQueue<GroupJob> _jobs;

    volatile char _isRun = 0;

    GroupManager* _owner;


    long debugIndex = 0;


    //MONITOR
    std::chrono::steady_clock::time_point _nextMonitor;
    int64 workTime = 0;
    int64 _handledJob = 0;
    long jobQSize = 0;
    
    int64 oldWorkTime = 0;
    int64 oldHandledJob = 0;
    long _leaveTps = 0;
    long _handledLeave = 0;
    long _enterTps = 0;
    long _handledEnter = 0;


};
