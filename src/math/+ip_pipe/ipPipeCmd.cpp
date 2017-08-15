//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "mex.h"
#include "matrix.h"
#include <map>
#include "ip_pipe.h"

//#define MEM_INFO

#if defined(MEM_INFO)
//https://msdn.microsoft.com/en-us/library/windows/desktop/aa366589(v=vs.85).aspx
    #define WIDTH 7
    #define DIV 1024
#endif

//------------------------------------------------------------------------------
extern "C" bool mxUnshareArray(mxArray *array_ptr, bool noDeepCopy);

typedef void(*CmdFunc)(int,mxArray *[],int,const mxArray *[]);
typedef std::map<const SU::TString,CmdFunc> TCmdMap;

static TCmdMap& getCmdMap();

static void isPipeViewExistCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 
static void createPipeViewCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 

static void setRdyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 
static void waitPeerRdyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 
static void isReadyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 
static void isPeerReadyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 
static void isPipeReadyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 

// not implemented: chunkNum
// not implemented: usedChunkNum
static void isBufEmptyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 
static void isBufFullCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 

static void transferBufCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); 
// not implemented: chunkAccess
// not implemented: advanceIdx

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs < 2) {
        mexErrMsgTxt("[ipPipeCmd] bad number of input parameters\n");
    }
    const mxArray* funcNameParam = prhs[0];
    if(!mxIsChar(funcNameParam)) {
        mexErrMsgTxt("[ipPipeCmd] type of input parameters\n");
    }
    TCmdMap::iterator cmdFunc = getCmdMap().find(SU::TString(mxArrayToString(funcNameParam)));
    if(cmdFunc != getCmdMap().end()) {
        (cmdFunc->second)(nlhs,plhs,nrhs,prhs);
    } else {
        mexErrMsgTxt("[ipPipeCmd] bad command\n");
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
TCmdMap& getCmdMap()
{
    static bool Init = false;
    static TCmdMap CmdMap;
    if(!Init) {
        Init = true;
        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("isPipeViewExist")),isPipeViewExistCmd));
        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("createPipeView")),createPipeViewCmd));

        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("setRdy")),setRdyCmd));
        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("waitPeerRdy")),waitPeerRdyCmd));
        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("isReady")),isReadyCmd));
        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("isPeerReady")),isPeerReadyCmd));
        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("isPipeReady")),isPipeReadyCmd));

        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("isBufEmpty")),isBufEmptyCmd));
        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("isBufFull")),isBufFullCmd));

        CmdMap.insert(std::pair<SU::TString,CmdFunc>(SU::TString(TEXT("transferBuf")),transferBufCmd));
    }
    return CmdMap;
}

//------------------------------------------------------------------------------
// prhs[0] - char* - must be "isPipeViewExist"
// prhs[1] - char* - pipe name
//
// plhs[0] - logical - ok/!ok
//------------------------------------------------------------------------------
void isPipeViewExistCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs != 2) {
        mexErrMsgTxt("[isPipeViewExistCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1])) {
        mexErrMsgTxt("[isPipeViewExistCmd] bad input parameters\n");
    }
    if(nlhs > 1) {
        mexErrMsgTxt("[isPipeViewExistCmd] bad number of output parameters\n");
    }
    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
    mxLogical res = TPipeViewPool::isPipeViewExist(pipeViewName);
    if(nlhs == 1) {
        plhs[0] = mxCreateLogicalScalar(res);
    }
}

