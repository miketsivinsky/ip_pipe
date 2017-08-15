//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if !defined(IP_PIPE_DEFS)
#define IP_PIPE_DEFS

namespace IP_pipe
{
	typedef enum TStatus
	{
		Ok                 = 0,
		NotReady           = 1,
		TimeoutExpired     = 2,
		PipeEmpty          = 3,
		PipeFull           = 4,
		BadBufSize         = 5,
		TimeoutRx          = 6,
		TimeoutTx          = 7,
		NotExist           = 8,
		OtherError         = 9
	};
}

#endif /* IP_PIPE_DEFS */

