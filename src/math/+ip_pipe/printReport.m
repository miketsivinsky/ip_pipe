%--------------------------------------------------------------------------
function printReport(fullName,rxBufNumRvd,totalTime,rxTotalWords,rxElemSize,rxMissedFrames)
    fprintf(1,'-----------------------------------------------------------------------\n');
    fprintf(1,'[%s] finished\n\n',fullName);
    fprintf(1,'rx buf received:  %15d\n',rxBufNumRvd);
    fprintf(1,'rx missed frames: %15d\n',rxMissedFrames);
    fprintf(1,'total time:       %15.0f ms\n',totalTime*1000);
    fprintf(1,'rx transfer rate: %15.1f MB/s\n',(rxTotalWords*rxElemSize/totalTime)/1e6);
    fprintf(1,'rx total words:   %15d\n',rxTotalWords);
    fprintf(1,'-----------------------------------------------------------------------\n\n');
end