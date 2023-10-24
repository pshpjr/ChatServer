#include "stdafx.h"
#include "ContentJob.h"
#include "CSerializeBuffer.h"
TLSPool<ContentJob, 0, false> ContentJob::_pool;

void ContentJob::Free()
{
	_buffer->Release();
	
	_pool.Free(this);
}
