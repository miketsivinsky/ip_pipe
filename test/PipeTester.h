//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if !defined(PIPE_TESTER)
#define PIPE_TESTER

#include <windows.h>
#include <cstdint>
#include <tchar.h>

#include "SU.h"

//------------------------------------------------------------------------------
template<typename T> class TPipeTester
{
    public:
		static unsigned ElemSize() { return sizeof(T); }
		static TCHAR* num2str(uint64_t num);

        TPipeTester(uint32_t packetSize, uint32_t packetNum, bool enaTest);
        virtual ~TPipeTester() { delete [] mBuf; }
        uint8_t* pipePacket() { return reinterpret_cast<uint8_t*>(mBuf); }
        uint32_t& transferPacketSize() { mPacketSize = mPacketSizeParam*sizeof(T); return mPacketSize; }
		uint32_t getPacketSize() const { return mPacketSize; }
        uint32_t packetNum() const { return mPacketNum; }
		void packetNumInc() { ++mPacketNum; }
        bool isTestFinish() { return mPacketNum >= mPacketNumParam; }
        uint32_t packetErrors() const { return mPacketErrors; }
        uint32_t totalErrors() const { return mTotalErrors; }
		bool enaTest() const { return mEnaTest; }
		uint64_t getTransferredWords() const { return getTransferredBytes()/sizeof(T); /*return ((uint64_t)packetNum())*mPacketSizeParam;*/ }
		uint64_t getTransferredBytes() const { return mTransferredBytes; /*return getTransferredWords()*sizeof(T);*/ }

        virtual void preProcess(bool bypass) = 0;
		virtual void postProcess(bool) { mTransferredBytes += mPacketSize; }
		void timerStart() { mTimer.Start(); }
		void timerCatch() { mTimer.Catch(); }
		uint64_t elapsedTime_us() { return mTimer.GetStopDelta(); }
		uint64_t elapsedTime_ms() { return mTimer.GetStopDelta()/1000; }
		uint64_t transferRate() { return getTransferredBytes()/elapsedTime_us(); } // MB/s
		double fTransferRate() { return double(getTransferredBytes())/elapsedTime_us(); } // MB/s

    protected:
        void incValue() {  mValue += IncValue; }

		static const T    StartValue = 0;
        static const T	  IncValue   = 7;

		const uint32_t    mPacketSizeParam;
        const uint32_t    mPacketNumParam;

        uint32_t          mPacketSize;
        T*                mBuf;
        T                 mValue;
        uint32_t          mPacketNum;
        uint32_t          mTotalErrors;
        uint32_t          mPacketErrors;
		bool              mEnaTest;
		SU::TElapsedTimer mTimer;
		uint64_t          mTransferredBytes;
};

//------------------------------------------------------------------------------
template <typename T> class TTxPipeTester : public TPipeTester<T>
{
    public:
        TTxPipeTester(uint32_t packetSize, uint32_t packetNum, bool enaTest = true) : TPipeTester(packetSize, packetNum, enaTest) {}
        virtual void preProcess(bool bypass);
};

//------------------------------------------------------------------------------
template <typename T> class TRxPipeTester : public TPipeTester<T>
{
    public:
        TRxPipeTester(uint32_t packetSize, uint32_t packetNum, bool enaTest = true, bool enaStreamSync = false) : TPipeTester(packetSize, packetNum, enaTest), mEnaStreamSync(enaStreamSync) {}
        virtual void preProcess(bool) { }
        virtual void postProcess(bool bypass);

	private:
		bool mEnaStreamSync;
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// very very ugly :(
//------------------------------------------------------------------------------
template<typename T> TCHAR* TPipeTester<T>::num2str(uint64_t num)
{
	const unsigned BufSize  = 128;
	const unsigned GroupLen = 3;
	const TCHAR GroupDivider = TEXT(',');
	static TCHAR outBuf[BufSize];
	TCHAR tmpBuf[BufSize];

	_sntprintf_s(outBuf,BufSize/2,TEXT("%lld"),num);
	size_t len = _tcslen(outBuf);
	unsigned idxOut = 0;
	unsigned numDigit = 0;
	for(size_t idxIn = len; idxIn; --idxIn) {
		tmpBuf[idxOut++] = outBuf[idxIn-1];
		if((++numDigit == GroupLen) && (idxIn-1)) {
			numDigit = 0;
			tmpBuf[idxOut++] = GroupDivider;
		} 
	}
	tmpBuf[idxOut] = 0;
	len = _tcslen(tmpBuf);
	idxOut = 0;
	for(size_t idxIn = len; idxIn; --idxIn) {
		outBuf[idxOut++] = tmpBuf[idxIn-1];
	}
	outBuf[idxOut] = 0;
	return outBuf;
}

//------------------------------------------------------------------------------
template<typename T> TPipeTester<T>::TPipeTester(uint32_t packetSize, uint32_t packetNum, bool enaTest) :
                                           mPacketSizeParam(packetSize),
                                           mBuf(0),
                                           mPacketNumParam(packetNum),
                                           mValue(StartValue),
                                           mPacketNum(0),
                                           mPacketErrors(0),
										   mTotalErrors(0),
										   mEnaTest(enaTest),
										   mTransferredBytes(0)
{
	mTimer.Calibrate();
	mBuf = new T [mPacketSizeParam];
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<typename T> void TTxPipeTester<T>::preProcess(bool bypass)
{
	if(bypass || !mEnaTest)
		return;

    T* buf = mBuf;
    for(unsigned k = 0; k < mPacketSizeParam; ++k) {
		*buf++ = mValue;
		//DELETE THIS LINE!!! printf("mValue = %d\n",mValue);
		//if(k == 300)
		//	*(buf-1) = 100;
        incValue();
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<typename T> void TRxPipeTester<T>::postProcess(bool bypass)
{
	TPipeTester<T>::postProcess(bypass);
	if(bypass || !mEnaTest)
		return;

	mPacketErrors = 0;
	T* buf = mBuf;
    for(unsigned k = 0; k < /*mPacketSizeParam*/ mPacketSize/sizeof(T); ++k) {
		//DELETE THIS LINE!!! _tprintf(TEXT("[+++] idx = %4d, %6d != %6d\n"),k, *buf, mValue);
		if(*buf != mValue) {
			//_tprintf(TEXT("[***] idx = %4d, received(%6d) != expected(%6d)\n"),k, *buf, mValue);
			if(mEnaStreamSync) {
				_tprintf(TEXT("[***] idx = %4d, received(%6ld) != expected(%6ld)\n"),k, int(*buf), int(mValue));
				mValue = *buf;
			}
            ++mPacketErrors;
         }
        ++buf;
        incValue();
    }
	mTotalErrors += mPacketErrors;
}



#endif // PIPE_TESTER


