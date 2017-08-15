#include <windows.h>
#include <cstdio>
#include <string.h>
#include <tchar.h>

#include "SU.h"
#include "IP_PipeLib.h"

//------------------------------------------------------------------------------
typedef  const TCHAR* TFileName;
#define FILE_NAME(NAME) TEXT(NAME)
typedef TCHAR TPipeViewName[80];
#define RX_PIPE TEXT("rx")
#define TX_PIPE TEXT("tx")

//------------------------------------------------------------------------------
TFileName    	  LoggerFileName        = FILE_NAME("Logger");
const uint32_t    LoggerChunkSize       = 4;
const uint32_t    LoggerChunkNum        = 4;
const int32_t     LoggerInitTimeout     = 1000;
const int32_t     LoggerTransferTimeout = 500;

//------------------------------------------------------------------------------
TFileName    	  MathFileName          = FILE_NAME("toMath");
const uint32_t    MathChunkSize         = 1024*1024;
const uint32_t    MathChunkNum          = 256;
const int32_t     MathInitTimeout       = 1000;
const int32_t     MathTransferTimeout   = 500;



//------------------------------------------------------------------------------
int _tmain(int argc, TCHAR* argv[])
{
	//---
	_tprintf(TEXT("----------------------------------------------------------\n"));
	TPipeViewName loggerPipeViewName;
	IP_pipe::createPipeView(LoggerFileName,RX_PIPE,LoggerChunkSize, LoggerChunkNum);
	IP_pipe::genPipeViewName(LoggerFileName,RX_PIPE,loggerPipeViewName);
	_tprintf(TEXT("[INFO] [%6s] start\n"),loggerPipeViewName);
	IP_pipe::setRdy(loggerPipeViewName);

	TPipeViewName mathPipeViewName;
	IP_pipe::createPipeView(MathFileName,RX_PIPE,MathChunkSize, MathChunkNum);
	IP_pipe::genPipeViewName(MathFileName,RX_PIPE,mathPipeViewName);
	_tprintf(TEXT("[INFO] [%6s] start\n"),mathPipeViewName);
	IP_pipe::setRdy(mathPipeViewName);

	_tprintf(TEXT("rdy: %d, peer_rdy: %d\n"),IP_pipe::isReady(mathPipeViewName),IP_pipe::isPeerReady(mathPipeViewName));

	//---
	uint32_t fileNum = 0;
	SU::TElapsedTimer timer;
	timer.Calibrate();

	while(IP_pipe::isPipeReady(mathPipeViewName) && IP_pipe::isPipeReady(loggerPipeViewName)) {
		uint32_t* bufSizePtr;
		uint8_t*  chunk;
		IP_pipe::TStatus trStatus = IP_pipe::chunkAccess(mathPipeViewName, bufSizePtr, chunk, MathTransferTimeout);
		if(trStatus == IP_pipe::Ok) {
			if(!IP_pipe::isBufEmpty(loggerPipeViewName)) {
				uint32_t bufSize;
				int32_t  lensControlValue;
				trStatus = IP_pipe::transferBuf(loggerPipeViewName, bufSize, reinterpret_cast<uint8_t*>(&lensControlValue),LoggerTransferTimeout);
				if(trStatus == IP_pipe::Ok) {
					timer.Start();
					_tprintf(TEXT("[INFO] LensControlValue: %4d\n"),lensControlValue);
					FILE* file;
					TCHAR fileName[200];
					swprintf_s(fileName,TEXT("log/frame[%03d][%+05d].dat"),++fileNum,lensControlValue);
					if(!_wfopen_s(&file,fileName,TEXT("wb"))) {
						fwrite(&fileNum, sizeof(fileNum),1,file);
						fwrite(&lensControlValue, sizeof(lensControlValue),1,file);
						fwrite(chunk,sizeof(*chunk),*bufSizePtr,file);
						fclose(file);
					}
					timer.Catch();
					//_tprintf(TEXT("elapsed time: %lld ms\n"),timer.GetStopDelta()/1000); 
				}
			}
			/*trStatus =*/ IP_pipe::advanceIdx(mathPipeViewName);
		}
	}
	_tprintf(TEXT("af_logger inished\n"));
}


