#pragma once

template <typename T>
class LockGuard
{
public:
	LockGuard(T& cs) : _cs(cs) { _cs.Lock(); }
	~LockGuard() { _cs.Unlock(); }

	T& operator *() { return _cs; }

private:
	T& _cs;
};

template <typename T>
class UnlockGuard
{
public:
	UnlockGuard(T& cs) : _cs(cs) {}
	~UnlockGuard() { _cs.Unlock(); }

private:
	T& _cs;

};