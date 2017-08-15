#include <cstdio>
#include <string.h>
#include <tchar.h>

#if defined(DLL_TEST)
	#include "IP_PipeLib.h"

	typedef  const TCHAR* TFileName;
	#define FILE_NAME(NAME) TEXT(NAME)
	typedef TCHAR TPipeViewName[80];
	#define RX_PIPE TEXT("rx")
	#define TX_PIPE TEXT("tx")
	#define PREFIX1 IP_pipe
	#define PREFIX2 IP_pipe
	#define PIPE_VIEW_NAME_STR pipeViewName  
#else
	#include "IP_Pipe.h"
	typedef  const SU::TString TFileName;
	#define FILE_NAME(NAME) SU::TString(TEXT(NAME))
	typedef  SU::TString TPipeViewName;  
	#define RX_PIPE T_IP_PipeView::PipeRx
	#define TX_PIPE T_IP_PipeView::PipeTx
	#define PREFIX1 TPipeViewPool
	#define PREFIX2 T_IP_PipeView
	#define PIPE_VIEW_NAME_STR pipeViewName.c_str()  
#endif


#include "PipeTester.h"

typedef uint64_t TData;

#define USE_SPLIT_TRANSFER
//#define MEM_INFO

#if defined(MEM_INFO)
	//https://msdn.microsoft.com/en-us/library/windows/desktop/aa366589(v=vs.85).aspx
    #define WIDTH 7
    #define DIV 1024
#endif

//------------------------------------------------------------------------------
//TFileName    	  FileName    = FILE_NAME("chA");
TFileName    	  FileName    = FILE_NAME("toMath");
const uint32_t    ChunkSize   = 1024*1024;
const uint32_t    ChunkNum    = 256;
const uint32_t    PacketSize  = 1024*1024/TPipeTester<TData>::ElemSize();
//const uint32_t    PacketSize  = 128/TPipeTester<TData>::ElemSize();
const int32_t     TransferTimeout = 1000;
const uint32_t    PrintInterval = 1024;
bool              TestEnabled = true;
const bool        EnaResync   = true;
uint32_t          PacketNum   = 1*1024;


