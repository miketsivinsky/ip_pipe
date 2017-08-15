//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "mex.h"
#include "matrix.h"
#include <cstdint>

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template <typename T> static T checkBufFunc(void* buf, unsigned start_pos, unsigned checkLen, double currValue, double incValue, unsigned& errNum, double enaStreamResync)
{
    T* bufT = reinterpret_cast<T*>(buf) + start_pos;
    T currValueT = static_cast<T>(currValue);
    const T IncValueT = static_cast<T>(incValue);
    
    errNum = 0;
    for(unsigned k = 0; k < checkLen; ++k) {
        if(*bufT != currValueT) {
            if(enaStreamResync) {
                currValueT = *bufT;
            } 
            ++errNum;
        }    
        ++bufT;
        currValueT += IncValueT;
    }
    return currValueT;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template <typename T> static void setNextValue(int nlhs, mxArray *plhs[], mxClassID elemClassId, T nextValue)
{
    if(nlhs == 2) {
        mwSize ndim = 1;
        mwSize dims = 1;
        mxArray* nextValueArray = mxCreateNumericArray(ndim,&dims,elemClassId,mxREAL);
        plhs[1] = nextValueArray;
        *(reinterpret_cast<T*>(mxGetData(nextValueArray))) = nextValue;
    }
}

//------------------------------------------------------------------------------
// prhs[0] - numeric*  - checked input buf
// prhs[1] - numeric   - startPos, start position of buf check
// prhs[2] - numeric   - checkLen, length of checked sequence
// prhs[3] - numeric   - testValue, buffer[startPos] must be equal this value
// prhs[4] - numeric   - incValue, buffer[n+1] must be equal buffer[n] + incValue
// prhs[5] - numeric   - enaStreamResync, when == 1, testValue adapted when error occured
//
// plhs[0] - double    - errNum
// plhs[1] - numeric   - testValue which must be equal buffer[startPos] for next buffer
//------------------------------------------------------------------------------
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if(nrhs != 6) {
        mexErrMsgTxt("[checkBuf] bad number of input parameters\n");
    }
    
    mxClassID elemClassId = mxGetClassID(prhs[0]);
    if((elemClassId != mxGetClassID(prhs[3])) || (elemClassId != mxGetClassID(prhs[4]))) {
        mexErrMsgTxt("[checkBuf] bad input parameters\n");
    }
    
    void* buf = mxGetData(prhs[0]);
    unsigned startPos = static_cast<unsigned>(mxGetScalar(prhs[1]));
    size_t  checkLen = static_cast<size_t>(mxGetScalar(prhs[2]));
    double currValue = static_cast<double>(mxGetScalar(prhs[3]));
    double incValue  = static_cast<double>(mxGetScalar(prhs[4]));
    double enaStreamResync = mxGetScalar(prhs[5]);
    
    unsigned errNum;
    switch(elemClassId) {
        case mxUINT16_CLASS:
            setNextValue<uint16_t>(nlhs,plhs,elemClassId,checkBufFunc<uint16_t>(buf,startPos,checkLen,currValue,incValue,errNum,enaStreamResync));
            break;
        case mxUINT32_CLASS:
            setNextValue<uint32_t>(nlhs,plhs,elemClassId,checkBufFunc<uint32_t>(buf,startPos,checkLen,currValue,incValue,errNum,enaStreamResync));
            break;
        case mxUINT64_CLASS:
            setNextValue<uint64_t>(nlhs,plhs,elemClassId,checkBufFunc<uint64_t>(buf,startPos,checkLen,currValue,incValue,errNum,enaStreamResync));
            break;
        default:
            mexErrMsgTxt("[checkBuf] unsupported data types\n");
            break;
    }
   
    if(nlhs) {
        plhs[0] = mxCreateDoubleScalar(errNum);
    }
}
