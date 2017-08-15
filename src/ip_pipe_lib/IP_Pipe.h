//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if !defined(IP_PIPE)
#define IP_PIPE
#include <windows.h>
#include <tchar.h>
#include <cstdint>
#include <string>
#include <map>

#include "SU.h"
#include "IP_PipeDefs.h"

#define USE_BLOCKING_WRITE
#define USE_BLOCKING_READ

#define TRANSFER_BUF_IMPL 2

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class T_IP_PipeView
{   
    public:
        enum TType { PipeRx, PipeTx };

        T_IP_PipeView(const SU::TString& fileName, uint32_t chunkSize, uint32_t chunkNum);
        virtual ~T_IP_PipeView();

		const SU::TString& nameStr() const { return mName; }
		const TCHAR* name() const { return nameStr().c_str(); }
        virtual bool isRx() const = 0;
        bool isTx() const { return !isRx(); }
        static SU::TString error(IP_pipe::TStatus errStatus) { return ErrorList[errStatus]; }
        static SU::TString genPipeViewName(const SU::TString& name, TType type) {
            return (type == PipeRx) ? SU::TString(name + SU::TString(TEXT("_rx"))) : SU::TString(name + SU::TString(TEXT("_tx")));
        }

		virtual void setRdy() = 0;
        virtual IP_pipe::TStatus waitPeerRdy(DWORD timeout = INFINITE) = 0;
        virtual bool isReady() = 0;
        virtual bool isPeerReady() = 0;
        bool isPipeReady() { return isReady() && isPeerReady(); }

        uint32_t chunkNum() { return mChunkNum; }
        uint32_t usedChunkNum();
        bool isBufEmpty() { return (usedChunkNum() == 0); }
        bool isBufFull() { return (usedChunkNum() == chunkNum()); }

        virtual IP_pipe::TStatus transferBuf(uint32_t& bufSize, uint8_t* buf, int32_t timeout = INFINITE) = 0;
		virtual IP_pipe::TStatus chunkAccess(uint32_t*& bufSizePtr, uint8_t*& chunk, int32_t timeout = INFINITE) = 0;
		virtual IP_pipe::TStatus advanceIdx() = 0;

    protected:
        //static const unsigned StrBufLen = 64;
        
        //---
        struct THandShakeTable {
                                BYTE rdyTx;
                                BYTE rdyRx;
                                BYTE rsv[6];
                              };

        template<typename T> T* handShakeTablePtr() { return reinterpret_cast<T*>(mFileView); }
        THandShakeTable& handShakeTable() { return *(handShakeTablePtr<THandShakeTable>()); }

        //---
        struct TCircBufferParams
        {
            typedef uint32_t TPayloadSize;

            uint32_t mChunkSize;
            uint32_t mChunkNum;
            uint32_t mReadIdx;
            uint32_t mWriteIdx;
        };

        template<typename T> T* circBufParamsPtr() { return reinterpret_cast<T*>(handShakeTablePtr<THandShakeTable>() + 1); }
        TCircBufferParams& circBufParams() { return *(circBufParamsPtr<TCircBufferParams>()); }
        template<typename T> T* circBufPtr() { return reinterpret_cast<T*>(circBufParamsPtr<TCircBufferParams>() + 1); }
        template<typename T> T* chunkPtr(unsigned idx, unsigned byteOffset = 0) {
            return reinterpret_cast<T*>(circBufPtr<uint8_t>() + idx*(mChunkSize + sizeof(TCircBufferParams::TPayloadSize)) + byteOffset);
        }
        uint32_t& payloadSize(unsigned idx) { return *(chunkPtr<uint32_t>(idx)); }
        uint32_t* payloadSizePtr(unsigned idx) { return chunkPtr<uint32_t>(idx); }
        uint32_t incIdx(uint32_t idx) { return ((idx + 1) > chunkNum()) ? 0 : (idx + 1); }
        
        SU::TString    mName;
        HANDLE         mFile;
        HANDLE         mFileMapping;
        uint32_t       mChunkSize;
        uint32_t       mChunkNum;
        LPVOID         mFileView;

        #if defined(USE_BLOCKING_WRITE)
            SU::TSemaphore mSemTx;
        #endif
        #if defined(USE_BLOCKING_READ)
            SU::TSemaphore mSemRx;
        #endif
        SU::TSemaphore mSemTxRdy;
        SU::TSemaphore mSemRxRdy;

        //static TCHAR* GenSemName(const TCHAR* fileName, const TCHAR* semName);
        BYTE& hsTxRdy() { return handShakeTable().rdyTx; }
        BYTE& hsRxRdy() { return handShakeTable().rdyRx; }
        IP_pipe::TStatus translateWaitResult(DWORD waitResult);
        virtual void clearRdy() = 0;

    private:
        static const SU::TString ErrorList[IP_pipe::OtherError+1];
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class T_IP_PipeViewRx : public T_IP_PipeView
{
    public:
        T_IP_PipeViewRx(const SU::TString& fileName, uint32_t chunkSize, uint32_t chunkNum);
        ~T_IP_PipeViewRx();

        void setRdy() {
            handShakeTable().rdyRx = 1;
            mSemRxRdy.release();
        }
        IP_pipe::TStatus waitPeerRdy(DWORD timeout = INFINITE) { return translateWaitResult(mSemTxRdy.wait(timeout)); }
        bool isRx() const { return true; }
        bool isReady() { return (handShakeTable().rdyRx == 1); }
        bool isPeerReady() { return (handShakeTable().rdyTx == 1); }
        IP_pipe::TStatus transferBuf(uint32_t& bufSize, uint8_t* buf, int32_t timeout = INFINITE);
		IP_pipe::TStatus chunkAccess(uint32_t*& bufSizePtr, uint8_t*& chunk, int32_t timeout = INFINITE);
		IP_pipe::TStatus advanceIdx();

    protected:
        void clearRdy() { handShakeTable().rdyRx = 0; }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class T_IP_PipeViewTx : public T_IP_PipeView
{
    public:
        T_IP_PipeViewTx(const SU::TString& fileName, uint32_t chunkSize, uint32_t chunkNum);
        ~T_IP_PipeViewTx();
        void setRdy() {
            handShakeTable().rdyTx = 1;
            mSemTxRdy.release();
        }
        IP_pipe::TStatus waitPeerRdy(DWORD timeout = INFINITE) { return translateWaitResult(mSemRxRdy.wait(timeout)); }
        bool isRx() const { return false; }
        bool isReady() { return (handShakeTable().rdyTx == 1); }
        bool isPeerReady() { return (handShakeTable().rdyRx == 1); }
        IP_pipe::TStatus transferBuf(uint32_t& bufSize, uint8_t* buf, int32_t timeout = INFINITE);
		IP_pipe::TStatus chunkAccess(uint32_t*& bufSizePtr, uint8_t*& chunk, int32_t timeout = INFINITE);
		IP_pipe::TStatus advanceIdx();

    protected:
        void clearRdy() { handShakeTable().rdyTx = 0; }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class TPipeViewPool
{
    public:
        static bool isPipeViewExist(const SU::TString& name) { return (getPipeView(name) != 0); }
        static bool createPipeView(const SU::TString& name, T_IP_PipeView::TType pipeType, uint32_t chunkSize, uint32_t chunkNum);

        static bool setRdy(const SU::TString& name);
        static IP_pipe::TStatus waitPeerRdy(const SU::TString& name, DWORD timeout = INFINITE);
        static bool isReady(const SU::TString& name);
        static bool isPeerReady(const SU::TString& name);
        static bool isPipeReady(const SU::TString& name);

        static int32_t chunkNum(const SU::TString& name);
        static int32_t usedChunkNum(const SU::TString& name);
        static bool isBufEmpty(const SU::TString& name);
        static bool isBufFull(const SU::TString& name);

        static IP_pipe::TStatus transferBuf(const SU::TString& name, uint32_t& bufSize, uint8_t* buf, int32_t timeout = INFINITE);
		static IP_pipe::TStatus chunkAccess(const SU::TString& name, uint32_t*& bufSizePtr, uint8_t*& chunk, int32_t timeout = INFINITE);
		static IP_pipe::TStatus advanceIdx(const SU::TString& name);

    private:
        typedef std::map<SU::TString,T_IP_PipeView*> TPipeViewPoolMap;

        static TPipeViewPoolMap& instance() {
            static TPipeViewPool Pool;
            return Pool.mPipeViewPool;
        }
        TPipeViewPool() : mPipeViewPool() { }
        ~TPipeViewPool();
        static T_IP_PipeView* getPipeView(const SU::TString& name);

        TPipeViewPoolMap    mPipeViewPool;
};

#endif /* IP_PIPE */

