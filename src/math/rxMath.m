%--------------------------------------------------------------------------
function rxMath

%--------------------------------------------------------------------------
close all;
clear all;

import ip_pipe.*

%----- work mode ---------
OnlyImg    = 1;
ImgAndFFT  = 2;
ImgAndFFT2 = 3;

WorkMode = ImgAndFFT2;

%----- rx parameters -----
RxBufNum      = 1000;
RxPrintFactor = 100;

%--- pipe parameters
RxPipeInitTimeout     = 1000;
RxPipeTransferTimeout = 100;
RxPipeChunkSize       = 1024*1024;
RxPipeChunkNum        = 256;
RxPipeName            = 'toMath';

%--- frame parameters
RxFrameSizeX = 800;
RxFrameSizeY = 600;
ShiftFactor  = 2;

%--- alg parameters
LenFFT  = 512;
LineFFT = 300;

LenFFT2 = 128;

%--------------------------------------------------------------------------
[status, RxPipeFullName] = ipPipeStart(RxPipeName,'rx',RxPipeChunkSize,RxPipeChunkNum,RxPipeInitTimeout);
if(status ~= 0)
    return;
end

rxBufElemSize = sizeof('uint16');
rxBuf = genBuf(1,RxPipeChunkSize/rxBufElemSize,'uint16');
rxImgFrame  = genBuf(RxFrameSizeY,RxFrameSizeX,'uint8');
rxMathFrame = genBuf(RxFrameSizeY,RxFrameSizeX,'single');

switch WorkMode
    case OnlyImg
        GetRxFrameMode = 1;
        rxImgFrameHadle = initGraphics1(rxImgFrame);
    case ImgAndFFT
        GetRxFrameMode = 3;
        rxMathFFT = genBuf(1,LenFFT,'single');
        [rxImgFrameHadle,rxFFT_LineHandle] = initGraphics2(rxImgFrame,rxMathFFT(1:LenFFT/2));
    case ImgAndFFT2
        GetRxFrameMode = 3;
        [siX, eiX, siY, eiY] = computeFFT2_Zone(RxFrameSizeX, RxFrameSizeY, LenFFT2);
        [rxImgFrameHadle,rxFFT2_ImgHandle] = initGraphics3(rxImgFrame,rxMathFrame(siX:eiX,siY:eiY));
    otherwise
        GetRxFrameMode = 0;
end

%--------------------------------------------------------------------------
nRxBuf         = 0;
rxTotalWords   = 0;
jobTimeVec     = zeros(1,RxBufNum);
framePeriodVec = zeros(1,RxBufNum-1);
rxPrevFrameNum = 0;
rxMissedFrames = 0;

startTime = tic;
while(nRxBuf < RxBufNum)
    [status,rxWordsRvd] = ipPipeCmd('transferBuf',RxPipeFullName,0,rxBuf,RxPipeTransferTimeout);
    if(status ~= 0)
        fprintf(1,'[rx] transferBuf status %d, bufSize: %d\n',status,rxWordsRvd);
        break;
    end
    rxTotalWords =  rxTotalWords + rxWordsRvd;
    nRxBuf = nRxBuf + 1;
    if(nRxBuf > 1)
        framePeriodVec(nRxBuf-1) = toc(prevPacketTime);
    end
    prevPacketTime = tic;
    
    jobTime = tic;                     %---
    [status,rxFrameNum] = getRxFrame(GetRxFrameMode,rxBuf,rxWordsRvd,RxFrameSizeX,RxFrameSizeY,rxImgFrame,ShiftFactor,rxMathFrame);
    set(rxImgFrameHadle,'CData',rxImgFrame);
    if(WorkMode == ImgAndFFT)
        rxMathFFT = abs(fft(rxMathFrame(LineFFT,1:LenFFT))/LenFFT);
        set(rxFFT_LineHandle,'YData',rxMathFFT(1:LenFFT/2));
    end
    if(WorkMode == ImgAndFFT2)
        rxMathFFT2 = abs(fftshift(fft2(rxMathFrame(siX:eiX,siY:eiY)))/LenFFT2^2);
        set(rxFFT2_ImgHandle,'ZData',rxMathFFT2);
    end
    drawnow;
    if(nRxBuf <= 1)
        rxPrevFrameNum = rxFrameNum;
    else
        rxPrevFrameNum = rxPrevFrameNum + 1;
        if(rxPrevFrameNum ~= rxFrameNum)
            rxMissedFrames = rxMissedFrames + 1;
            rxPrevFrameNum = rxFrameNum;
        end    
    end
    jobTimeVec(nRxBuf) = toc(jobTime); %---
    
    if(status ~= 0)
        fprintf(1,'[rx] getRxFrame status %d\n',status);
        break;
    end
    if(rem(nRxBuf,RxPrintFactor) == 0)
        fprintf(1,'rxBuf received: %4d\n',nRxBuf);
    end    
end    
totalTime = toc(startTime);

%--------------------------------------------------------------------------
printReport(RxPipeFullName,nRxBuf,totalTime,rxTotalWords,rxBufElemSize,rxMissedFrames);

%--------------------------------------------------------------------------
% figure;
% plot(jobTimeVec*1000);
% grid;

% figure;
% plot(framePeriodVec*1000);
% grid;

%fprintf(1,'min frame period: %15.0f ms\n',min(framePeriodVec)*1000);
%fprintf(1,'max frame period: %15.0f ms\n',max(framePeriodVec)*1000);    
fprintf(1,'avg frame period: %15.0f ms\n\n',mean(framePeriodVec)*1000);

%fprintf(1,'min job time:     %15.0f ms\n',min(jobTimeVec)*1000);
fprintf(1,'max job time:     %15.0f ms\n',max(jobTimeVec)*1000);
fprintf(1,'avg job time:     %15.0f ms\n',mean(jobTimeVec)*1000);

clear ipPipeCmd;
%clear getRxFrame;

end

%--------------------------------------------------------------------------
function [siX, eiX, siY, eiY] = computeFFT2_Zone(sizeX, sizeY, fft2N)
    N2 = fft2N/2;

    offsetX = sizeX/2 - N2;
    siX = offsetX + 1;
    eiX = siX + 2*N2 - 1;

    offsetY = sizeY/2 - N2;
    siY = offsetY + 1;
    eiY = siY + 2*N2 - 1;
end
