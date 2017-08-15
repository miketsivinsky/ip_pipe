%--------------------------------------------------------------------------
function [hImg] = initGraphics1(imgBuf)
    import ip_pipe.*;
    
    BgFigColor = [0.5 0.5 0.5];
    maxGrayValue = double(intmax('uint8'));
    imgPos = [0.0 0.0 1.0 1.0];
    cImgLim = [0 maxGrayValue];
    colorSlice = (0:maxGrayValue)/maxGrayValue;
    colorMap = [colorSlice; colorSlice; colorSlice]';

    [hFig] = initFig('slon',BgFigColor,colorMap);
    [hImg] = initImg(imgBuf, hFig, imgPos, cImgLim);
    
    drawnow;
end
