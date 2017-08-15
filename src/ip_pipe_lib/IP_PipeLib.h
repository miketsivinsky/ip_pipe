//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef IP_PIPE_LIB
#define IP_PIPE_LIB

#include <windows.h>
#include <tchar.h>
#include <cstdint>
#include <string>

#ifdef __cplusplus
    extern "C" {
#endif

#include "IP_PipeDefs.h"

#ifdef IP_PIPE_LIB_EXPORT
    #define IP_PIPE_DLL_API __declspec(dllexport)
#else
    #define IP_PIPE_DLL_API __declspec(dllimport)
#endif

namespace IP_pipe
{
    IP_PIPE_DLL_API void genPipeViewName(const TCHAR* name, const TCHAR* type, TCHAR* fullName);

    IP_PIPE_DLL_API bool isPipeViewExist(const TCHAR* name);
    IP_PIPE_DLL_API bool createPipeView(const TCHAR* name, const TCHAR* type, uint32_t chunkSize, uint32_t chunkNum);

    IP_PIPE_DLL_API bool setRdy(const TCHAR* name);
    IP_PIPE_DLL_API TStatus waitPeerRdy(const TCHAR* name, DWORD timeout = INFINITE);
    IP_PIPE_DLL_API bool isReady(const TCHAR* name);
    IP_PIPE_DLL_API bool isPeerReady(const TCHAR* name);
    IP_PIPE_DLL_API bool isPipeReady(const TCHAR* name);

    IP_PIPE_DLL_API uint32_t chunkNum(const TCHAR* name);
    IP_PIPE_DLL_API uint32_t usedChunkNum(const TCHAR* name);
    IP_PIPE_DLL_API bool isBufEmpty(const TCHAR* name);
    IP_PIPE_DLL_API bool isBufFull(const TCHAR* name);

    IP_PIPE_DLL_API TStatus transferBuf(const TCHAR* name, uint32_t& bufSize, uint8_t* buf, int32_t timeout = INFINITE);
    IP_PIPE_DLL_API TStatus chunkAccess(const TCHAR* name, uint32_t*& bufSizePtr, uint8_t*& chunk, int32_t timeout = INFINITE);
    IP_PIPE_DLL_API TStatus advanceIdx(const TCHAR* name);
}

#ifdef __cplusplus
    }
#endif

#endif /* IP_PIPE_LIB */

