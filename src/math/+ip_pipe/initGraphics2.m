%--------------------------------------------------------------------------
function [hImg,hLine] = initGraphics2(imgBuf,graphBuf)
    import ip_pipe.*;
    
    BgFigColor = [0.5 0.5 0.5];
    maxGrayValue = double(intmax('uint8'));

    %---
    imgPos = [0.0 0.5 1.0 0.5];
    cImgLim = [0 maxGrayValue];
    colorSlice = (0:maxGrayValue)/maxGrayValue;
    colorMap = [colorSlice; colorSlice; colorSlice]';
    
    %---
    LineType = 1;
    LineSize = numel(graphBuf); 
    linePos = [0.0 0.0 1.0 0.5];
    lineBkColor = [0.4 0.4 0.4];
    lineGraphColor = 'b';
    lineXLim = [0 LineSize-1];
    lineYLim = [0 1023];
    lineXData = 0:LineSize-1;
    lineYData = graphBuf;

    %---
    [hFig]  = initFig('slon',BgFigColor,colorMap);
    [hImg]  = initImg(imgBuf, hFig, imgPos, cImgLim);
    [hLine] = initLine(lineXData,lineYData, LineType, hFig, linePos, lineBkColor, lineGraphColor,lineXLim, lineYLim);
    drawnow;
end
