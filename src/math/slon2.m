%--------------------------------------------------------------------------
function slon2

close all;
clear all;

N = 128;
MaxGrayValue = 255;
in_t  = 'double';
out_t = 'double';

x = 0:N-1;
y = 0:N-1;

[xx,yy] = meshgrid(x,y);
Z = cast(xx+yy+1,out_t);

colorSlice = (0:MaxGrayValue)/MaxGrayValue;
colorMap = [colorSlice; colorSlice; colorSlice]';
colorMap(1,:) = [0 0 1];
colorMap(255,:) = [1 0 1];

%hFig = figure('Colormap',colorMap);
hFig = figure;
hAxes = axes('Parent',hFig,'ZLim',[0 MaxGrayValue]);
h3D = surf(hAxes,xx,yy,Z,'CDataMapping','scaled','VertexNormals',[]);
set(hAxes,'XLim',[0 N-1],'YLim',[0 N-1],'ZLim',[0 MaxGrayValue],'CLim',[0 MaxGrayValue]);
%colorbar
xlabel(hAxes,'x','FontSize',12);
ylabel(hAxes,'y','FontSize',12);
%get(h3D)


K = 100;

totalTime = 0;
for k = 1:K
    Zin = testGen(N,k,255,in_t);
    startTime = tic;
    Zout = abs(fftshift(fft2(Zin))/N^2);
    Zout = log2(1 + Zout)*32;
    %Zout = Zin;
    set(h3D,'ZData',Zout); drawnow;
    totalTime = totalTime + toc(startTime);
    Z = Z+1;
    Z(Z > 255) = 255;
end
timeGraph3D = totalTime/K;
fprintf(1,'timeGraph3D:  %10.1f ms\n',timeGraph3D*1e3);

%get(hAxes)

end

%--------------------------------------------------------------------------
function [out] = testGen(frameSize,frameNum,A,type)
    out = zeros(frameSize,frameSize,type);
    fSize2 = frameSize/2;
    sideLen2 = rem(frameNum,fSize2) + 1;
    offset = fSize2 - sideLen2;
    startIdx = 1 + offset;
    endIdx   = startIdx + 2*sideLen2 - 1;
    out(startIdx:endIdx,startIdx:endIdx) = A;
end

% <SLON> rxMathFFT2 = abs(fftshift(fft2(single(rxMathFrame(1:256,1:256))))/256^2);
