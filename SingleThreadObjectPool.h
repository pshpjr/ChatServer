#pragma once

#pragma once
#include <new.h>
#include <memory>
class Node;

template <typename data, int dataId, bool usePlacement>
class TLSPool;


/**
 * \brief
 * \tparam data
 * \tparam dataId : short이내의 int 값이어야 함.
 * \tparam usePlacement  : 매번 생성자 호출할건지 여부. 기본값은 false
 */
template <typename data, int dataId, bool usePlacement = false>
class SingleThreadObjectPool
{
	friend class TLSPool<data, dataId, usePlacement>;

public:
	struct Node
	{
		Node* _head = nullptr;
		data _data;
		Node* _tail = nullptr;
	};

	SingleThreadObjectPool(int iBlockNum);

	virtual	~SingleThreadObjectPool();


	data* Alloc(void);


	bool Free(data* pdata);

	int		GetAllocCount(void) const { return _allocCount; }

	int		GetObjectCount(void) const { return _objectCount; }

	void SizeCheck() const {
		int size = 0;
		Node* _cur = _top;
		while (_cur != nullptr) {
			_cur = _cur->_tail;
			size++;
		
		}

		ASSERT_CRASH(_objectCount == size);
	}

private:
	Node* _top = nullptr;
	int _objectCount = 0;
	int _allocCount = 0;
	static constexpr int _identifier = dataId << 16;
	static int _instanceCount;
};
template <typename data, int dataId, bool usePlacement>
int SingleThreadObjectPool<data, dataId, usePlacement>::_instanceCount = 0;

//placement가 false일 때 생성 속도가 좀 느려도(루프 두 번) 코드 깔끔한 게 더 좋을 것 같음. 
template <typename data, int dataId, bool usePlacement>
SingleThreadObjectPool<data, dataId, usePlacement>::SingleThreadObjectPool(int iBlockNum)
{
	for (int i = 0; i < iBlockNum; ++i)
	{
		Node* newNode = (Node*)malloc(sizeof(Node));

		if constexpr (usePlacement == false) 
		{
			new(newNode) Node();
		}


		newNode->_tail = _top;
		_top = newNode;
	}


	_objectCount = iBlockNum;
	_allocCount = iBlockNum;
}

template <typename data, int dataId, bool usePlacement>
SingleThreadObjectPool<data, dataId, usePlacement>::~SingleThreadObjectPool()
{
	Node* node = _top;
	while (node != nullptr)
	{
		Node* tar = node;
		node = node->_tail;

		if constexpr (usePlacement)
		{
			free(tar);
		}
		else
		{
			delete tar;
		}
	}
}

template <typename data, int dataId, bool usePlacement>
data* SingleThreadObjectPool<data, dataId, usePlacement>::Alloc()
{
	Node* retNode = _top;
	if (retNode != nullptr)
	{
		_top = _top->_tail;
		_objectCount--;

		if constexpr (usePlacement)
		{
			retNode = (Node*)new(&retNode->_data)data();
		}
	}
	else
	{
		_allocCount++;
		retNode = new Node;
	}
#ifdef MYDEBUG
	retNode->_tail = (Node*)0x3412;
	retNode->_head = (Node*)0x3412;
#endif
	SizeCheck();


	return (data*)(&(retNode->_data));
}

template <typename data, int dataId, bool usePlacement>
bool SingleThreadObjectPool<data, dataId, usePlacement>::Free(data* pdata)
{
	Node* dataNode = (Node*)((char*)pdata - offsetof(Node, _data));

	//정상인지 테스트
#ifdef MYDEBUG
	if (dataNode->_head == (Node*)0x3412)
	{
		printf("correct\n");
	}
	if (dataNode->_tail == (Node*)0x3412)
	{
		printf("correct\n");
	}
#endif

	if constexpr (usePlacement)
	{
		pdata->~data();
	}

	dataNode->_tail = _top;
	_top = dataNode;

	_objectCount++;

	SizeCheck();


	return true;
}



