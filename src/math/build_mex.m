clc
mex -largeArrayDims -outdir . -I..\ip_pipe_lib -I..\su_lib +ip_pipe\ipPipeCmd.cpp ..\ip_pipe_lib\ip_pipe.cpp
mex -largeArrayDims -outdir . +ip_pipe\getRxFrame.cpp
mex -largeArrayDims -outdir . +ip_pipe\checkBuf.cpp

