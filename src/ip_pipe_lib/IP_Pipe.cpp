//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <cstdio>

#include "IP_Pipe.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const SU::TString T_IP_PipeView::ErrorList[] = 
{
			SU::TString(TEXT("Ok")),
			SU::TString(TEXT("NotReady")),
			SU::TString(TEXT("TimeoutExpired")),
			SU::TString(TEXT("PipeEmpty")),
			SU::TString(TEXT("PipeFull")),
			SU::TString(TEXT("BadBufSize")),         
			SU::TString(TEXT("TimeoutRx")),
			SU::TString(TEXT("TimeoutTx")),
			SU::TString(TEXT("NotExist")),
			SU::TString(TEXT("OtherError"))
};

#if 0
//------------------------------------------------------------------------------
TCHAR* T_IP_PipeView::GenSemName(const TCHAR* fileName, const TCHAR* semName)
{
	const unsigned StrBufLen = 64;
	static TCHAR StrBuf[StrBufLen];
	StrBuf[0] = 0;
	_tcscat_s(StrBuf,StrBufLen,fileName);
	_tcscat_s(StrBuf,StrBufLen,semName);
	return StrBuf;
}
#endif

//------------------------------------------------------------------------------
T_IP_PipeView::T_IP_PipeView(const SU::TString& fileName, uint32_t chunkSize, uint32_t chunkNum) :
																		mFile(INVALID_HANDLE_VALUE),
																		mFileMapping(NULL),
																		mChunkSize(chunkSize),
																		mChunkNum(chunkNum),
																		mFileView(NULL),
																		#if defined(USE_BLOCKING_WRITE)
																			mSemTx(chunkNum,chunkNum,(fileName+SU::TString(TEXT("_SemTx"))).c_str()),
																		#endif
																		#if defined(USE_BLOCKING_READ)
																			mSemRx(0,chunkNum,(fileName+SU::TString(TEXT("_SemRx"))).c_str()),
																		#endif
																		mSemTxRdy(0,1,(fileName +SU::TString(TEXT("_SemTxRdy"))).c_str()),
																		mSemRxRdy(0,1,(fileName +SU::TString(TEXT("_SemRxRdy"))).c_str())
{
	SU::TString TmpDirEnv = SU::TString(TEXT("TMP"));
	const unsigned FileNameSize = 512;
	TCHAR fullFileName[FileNameSize];
	DWORD result = ::GetEnvironmentVariable(TmpDirEnv.c_str(), fullFileName, static_cast<DWORD>(FileNameSize - fileName.length()));
	if((result == 0) || (result > FileNameSize)) {
		return;
	}
	_tcscat_s(fullFileName,FileNameSize,TEXT("\\"));
	_tcscat_s(fullFileName,FileNameSize,fileName.c_str());
  	/*DEBUG*/ //_tprintf(TEXT("slon %s\n"),fullFileName);

	mFile = ::CreateFile(
							fullFileName,                         // LPCTSTR lpFileName
							(GENERIC_READ | GENERIC_WRITE),       // DWORD dwDesiredAccess
							(FILE_SHARE_READ | FILE_SHARE_WRITE), // DWORD dwShareMode
							NULL,                                 // LPSECURITY_ATTRIBUTES lpSecurityAttributes
							OPEN_ALWAYS,                          // DWORD dwCreationDisposition
							FILE_ATTRIBUTE_NORMAL,                // DWORD dwFlagsAndAttributes
							NULL                                  // HANDLE hTemplateFile
						);
	if(mFile == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERROR] [T_IP_PipeView constructor] CreateFile error: %d\n"),::GetLastError());
		return;
	}

	uint32_t fileSize = sizeof(THandShakeTable) + sizeof(TCircBufferParams) + (mChunkNum + 1)*(sizeof(TCircBufferParams::TPayloadSize) + mChunkSize);
	mFileMapping = ::CreateFileMapping(
										mFile,          // HANDLE hFile
										NULL,           // LPSECURITY_ATTRIBUTES lpAttributes
										PAGE_READWRITE, // DWORD flProtect
										0,              // DWORD dwMaximumSizeHigh
										fileSize,       // DWORD dwMaximumSizeLow
										NULL            // LPCTSTR lpName
									  );
	if(mFileMapping == NULL) {
		_tprintf(TEXT("[ERROR] [T_IP_PipeView constructor] CreateFileMapping error: %d\n"),::GetLastError());
		return;
	}
	// /*DEBUG*/ _tprintf(TEXT("File size: %d\n"),fileSize);

	mFileView = ::MapViewOfFile( 
		                         mFileMapping,        // HANDLE hFileMappingObject,
			                     FILE_MAP_ALL_ACCESS, // DWORD dwDesiredAccess,
				                 0,                   // DWORD dwFileOffsetHigh,
					             0,                   // DWORD dwFileOffsetLow,       offset = 0
						         0                    // SIZE_T dwNumberOfBytesToMap, number of bytes to map = up to end of file
							   );

	if(mFileView == NULL) {
		_tprintf(TEXT("[ERROR] [T_IP_PipeView constructor] MapViewOfFile error: %d\n"),::GetLastError());
		return;
	}

	/*DEBUG*/ //_tprintf(TEXT("SS: %x %x %x %x\n"),mFileView, handShakeTablePtr<void>(), circBufParamsPtr<void>(), chunkPtr<char>(0,4));
	/*DEBUG*/ //_tprintf(TEXT("[INFO] [T_IP_PipeView constructor] OK\n"));
}

