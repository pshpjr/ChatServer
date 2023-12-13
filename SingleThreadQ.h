#pragma once
#include "SingleThreadObjectPool.h"

template<typename T>
class SingleThreadQ
{
	struct Node
	{
		T data;
		Node* next;
	};

public:
	SingleThreadQ():_pool(10000)
	{

	}


	void Enqueue(T data)
	{

		auto newNode = _pool.Alloc();
		newNode->data = data;

		if ( Size() == 0 )
		{
			_front = newNode;
			_rear = newNode;

		}
		else
		{
			_rear->next = newNode;
			_rear = newNode;
		}

		_size++;
	}

	bool Dequeue(T& data)
	{
		if ( Size() == 0 )
		{
			return false;
		}
		auto ret = _front;
		data = ret->data;

		if ( Size() == 1 )
		{
			_front = nullptr;
			_rear = nullptr;
		}
		else
		{
			_front = _front->next;
		}

		_pool.Free(ret);
		_size--;
		return true;
	}

	int Size() const { return _size; }

	bool empty() const { return _size == 0; }
private:
	//front 에서 뺌
	SingleThreadObjectPool<Node,0,false> _pool;
	int _size = 0;
	Node* _front;
	Node* _rear;
};

