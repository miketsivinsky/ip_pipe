//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "mex.h"
#include "matrix.h"
#include <cstdint>

//------------------------------------------------------------------------------
extern "C" bool mxUnshareArray(mxArray *array_ptr, bool noDeepCopy);

static const unsigned FrameHeaderLen = 16;
static bool checkParams(int nrhs, const mxArray *prhs[]);
static bool checkFrame(const uint16_t* inBuf, unsigned payloadSize, unsigned xSize, unsigned ySize, uint32_t& frameNum);
static void getImgFrame(const uint16_t* inBuf, unsigned xSize, unsigned ySize, uint8_t* imgFrame, unsigned shiftFactor);
template <typename T> static void getMathFrame(const uint16_t* inBuf, unsigned xSize, unsigned ySize, T* mathFrame);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template <typename T> void getMathFrame(const uint16_t* inBuf, unsigned xSize, unsigned ySize, T* mathFrame)
{
     for(unsigned i = 0; i < xSize; ++i) {
        for(unsigned j = 0; j < ySize; ++j) {
            *mathFrame++ = (*(inBuf + j*xSize + i));
        }
     }
}


//------------------------------------------------------------------------------
// prhs[0] - numeric - cmd mask for ena/dis output arrays forming
// prhs[1] - uint16* - input raw 16-bit array
// prhs[2] - numeric - number of the received words
// prhs[3] - numeric - xSize of frame
// prhs[4] - numeric - ySize of frame
// prhs[5] - uint8*  - output frame image array (8 bit, scaled)
// prhs[6] - numeric - shift factor for frame image array
// prhs[7] - double* - output frame 'math' array (double)
//
// plhs[0] - numeric - status
// plhs[1] - double  - frameNum
//------------------------------------------------------------------------------
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    const unsigned NoError          = 0;
    const unsigned BadParams        = 1;
    const unsigned FrameHeaderError = 2;

    unsigned res = NoError;
    
    if(!checkParams(nrhs,prhs)) {
        res = BadParams;
        if(nlhs > 0) {
            plhs[0] = mxCreateDoubleScalar(res);
            if(nlhs > 1) {
                plhs[1] = mxCreateDoubleScalar(-1);
            }
        }
    }

    //---
    unsigned cmdMask = static_cast<unsigned>(mxGetScalar(prhs[0]));
    uint16_t* inBuf = static_cast<uint16_t*>(mxGetData(prhs[1]));
    unsigned payloadSize = static_cast<unsigned>(mxGetScalar(prhs[2]));
    unsigned xSize = static_cast<unsigned>(mxGetScalar(prhs[3]));
    unsigned ySize = static_cast<unsigned>(mxGetScalar(prhs[4]));
    uint32_t frameNum; 
    
    //---
    if(!checkFrame(inBuf,payloadSize,xSize,ySize,frameNum)) {
        res = FrameHeaderError;
    }
    
    //---
    if((res == NoError) && (cmdMask & 0x1)) {
        mxUnshareArray(const_cast<mxArray *>(prhs[5]), true);
        uint8_t* imgFrame = static_cast<uint8_t*>(mxGetData(prhs[5]));
        unsigned shiftFactor = static_cast<unsigned>(mxGetScalar(prhs[6]));
        getImgFrame(inBuf+FrameHeaderLen,xSize,ySize,imgFrame,shiftFactor);
    }
    //---
    if((res == NoError) && (cmdMask & 0x2)) {
        mxUnshareArray(const_cast<mxArray *>(prhs[7]), true);
        mxClassID elemClassId = mxGetClassID(prhs[7]);
        switch(elemClassId) {
            case mxSINGLE_CLASS:
                {
                    typedef float T;
                    T* mathFrame = static_cast<T*>(mxGetData(prhs[7]));
                    getMathFrame<T>(inBuf+FrameHeaderLen,xSize,ySize,mathFrame);
                }
                break;
            case mxDOUBLE_CLASS:
                {
                    typedef double T;
                    T* mathFrame = static_cast<T*>(mxGetData(prhs[7]));
                    getMathFrame<T>(inBuf+FrameHeaderLen,xSize,ySize,mathFrame);
                }
                break;

            default:
                break;
        }
    }
    
    if(nlhs > 0) {
        plhs[0] = mxCreateDoubleScalar(res);
        if(nlhs > 1) {
            plhs[1] = mxCreateDoubleScalar(frameNum);
        }
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool checkParams(int nrhs, const mxArray *prhs[])
{
    if(nrhs < 8) {
        mexErrMsgTxt("[getRxFrame] bad number of input parameters\n");
        return false;
    }
    if(
          !mxIsNumeric(prhs[0]) // cmd mask for ena/dis output arrays forming
       || !mxIsUint16(prhs[1])  // input raw 16-bit array
       || !mxIsNumeric(prhs[2]) // number of the received words
       || !mxIsNumeric(prhs[3]) // xSize of frame
       || !mxIsNumeric(prhs[4]) // ySize of frame
       || !mxIsUint8(prhs[5])   // output frame image array (8 bit, scaled)
       || !mxIsNumeric(prhs[6]) // shift factor for frame image array
       || !mxIsNumeric(prhs[7]) // output frame 'math' array
       )
        return false;
    return true;    
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool checkFrame(const uint16_t* inBuf, unsigned payloadSize, unsigned xSize, unsigned ySize, uint32_t& frameNum)
{
    const uint32_t* frameHeader = reinterpret_cast<const uint32_t*>(inBuf);
    
    //uint32_t ClassId    = frameHeader[0];
    //uint32_t MsgClassId = frameHeader[1];
    //uint32_t NetSrc     = frameHeader[2];
    //uint32_t NetDst     = frameHeader[3];
    frameNum            = frameHeader[4];
    //uint32_t PixelSize  = frameHeader[5];
    uint32_t SizeY      = frameHeader[6];
    uint32_t SizeX      = frameHeader[7];
    
    if((SizeX != xSize) || (SizeY != ySize) || (payloadSize != (SizeX*SizeY + FrameHeaderLen)))
        return false;
    
    return true;    
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void getImgFrame(const uint16_t* inBuf, unsigned xSize, unsigned ySize, uint8_t* imgFrame, unsigned shiftFactor)
{
     for(unsigned i = 0; i < xSize; ++i) {
        for(unsigned j = 0; j < ySize; ++j) {
            *imgFrame++ = (*(inBuf + j*xSize + i)) >> shiftFactor;
        }
     }
}
