%---
DataType  = 'uint16';
Test      = 1;
IncValue  = 7;
MemInfo   = 0;
StartPos  = 16;
EnaReSync = 1;

Name    = 'toMath';
Type    = 'rx';

ChunkSize   = 1024*1024;
ChunkNum    = 256;
PacketNum   = 500; %8*1024;
PrintFactor = 100;

%---
import ip_pipe.*
    
if(strcmp(Type,'tx'))
    DataType = 'uint64';
end    
IncValue = cast(IncValue,DataType);
testVal  = cast(0,DataType);
bufSize = 0;
elemSize = sizeof(DataType);
BufSize = ChunkSize/elemSize;
buf = genBuf(1,BufSize,DataType);

%--------------------------------------------------------------------------
[status, FullName] = ipPipeStart(Name,Type,ChunkSize,ChunkNum,1000);
if(status ~= 0)
    return;
end    

%---
TestConditionRx = Test & strcmp(Type,'rx');
TestConditionTx = Test & strcmp(Type,'tx');
pktError = 0;
errFlag = 0;

%---
if MemInfo
    [u1,s1] = memory;
end

%---
totalWords = 0;
tic;
for pkt = 1:PacketNum
    if(TestConditionTx)
        buf = typecast((testVal:IncValue:(testVal+IncValue*(BufSize-1))),DataType);
        testVal = buf(end) + IncValue;
    end
    [res,bufSize] = ipPipeCmd('transferBuf',FullName,BufSize,buf,1000);
    totalWords =  totalWords + bufSize;
    if(res ~= 0)
        fprintf(1,'transferBuf status %d, bufSize: %d, BufSize: %d\n',res,bufSize,BufSize);
        break;
    end    
    if(TestConditionRx)
        [errNum,testVal] = checkBuf(buf,StartPos,(bufSize-StartPos),testVal,IncValue,EnaReSync);
        pktError = pktError + errNum;
    end    
    if(rem(pkt,PrintFactor) == 0)
        fprintf(1,'Pkt: %8d, pktError: %8d\n',pkt,pktError);
    end    
end
testTime = toc;

%---
if MemInfo
    DivKB = 1024;
    [u2,s2] = memory;
    dMaxPossibleArrayBytes         = u2.MaxPossibleArrayBytes - u1.MaxPossibleArrayBytes;
    dMemAvailableAllArrays         = u2.MemAvailableAllArrays - u1.MemAvailableAllArrays;
    dMemUsedMATLAB                 = u2.MemUsedMATLAB - u1.MemUsedMATLAB;
    dVirtualAddressSpaceAvailable  = s2.VirtualAddressSpace.Available - s1.VirtualAddressSpace.Available;
    dSystemMemoryAvailable         = s2.SystemMemory.Available - s1.SystemMemory.Available;
    dPhysicalMemoryAvailable       = s2.PhysicalMemory.Available - s1.PhysicalMemory.Available;
    
    fprintf(1,'[user] MaxPossibleArrayBytes         delta %12d\n',dMaxPossibleArrayBytes/DivKB);
    fprintf(1,'[user] MemAvailableAllArrays         delta %12d\n',dMemAvailableAllArrays/DivKB);
    fprintf(1,'[user] MemUsedMATLAB                 delta %12d\n',dMemUsedMATLAB/DivKB);
    fprintf(1,'[sys]  VirtualAddressSpace.Available delta %12d\n',dVirtualAddressSpaceAvailable/DivKB);
    fprintf(1,'[sys]  SystemMemory.Available        delta %12d\n',dSystemMemoryAvailable/DivKB);
    fprintf(1,'[sys]  PhysicalMemory.Available      delta %12d\n',dPhysicalMemoryAvailable/DivKB);
end

%---
fprintf(1,'-----------------------------------------------------------------------\n');
fprintf(1,'[%s] end\n\n',FullName);
fprintf(1,'test enabled:  %15d\n',Test);
fprintf(1,'total pkt:     %15d\n',PacketNum);
fprintf(1,'total time:    %15.0f ms\n',testTime*1000);
fprintf(1,'transfer rate: %15.1f MB/s\n',(totalWords*elemSize/testTime)/1e6);
fprintf(1,'word size:     %15d\n',elemSize);
fprintf(1,'total bytes:   %15d\n',totalWords*elemSize);
fprintf(1,'total words:   %15d\n',totalWords);
fprintf(1,'total error:   %15d\n',pktError);
fprintf(1,'-----------------------------------------------------------------------\n\n');

clear ipPipeCmd;
clear checkBuf;