//------------------------------------------------------------------------------
T_IP_PipeView::~T_IP_PipeView()
{
	if(mFileView != NULL) {
		if(!::UnmapViewOfFile(mFileView))
			_tprintf(TEXT("[ERROR] [T_IP_PipeView destructor] UnmapViewOfFile error: %d\n"),::GetLastError());
	}
	if(mFileMapping != NULL) {
		if(!::CloseHandle(mFileMapping))
			_tprintf(TEXT("[ERROR] [T_IP_PipeView destructor] FileMapping closing error: %d\n"),::GetLastError());
	}
	if(mFile != INVALID_HANDLE_VALUE) {
		if(!::CloseHandle(mFile))
			_tprintf(TEXT("[INFO] [T_IP_PipeView destructor] File closing error: %d\n"),::GetLastError());
	}
	/*DEBUG*/ //_tprintf(TEXT("[INFO] [T_IP_PipeView destructor] OK\n"));
}

//------------------------------------------------------------------------------
IP_pipe::TStatus T_IP_PipeView::translateWaitResult(DWORD waitResult)
{
	switch (waitResult)  {
		case WAIT_OBJECT_0:
			return IP_pipe::Ok;
		case WAIT_TIMEOUT:
			return IP_pipe::TimeoutExpired;
		default:
			return IP_pipe::OtherError;
	}
}

