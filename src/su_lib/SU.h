#ifndef SU_H
#define SU_H

#include <windows.h>
#include <string>
#include <tchar.h>

// System Utilities
namespace SU { 

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
typedef std::basic_string<TCHAR> TString;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<class T> class TAutoExit
{
 public:
  TAutoExit(T& item) : m_item(item) { m_item.Enter(); }
  ~TAutoExit() { m_item.Exit(); }

 private:
  T& m_item;
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class TCriticalSection
{
 public:
  TCriticalSection();
  ~TCriticalSection();
  void Enter();
  void Leave();
  void Exit() { Leave(); }


 private:
  TCriticalSection(TCriticalSection& CS) {}

  CRITICAL_SECTION m_CS;
};

typedef TAutoExit<TCriticalSection> TAutoExitCS;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class TEvent
{
 public: 
  typedef enum {_SIGNALED = WAIT_OBJECT_0, _TIME_OUT = WAIT_TIMEOUT} TResult;

  TEvent(bool fManualReset = false);
  TEvent(bool fManualReset, bool fInitialState);
  ~TEvent();
  void Set();
  void Reset();
  TResult Wait(DWORD dwMilliSeconds);
  HANDLE GetHandle() const { return m_hEvent; }

 private:
  TEvent(const TEvent& ev){}
  HANDLE	m_hEvent;
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class TSemaphore
{
 public:
  TSemaphore(LONG initCount, LONG maxCount, LPCTSTR name) : mSemaphore(NULL)
  {
    mSemaphore = ::CreateSemaphore(NULL, initCount, maxCount, name);
	if(::GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(TEXT("[WARN] semaphore %s already exists\n"),name);
	}
	//_tprintf(TEXT("[INFO] [TSemaphore constructor] %s 0x%08x\n"),name,mSemaphore);
  }
  ~TSemaphore() 
  {
    ::CloseHandle(mSemaphore);
	 //_tprintf(TEXT("[INFO] [TSemaphore destructor]\n"));
  }
  BOOL release(LONG lReleaseCount_ = 1)
  {
   return ::ReleaseSemaphore(mSemaphore, lReleaseCount_, NULL);
  }
  DWORD wait(DWORD dwMilliSeconds = INFINITE) { return ::WaitForSingleObject(mSemaphore, dwMilliSeconds); }

 private:
  HANDLE mSemaphore;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class TElapsedTimer
{
 public:
  TElapsedTimer() 
   {
    sFrequency_m.QuadPart = 0; 
    sStartTime_m.QuadPart = 0;
    sStopTime_m.QuadPart = 0;
    BOOL bIsSupported_ = Calibrate();
   }
  
  BOOL Calibrate() { return ::QueryPerformanceFrequency(&sFrequency_m); }
  LONGLONG GetFrequency() const { return sFrequency_m.QuadPart; }
  void Start() { ::QueryPerformanceCounter(&sStartTime_m); }
  void Catch()  { ::QueryPerformanceCounter(&sStopTime_m); }
  LONGLONG GetStopDelta() const // in us
   {
    LONGLONG llDelta_ = sStopTime_m.QuadPart - sStartTime_m.QuadPart;
    return (1000000*llDelta_)/sFrequency_m.QuadPart;
   }

 private:
  LARGE_INTEGER sFrequency_m; 
  LARGE_INTEGER sStartTime_m;
  LARGE_INTEGER sStopTime_m;
};

};

#endif	// SU_H