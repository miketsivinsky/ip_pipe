//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "IP_Pipe.h"

#define IP_PIPE_LIB_EXPORT
#include "IP_PipeLib.h"

namespace IP_pipe
{

//------------------------------------------------------------------------------
void genPipeViewName(const TCHAR* name, const TCHAR* type, TCHAR* fullName)
{
    const size_t MaxNameLen = 80;
    T_IP_PipeView::TType pipeType = (_tcscmp(type,TEXT("rx")) == 0) ? T_IP_PipeView::PipeRx : T_IP_PipeView::PipeTx;
    _tcscpy_s(fullName,MaxNameLen,T_IP_PipeView::genPipeViewName(std::wstring(name),pipeType).c_str());
}

//------------------------------------------------------------------------------
bool isPipeViewExist(const TCHAR* name)
{
    return TPipeViewPool::isPipeViewExist(std::wstring(name));
}

//------------------------------------------------------------------------------
bool createPipeView(const TCHAR* name, const TCHAR* type, uint32_t chunkSize, uint32_t chunkNum)
{
    T_IP_PipeView::TType pipeType = (_tcscmp(type,TEXT("rx")) == 0) ? T_IP_PipeView::PipeRx : T_IP_PipeView::PipeTx;
    return TPipeViewPool::createPipeView(std::wstring(name), pipeType, chunkSize, chunkNum);
}


//------------------------------------------------------------------------------
bool setRdy(const TCHAR* name)
{
    return TPipeViewPool::setRdy(std::wstring(name));
}

//------------------------------------------------------------------------------
TStatus waitPeerRdy(const TCHAR* name, DWORD timeout)
{
    return TPipeViewPool::waitPeerRdy(std::wstring(name),timeout);
}

//------------------------------------------------------------------------------
bool isReady(const TCHAR* name)
{
    return TPipeViewPool::isReady(std::wstring(name));
}

//------------------------------------------------------------------------------
bool isPeerReady(const TCHAR* name)
{
    return TPipeViewPool::isPeerReady(std::wstring(name));
}

//------------------------------------------------------------------------------
bool isPipeReady(const TCHAR* name)
{
    return TPipeViewPool::isPipeReady(std::wstring(name));
}

//------------------------------------------------------------------------------
uint32_t chunkNum(const TCHAR* name)
{
    return TPipeViewPool::chunkNum(std::wstring(name));
}

//------------------------------------------------------------------------------
uint32_t usedChunkNum(const TCHAR* name)
{
    return TPipeViewPool::usedChunkNum(std::wstring(name));
}

//------------------------------------------------------------------------------
bool isBufEmpty(const TCHAR* name)
{
    return TPipeViewPool::isBufEmpty(std::wstring(name));
}

//------------------------------------------------------------------------------
bool isBufFull(const TCHAR* name)
{
    return TPipeViewPool::isBufFull(std::wstring(name));
}

//------------------------------------------------------------------------------
TStatus transferBuf(const TCHAR* name, uint32_t& bufSize, uint8_t* buf, int32_t timeout)
{
    return TPipeViewPool::transferBuf(std::wstring(name),bufSize,buf,timeout);
}

//------------------------------------------------------------------------------
TStatus chunkAccess(const TCHAR* name, uint32_t*& bufSizePtr, uint8_t*& chunk, int32_t timeout)
{
    return TPipeViewPool::chunkAccess(std::wstring(name),bufSizePtr,chunk,timeout);
}

//------------------------------------------------------------------------------
TStatus advanceIdx(const TCHAR* name)
{
    return TPipeViewPool::advanceIdx(std::wstring(name));
}

}