//------------------------------------------------------------------------------
// prhs[0] - char*   - must be "createPipeView"
// prhs[1] - char*   - pipe name
// prhs[2] - char*   - pipe type ("rx" or "tx")
// prhs[3] - numeric - chunk size (in bytes)
// prhs[4] - numeric - chunk num
//
// plhs[0] - logical - ok/!ok
//------------------------------------------------------------------------------
void createPipeViewCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs != 5) {
        mexErrMsgTxt("[createPipeViewCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1]) || !mxIsChar(prhs[2])|| (!mxIsNumeric(prhs[3])) || (!mxIsNumeric(prhs[4]))) {
        mexErrMsgTxt("[createPipeViewCmd] bad input parameters\n");
    }
    if(nlhs > 1) {
        mexErrMsgTxt("[createPipeViewCmd] bad number of output parameters\n");
    }
    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
        
    T_IP_PipeView::TType pipeViewType;
    if(_tcscmp(mxArrayToString(prhs[2]),TEXT("rx")) == 0)
        pipeViewType = T_IP_PipeView::PipeRx;
    else 
        pipeViewType = T_IP_PipeView::PipeTx;
    uint32_t chunkSize = static_cast<uint32_t>(mxGetScalar(prhs[3]));
    uint32_t chunkNum  = static_cast<uint32_t>(mxGetScalar(prhs[4]));
    mxLogical res = TPipeViewPool::createPipeView(pipeViewName, pipeViewType, chunkSize, chunkNum);
    if(nlhs == 1) {
        plhs[0] = mxCreateLogicalScalar(res);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void setRdyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs != 2) {
        mexErrMsgTxt("[setRdyCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1])) {
        mexErrMsgTxt("[setRdyCmd] bad input parameters\n");
    }
    if(nlhs > 1) {
        mexErrMsgTxt("[setRdyCmd] bad number of output parameters\n");
    }
    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
    mxLogical res = TPipeViewPool::setRdy(pipeViewName);
    if(nlhs == 1) {
        plhs[0] = mxCreateLogicalScalar(res);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void waitPeerRdyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if((nrhs < 2) || (nrhs > 3)) {
        mexErrMsgTxt("[waitPeerRdyCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1])) {
        mexErrMsgTxt("[waitPeerRdyCmd] bad input parameters (1)\n");
    }
    if(nrhs == 3 ) {
        if(!mxIsNumeric(prhs[2])) {
            mexErrMsgTxt("[waitPeerRdyCmd] bad input parameters (2)\n");
        }
    }
    if(nlhs > 1) {
        mexErrMsgTxt("[waitPeerRdyCmd] bad number of output parameters\n");
    }
    
    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
    IP_pipe::TStatus status;
    if(nrhs == 2) {
         status = TPipeViewPool::waitPeerRdy(pipeViewName);
    } else {
        int32_t timeout = static_cast<int32_t>(mxGetScalar(prhs[2]));
        status = TPipeViewPool::waitPeerRdy(pipeViewName,timeout);
    }
    if(nlhs == 1) {
        plhs[0] = mxCreateDoubleScalar(status);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void isReadyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs != 2) {
        mexErrMsgTxt("[isReadyCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1])) {
        mexErrMsgTxt("[isReadyCmd] bad input parameters\n");
    }
    if(nlhs > 1) {
        mexErrMsgTxt("[isReadyCmd] bad number of output parameters\n");
    }
    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
    mxLogical res = TPipeViewPool::isReady(pipeViewName);
    if(nlhs == 1) {
        plhs[0] = mxCreateLogicalScalar(res);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void isPeerReadyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs != 2) {
        mexErrMsgTxt("[isPeerReadyCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1])) {
        mexErrMsgTxt("[isPeerReadyCmd] bad input parameters\n");
    }
    if(nlhs > 1) {
        mexErrMsgTxt("[isPeerReadyCmd] bad number of output parameters\n");
    }
    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
    mxLogical res = TPipeViewPool::isPeerReady(pipeViewName);
    if(nlhs == 1) {
        plhs[0] = mxCreateLogicalScalar(res);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void isPipeReadyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs != 2) {
        mexErrMsgTxt("[isPipeReadyCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1])) {
        mexErrMsgTxt("[isPipeReadyCmd] bad input parameters\n");
    }
    if(nlhs > 1) {
        mexErrMsgTxt("[isPipeReadyCmd] bad number of output parameters\n");
    }
    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
    mxLogical res = TPipeViewPool::isPipeReady(pipeViewName);
    if(nlhs == 1) {
        plhs[0] = mxCreateLogicalScalar(res);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void isBufEmptyCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs != 2) {
        mexErrMsgTxt("[isBufEmptyCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1])) {
        mexErrMsgTxt("[isBufEmptyCmd] bad input parameters\n");
    }
    if(nlhs > 1) {
        mexErrMsgTxt("[isBufEmptyCmd] bad number of output parameters\n");
    }
    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
    mxLogical res = TPipeViewPool::isBufEmpty(pipeViewName);
    if(nlhs == 1) {
        plhs[0] = mxCreateLogicalScalar(res);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void isBufFullCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs != 2) {
        mexErrMsgTxt("[isBufFullCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1])) {
        mexErrMsgTxt("[isBufFullCmd] bad input parameters\n");
    }
    if(nlhs > 1) {
        mexErrMsgTxt("[isBufFullCmd] bad number of output parameters\n");
    }
    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
    mxLogical res = TPipeViewPool::isBufFull(pipeViewName);
    if(nlhs == 1) {
        plhs[0] = mxCreateLogicalScalar(res);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void transferBufCmd(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if((nrhs < 4) || (nrhs > 5)) {
        mexErrMsgTxt("[transferBufCmd] bad number of input parameters\n");
    }
    if(!mxIsChar(prhs[1]) || !mxIsNumeric(prhs[2]) || !mxIsNumeric(prhs[3])) {
        mexErrMsgTxt("[transferBufCmd] bad input parameters (1)\n");
    }
    if(nrhs == 5 ) {
        if(!mxIsNumeric(prhs[4])) {
            mexErrMsgTxt("[transferBufCmd] bad input parameters (2)\n");
        }
    }
    if(nlhs > 2) {
        mexErrMsgTxt("[transferBufCmd] bad number of output parameters\n");
    }

    SU::TString pipeViewName = SU::TString(mxArrayToString(prhs[1]));  
    //SU::TString pipeViewName(mxArrayToString(prhs[1]));  
    uint32_t bufSize = static_cast<uint32_t>(mxGetScalar(prhs[2]));
    size_t elemSize = mxGetElementSize(prhs[3]);
    bufSize = static_cast<uint32_t>(bufSize*elemSize);
    mxUnshareArray(const_cast<mxArray *>(prhs[3]), true);  
    uint8_t* buf = reinterpret_cast<uint8_t*>(mxGetData(prhs[3]));

    #if defined(MEM_INFO)
        MEMORYSTATUSEX statex1, statex2;
        statex1.dwLength = sizeof (statex1);
        statex2.dwLength = sizeof (statex2);
    #endif
    
    IP_pipe::TStatus status;
    #if defined(MEM_INFO)
            GlobalMemoryStatusEx (&statex1);
    #endif
    if(nrhs == 4) {
         status = TPipeViewPool::transferBuf(pipeViewName,bufSize,buf);
    } else {
        int32_t timeout = static_cast<int32_t>(mxGetScalar(prhs[4]));
        status = TPipeViewPool::transferBuf(pipeViewName,bufSize,buf,timeout);
    }
    #if defined(MEM_INFO)
        GlobalMemoryStatusEx (&statex2);

        int64_t dPhysicalMem = statex2.ullAvailPhys - statex1.ullAvailPhys;
        int64_t dVirtualMem  = statex2.ullAvailVirtual - statex1.ullAvailVirtual;
        int64_t dPagingFile  = statex2.ullAvailPageFile - statex1.ullAvailPageFile;
    
        static uint64_t count = 0;
        static int64_t gdPhy = 0;
        static int64_t gdVirt = 0;
        static int64_t gdPF = 0;
    
        ++count;
        gdPhy  += dPhysicalMem;
        gdVirt += dVirtualMem;
        gdPF   += dPagingFile;
            
        _tprintf (TEXT("*****************************\n"));
        _tprintf (TEXT("%*I64d count\n"),WIDTH, count);
        _tprintf (TEXT("%*I64d (diff)  KB of physical memory\n"),WIDTH, gdPhy/DIV);
        _tprintf (TEXT("%*I64d (diff)  KB of virtual memory\n"),WIDTH, gdVirt/DIV);
        _tprintf (TEXT("%*I64d (diff)  KB of paging file\n"),WIDTH, gdPF/DIV);
    #endif
    
    if(nlhs) {
        plhs[0] = mxCreateDoubleScalar(status);
    }
    if(nlhs == 2) {
        bufSize = static_cast<uint32_t>(bufSize/elemSize);
        plhs[1] = mxCreateDoubleScalar(bufSize);
    }
}


