#include "stdafx.h"
#include "ContentJob.h"
#include "CSerializeBuffer.h"
TLSPool<ContentJob, 0, false> ContentJob::_pool;

void ContentJob::Free()
{
	_buffer->Release();
	
	_pool.Free(this);
}

ContentJob* ContentJob::Alloc(CSerializeBuffer* buffer)
{
	auto ret = _pool.Alloc();

	ret->_buffer = buffer;
	buffer->IncreaseRef();
	return ret;
}