//------------------------------------------------------------------------------
uint32_t T_IP_PipeView::usedChunkNum()
{
	uint32_t rxIdx = circBufParams().mReadIdx;
	uint32_t txIdx = circBufParams().mWriteIdx;

	/*DEBUG*/ //_tprintf(TEXT("usedChunkNum rx:%2d tx:%2d\n"),rxIdx,txIdx);

	if(rxIdx == txIdx) {
		return 0;
	}
	if(rxIdx < txIdx) {
		return (txIdx - rxIdx); 
	} else {
		return (chunkNum() + 1 + txIdx - rxIdx);
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
T_IP_PipeViewRx::T_IP_PipeViewRx(const SU::TString& fileName, uint32_t chunkSize, uint32_t chunkNum) : T_IP_PipeView(fileName, chunkSize, chunkNum)
{
	clearRdy();
	mName = genPipeViewName(fileName,T_IP_PipeView::PipeRx);
	/*DEBUG*/ //_tprintf(TEXT("[INFO] [T_IP_PipeViewRx constructor] %6s\n"),mName.c_str());
}

//------------------------------------------------------------------------------
T_IP_PipeViewRx::~T_IP_PipeViewRx() 
{
	if(mFileView != NULL) {
		clearRdy();
	}
	/*DEBUG*/ //_tprintf(TEXT("[INFO] [T_IP_PipeViewRx destructor] OK\n"));
}

#if (TRANSFER_BUF_IMPL == 1)
//------------------------------------------------------------------------------
IP_pipe::TStatus T_IP_PipeViewRx::transferBuf(uint32_t& bufSize, uint8_t* buf, int32_t timeout)
{
	if(!isReady())
		return IP_pipe::NotReady;

	#if defined(USE_BLOCKING_READ)
		if(translateWaitResult(mSemRx.wait(timeout) != IP_pipe::Ok)) {
			return IP_pipe::TimeoutRx;
		}
	#else
		if(isBufEmpty()) {
			return IP_pipe::PipeEmpty;
		}
	#endif

	uint32_t idx = circBufParams().mReadIdx;
	bufSize = payloadSize(idx);
	if((bufSize == 0) || (bufSize > circBufParams().mChunkSize))
		return IP_pipe::BadBufSize;
	std::memcpy(buf, chunkPtr<uint8_t>(idx,sizeof(TCircBufferParams::TPayloadSize)), bufSize);
	circBufParams().mReadIdx = incIdx(idx);
	// uint8_t* slon = chunkPtr<uint8_t>(idx,sizeof(TCircBufferParams::TPayloadSize));
	// _tprintf(TEXT("[Rx] rxIdx %02d, txIdx %02d, buf 0x%08x\n"),circBufParams().mReadIdx,circBufParams().mWriteIdx, slon);
	#if defined(USE_BLOCKING_WRITE)
		mSemTx.release();
	#endif
	
	return IP_pipe::Ok;
}
#else
//------------------------------------------------------------------------------
IP_pipe::TStatus T_IP_PipeViewRx::transferBuf(uint32_t& bufSize, uint8_t* buf, int32_t timeout)
{
	uint32_t* bufSizePtr;
	uint8_t*  chunk;

	IP_pipe::TStatus status = chunkAccess(bufSizePtr,chunk,timeout);
	if(status != IP_pipe::Ok)
		return status;

	bufSize = *bufSizePtr;
	std::memcpy(buf,chunk,bufSize);

	return advanceIdx();
}
#endif


//------------------------------------------------------------------------------
IP_pipe::TStatus T_IP_PipeViewRx::chunkAccess(uint32_t*& bufSizePtr, uint8_t*& chunk, int32_t timeout)
{
	if(!isReady())
		return IP_pipe::NotReady;

	#if defined(USE_BLOCKING_READ)
		if(translateWaitResult(mSemRx.wait(timeout) != IP_pipe::Ok)) {
			return IP_pipe::TimeoutRx;
		}
	#else
		if(isBufEmpty()) {
			return IP_pipe::PipeEmpty;
		}
	#endif

	uint32_t idx = circBufParams().mReadIdx;
	bufSizePtr = payloadSizePtr(idx);
	if((*bufSizePtr == 0) || (*bufSizePtr > circBufParams().mChunkSize))
		return IP_pipe::BadBufSize;

	chunk = chunkPtr<uint8_t>(idx,sizeof(TCircBufferParams::TPayloadSize));

	return IP_pipe::Ok;
}

//------------------------------------------------------------------------------
IP_pipe::TStatus T_IP_PipeViewRx::advanceIdx()
{
	if(!isReady())
		return IP_pipe::NotReady;

	uint32_t idx = circBufParams().mReadIdx;
	circBufParams().mReadIdx = incIdx(idx);

	#if defined(USE_BLOCKING_WRITE)
		mSemTx.release();
	#endif

	return IP_pipe::Ok;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
T_IP_PipeViewTx::T_IP_PipeViewTx(const SU::TString& fileName, uint32_t chunkSize, uint32_t chunkNum) : T_IP_PipeView(fileName, chunkSize, chunkNum)
{
	circBufParams().mChunkNum  = mChunkNum;
	circBufParams().mChunkSize = mChunkSize;
	circBufParams().mWriteIdx  = 0;
	circBufParams().mReadIdx   = 0;

	clearRdy();
	mName = genPipeViewName(fileName,T_IP_PipeView::PipeTx);
	/*DEBUG*/ //_tprintf(TEXT("[INFO] [T_IP_PipeViewTx constructor] %6s\n"),name());
}

//------------------------------------------------------------------------------
T_IP_PipeViewTx::~T_IP_PipeViewTx() 
{
	if(mFileView != NULL) {
		clearRdy();
		
		#if 0
			circBufParams().mChunkNum  = 0;
			circBufParams().mChunkSize = 0;
			circBufParams().mWriteIdx  = 0;
			circBufParams().mReadIdx   = 0;
		#endif
	}
	/*DEBUG*/ //_tprintf(TEXT("[INFO] [T_IP_PipeViewTx destructor] OK\n"));
}

#if (TRANSFER_BUF_IMPL == 1)
//------------------------------------------------------------------------------
IP_pipe::TStatus T_IP_PipeViewTx::transferBuf(uint32_t& bufSize, uint8_t* buf, int32_t timeout)
{
	if(!isReady())
		return IP_pipe::NotReady;

	#if defined(USE_BLOCKING_WRITE)
		if(translateWaitResult(mSemTx.wait(timeout) != IP_pipe::Ok)) {
			return IP_pipe::TimeoutTx;
		}
	#else
		if(isBufFull()) {
			//_tprintf(TEXT("usedChunkNum = %2d, chunkNum = %2d\n"),usedChunkNum(),chunkNum());
			return IP_pipe::PipeFull;
		}
	#endif

	if((bufSize == 0) || (bufSize > circBufParams().mChunkSize))
		return IP_pipe::BadBufSize;

	uint32_t idx = circBufParams().mWriteIdx;
	//uint8_t* slon = chunkPtr<uint8_t>(idx,sizeof(TCircBufferParams::TPayloadSize));
	//uint32_t* slon32 = reinterpret_cast<uint32_t*>(slon);
	//uint32_t* buf32 = reinterpret_cast<uint32_t*>(buf);
	std::memcpy(chunkPtr<uint8_t>(idx,sizeof(TCircBufferParams::TPayloadSize)),buf, bufSize);
	payloadSize(idx) = bufSize;
	circBufParams().mWriteIdx = incIdx(idx);
	//_tprintf(TEXT("[Tx] rxIdx %02d, txIdx %02d, buf 0x%08x, %4d\n"),circBufParams().mReadIdx,circBufParams().mWriteIdx, slon, bufSize);
	//for(int k = 0; k < 8; ++k) {
	//	_tprintf(TEXT("[%1d] = %4d, %4d\n"),k, slon32[k],buf32[k]);
	//}

	#if defined(USE_BLOCKING_READ)
		mSemRx.release();
	#endif
	
	return IP_pipe::Ok;
}
#else
//------------------------------------------------------------------------------
IP_pipe::TStatus T_IP_PipeViewTx::transferBuf(uint32_t& bufSize, uint8_t* buf, int32_t timeout)
{
	uint32_t* bufSizePtr;
	uint8_t*  chunk;

	IP_pipe::TStatus status = chunkAccess(bufSizePtr,chunk,timeout);
	if(status != IP_pipe::Ok)
		return status;

	if((bufSize == 0) || (bufSize > circBufParams().mChunkSize))
		return IP_pipe::BadBufSize;

	*bufSizePtr = bufSize;
	std::memcpy(chunk,buf,bufSize);

	return advanceIdx();
}
#endif

//------------------------------------------------------------------------------
IP_pipe::TStatus T_IP_PipeViewTx::chunkAccess(uint32_t*& bufSizePtr, uint8_t*& chunk, int32_t timeout)
{
	if(!isReady())
		return IP_pipe::NotReady;

	#if defined(USE_BLOCKING_WRITE)
		if(translateWaitResult(mSemTx.wait(timeout) != IP_pipe::Ok)) {
			return IP_pipe::TimeoutTx;
		}
	#else
		if(isBufFull()) {
			//_tprintf(TEXT("usedChunkNum = %2d, chunkNum = %2d\n"),usedChunkNum(),chunkNum());
			return IP_pipe::PipeFull;
		}
	#endif

	uint32_t idx = circBufParams().mWriteIdx;
	bufSizePtr = payloadSizePtr(idx);
	chunk = chunkPtr<uint8_t>(idx,sizeof(TCircBufferParams::TPayloadSize));

	return IP_pipe::Ok;
}

//------------------------------------------------------------------------------
IP_pipe::TStatus T_IP_PipeViewTx::advanceIdx()
{
	if(!isReady())
		return IP_pipe::NotReady;

	uint32_t idx = circBufParams().mWriteIdx;
	circBufParams().mWriteIdx = incIdx(idx);

	#if defined(USE_BLOCKING_READ)
		mSemRx.release();
	#endif

	return IP_pipe::Ok;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
TPipeViewPool::~TPipeViewPool()
{
    for(TPipeViewPoolMap::iterator pipeView = instance().begin(); pipeView != instance().end(); ++pipeView) {
		//_tprintf(TEXT("[pipe deleted] id: %s\n"), pipeView->first.c_str());
        delete pipeView->second;
    }
	//_tprintf(TEXT("[pipe pool destructor]\n"));
}

//------------------------------------------------------------------------------
T_IP_PipeView* TPipeViewPool::getPipeView(const SU::TString& name)
{
	TPipeViewPoolMap::iterator pipeView = instance().find(name);
	if(pipeView == instance().end())
		return 0;
	else
		return pipeView->second;
}

//------------------------------------------------------------------------------
bool TPipeViewPool::createPipeView(const SU::TString& name, T_IP_PipeView::TType pipeType, uint32_t chunkSize, uint32_t chunkNum)
{
	SU::TString pipeViewName = T_IP_PipeView::genPipeViewName(name,pipeType);
	if(isPipeViewExist(pipeViewName))
		return false;
	T_IP_PipeView* pipeView;
	if(pipeType == T_IP_PipeView::PipeRx)
		pipeView = new T_IP_PipeViewRx(name, chunkSize, chunkNum);
	else
		pipeView = new T_IP_PipeViewTx(name, chunkSize, chunkNum);
	instance().insert(std::pair<SU::TString,T_IP_PipeView*>(pipeViewName,pipeView));
	return true;
}

//------------------------------------------------------------------------------
bool TPipeViewPool::setRdy(const SU::TString& name)
{
	if(!isPipeViewExist(name))
		return false;
	getPipeView(name)->setRdy();
	return true;
}

//------------------------------------------------------------------------------
IP_pipe::TStatus TPipeViewPool::waitPeerRdy(const SU::TString& name, DWORD timeout)
{
	if(!isPipeViewExist(name))
		return IP_pipe::NotExist;
	return getPipeView(name)->waitPeerRdy(timeout);
}

//------------------------------------------------------------------------------
bool TPipeViewPool::isReady(const SU::TString& name)
{
	if(!isPipeViewExist(name))
		return false;
	return getPipeView(name)->isReady();
}

//------------------------------------------------------------------------------
bool TPipeViewPool::isPeerReady(const SU::TString& name)
{
	if(!isPipeViewExist(name))
		return false;
	return getPipeView(name)->isPeerReady();
}

//------------------------------------------------------------------------------
bool TPipeViewPool::isPipeReady(const SU::TString& name)
{
	if(!isPipeViewExist(name))
		return false;
	return getPipeView(name)->isPipeReady();
}

//------------------------------------------------------------------------------
int32_t TPipeViewPool::chunkNum(const SU::TString& name)
{
	if(!isPipeViewExist(name))
		return -1;
	return getPipeView(name)->chunkNum();
}

//------------------------------------------------------------------------------
int32_t TPipeViewPool::usedChunkNum(const SU::TString& name)
{
	if(!isPipeViewExist(name))
		return -1;
	return getPipeView(name)->usedChunkNum();
}

//------------------------------------------------------------------------------
bool TPipeViewPool::isBufEmpty(const SU::TString& name)
{
	if(!isPipeViewExist(name))
		return false;
	return getPipeView(name)->isBufEmpty();
}

//------------------------------------------------------------------------------
bool TPipeViewPool::isBufFull(const SU::TString& name)
{
	if(!isPipeViewExist(name))
		return false;
	return getPipeView(name)->isBufFull();
}

//------------------------------------------------------------------------------
IP_pipe::TStatus TPipeViewPool::transferBuf(const SU::TString& name, uint32_t& bufSize, uint8_t* buf, int32_t timeout)
{
	if(!isPipeViewExist(name))
		return IP_pipe::NotExist;
	return getPipeView(name)->transferBuf(bufSize, buf, timeout);
}

//------------------------------------------------------------------------------
IP_pipe::TStatus TPipeViewPool::chunkAccess(const SU::TString& name, uint32_t*& bufSizePtr, uint8_t*& chunk, int32_t timeout)
{
	if(!isPipeViewExist(name))
		return IP_pipe::NotExist;
	return getPipeView(name)->chunkAccess(bufSizePtr, chunk, timeout);
}

//------------------------------------------------------------------------------
IP_pipe::TStatus TPipeViewPool::advanceIdx(const SU::TString& name)
{
	if(!isPipeViewExist(name))
		return IP_pipe::NotExist;
	return getPipeView(name)->advanceIdx();
}
