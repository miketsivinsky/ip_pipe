%--------------------------------------------------------------------------
function [hImg,h3D] = initGraphics3(imgBuf,fft2Buf)
    import ip_pipe.*;
    
    BgFigColor = [0.5 0.5 0.5];
    maxGrayValue = double(intmax('uint8'));

    %---
    imgPos = [0.0 0.0 1.0 1.0];
    cImgLim = [0 maxGrayValue];
    colorSlice = (0:maxGrayValue)/maxGrayValue;
    colorMap1 = [colorSlice; colorSlice; colorSlice]';
    
    %---
    fft2pos   = [0.0 0.0 1.0 1.0];
    cLim      = [0 maxGrayValue];
    zLim      = [0 10];
    colorMap2 = jet(maxGrayValue+1);
    
    %---
    [hFig1]  = initFig('input',BgFigColor,colorMap1);
    [hFig2]  = initFig('fft2',BgFigColor,colorMap2);
    [hImg]   = initImg(imgBuf, hFig1, imgPos, cImgLim);
    [h3D]    = init3D(fft2Buf, hFig2, fft2pos, cLim, zLim);
    %colorbar
    drawnow;
end