//------------------------------------------------------------------------------
int _tmain(int argc, TCHAR* argv[])
{
    if(argc <= 3) {
        _tprintf(TEXT("No input parameters\n"));
        return 0;
    }
    TPipeTester<TData>*   pipeTester = 0;
	TPipeViewName pipeViewName;

	if(_tcscmp(TEXT("-c"),argv[2]) == 0) {
		TestEnabled = true;
	} else {
		TestEnabled = false;
	}

	PacketNum = _ttoi(argv[3]);
	PacketNum *= 1024;
	//printf("PacketNum = %d, Size = %d\n",PacketNum,PacketSize);

	bool isPipeRx = false;
	if(_tcscmp(TEXT("-rx"),argv[1]) == 0) {
		_tprintf(TEXT("----------------------------------------------------------\n"));
		PREFIX1::createPipeView(FileName,RX_PIPE,ChunkSize, ChunkNum);
		#if defined(DLL_TEST)
			PREFIX2::genPipeViewName(FileName,RX_PIPE,pipeViewName);
		#else
			pipeViewName = PREFIX2::genPipeViewName(FileName,RX_PIPE);
		#endif
        pipeTester = new TRxPipeTester<TData>(PacketSize, PacketNum, TestEnabled, EnaResync);
		isPipeRx = true;
    }
    if(_tcscmp(TEXT("-tx"),argv[1]) == 0) {
		_tprintf(TEXT("----------------------------------------------------------\n"));
		PREFIX1::createPipeView(FileName,TX_PIPE,ChunkSize, ChunkNum);
		#if defined(DLL_TEST)
			PREFIX2::genPipeViewName(FileName,TX_PIPE,pipeViewName);
		#else
			pipeViewName = PREFIX2::genPipeViewName(FileName,TX_PIPE);
		#endif
        pipeTester = new TTxPipeTester<TData>(PacketSize, PacketNum, TestEnabled);
    }
    if(!PREFIX1::isPipeViewExist(pipeViewName) || !pipeTester) {
        _tprintf(TEXT("Bad input parameter [1]\n"));
        return 0;
    }

    _tprintf(TEXT("----------------------------------------------------------\n"));
	_tprintf(TEXT("[%6s] start\n\n"),PIPE_VIEW_NAME_STR);
	PREFIX1::setRdy(pipeViewName);

	if(!PREFIX1::isPipeReady(pipeViewName)) {
		if(PREFIX1::waitPeerRdy(pipeViewName) == IP_pipe::TimeoutExpired) {
			_tprintf(TEXT("Timeout expired for %10s PipeView\n"), PIPE_VIEW_NAME_STR);
			return 0;
		}
	}
	_tprintf(TEXT("[%s] rdy: %1d, peer rdy: %1d\n\n"), PIPE_VIEW_NAME_STR, PREFIX1::isReady(pipeViewName), PREFIX1::isPeerReady(pipeViewName));

    bool bypass = false;
    IP_pipe::TStatus trStatus;
	uint32_t nReadyCounter = 0;

	#if defined(MEM_INFO)
		MEMORYSTATUSEX statex1, statex2;
		statex1.dwLength = sizeof (statex1);
		statex2.dwLength = sizeof (statex2);

		static int64_t gdPhy = 0;
		static int64_t gdVirt = 0;
		static int64_t gdPF = 0;
	#endif

	int k = 0;
	pipeTester->timerStart();
    while(!pipeTester->isTestFinish()) {
        if(true /*PREFIX1::isPeerReady(pipeViewName)*/) {
            pipeTester->preProcess(bypass);
			#if defined(MEM_INFO)
				GlobalMemoryStatusEx (&statex1);
			#endif
			 //DELETE THIS LINE!!! _tprintf(TEXT("[%6s] slon1\n"),PIPE_VIEW_NAME_STR);
			//DELETE THIS LINE!!! printf("tester packet size = %d\n",pipeTester->transferPacketSize());
			#if !defined(USE_SPLIT_TRANSFER)
				trStatus = PREFIX1::transferBuf(pipeViewName, pipeTester->transferPacketSize(), pipeTester->pipePacket(),TransferTimeout);
			#else
				uint32_t* bufSizePtr;
				uint8_t*  chunk;
				trStatus = PREFIX1::chunkAccess(pipeViewName, bufSizePtr, chunk, TransferTimeout);
				if(trStatus == IP_pipe::Ok) {
					if(isPipeRx) {
						pipeTester->transferPacketSize() = *bufSizePtr;
						std::memcpy(pipeTester->pipePacket(),chunk, *bufSizePtr);
					} else {
						*bufSizePtr = pipeTester->transferPacketSize();
						std::memcpy(chunk,pipeTester->pipePacket(), *bufSizePtr);
					}
					trStatus = PREFIX1::advanceIdx(pipeViewName);
				} else {
					break;
				}
			#endif
			 //DELETE THIS LINE!!! _tprintf(TEXT("[%6s] slon2, %d\n"),PIPE_VIEW_NAME_STR,trStatus);
			#if defined(MEM_INFO)
				GlobalMemoryStatusEx (&statex2);
				int64_t dPhysicalMem = statex2.ullAvailPhys - statex1.ullAvailPhys;
				int64_t dVirtualMem  = statex2.ullAvailVirtual - statex1.ullAvailVirtual;
				int64_t dPagingFile  = statex2.ullAvailPageFile - statex1.ullAvailPageFile;
				gdPhy  += dPhysicalMem;
				gdVirt += dVirtualMem;
				gdPF   += dPagingFile;
			#endif
    
			bypass = (trStatus != IP_pipe::Ok);
            pipeTester->postProcess(bypass);
			if(!bypass) {
				pipeTester->packetNumInc();
			} else {
				++nReadyCounter;
                break;
			}
			if((pipeTester->packetNum()%PrintInterval) == 0) {
				_tprintf(TEXT("[INFO] KPacket transferred: %5d\n"),pipeTester->packetNum()/PrintInterval);
			}
        } else {
            _tprintf(TEXT("[WARN] pipe %10s is closed, usedChunkNum = %2d, chunkNum = %2d\n"),FileName,PREFIX1::usedChunkNum(pipeViewName),PREFIX1::chunkNum(pipeViewName));
            break;
        }
    }
	pipeTester->timerCatch();

	#if defined(MEM_INFO)
		_tprintf (TEXT("*****************************\n"));
		_tprintf (TEXT("%*I64d (diff)  KB of physical memory\n"),WIDTH, gdPhy/DIV);
		_tprintf (TEXT("%*I64d (diff)  KB of virtual memory\n"),WIDTH, gdVirt/DIV);
		_tprintf (TEXT("%*I64d (diff)  KB of paging file\n"),WIDTH, gdPF/DIV);
	#endif

    _tprintf(TEXT("----------------------------------------------------------\n"));
    _tprintf(TEXT("[%6s] end\n\n"),PIPE_VIEW_NAME_STR);
    _tprintf(TEXT("%-14s%20d\n"),TEXT("test enabled:"),pipeTester->enaTest());
    _tprintf(TEXT("%-14s%20d\n"),TEXT("total pkt:"),pipeTester->packetNum());
    _tprintf(TEXT("%-14s%20s ms\n"),TEXT("total time:"),TPipeTester<TData>::num2str(pipeTester->elapsedTime_ms()));
    //_tprintf(TEXT("%-14s%20s MB/s\n"),TEXT("transfer rate:"),TPipeTester<TData>::num2str(pipeTester->transferRate()));
    _tprintf(TEXT("%-14s%20.1f MB/s\n"),TEXT("transfer rate:"),pipeTester->fTransferRate());
    _tprintf(TEXT("%-14s%20d bytes\n"),TEXT("word size:"),TPipeTester<TData>::ElemSize());
    _tprintf(TEXT("%-14s%20s\n"),TEXT("total bytes:"),TPipeTester<TData>::num2str(pipeTester->getTransferredBytes()));
    _tprintf(TEXT("%-14s%20s\n"),TEXT("total words:"),TPipeTester<TData>::num2str(pipeTester->getTransferredWords()));
    _tprintf(TEXT("%-14s%20d\n"),TEXT("nReady:"),nReadyCounter);
    _tprintf(TEXT("%-14s%20d\n"),TEXT("total err:"),pipeTester->totalErrors());
    _tprintf(TEXT("----------------------------------------------------------\n\n"));
		
    delete pipeTester;
}


