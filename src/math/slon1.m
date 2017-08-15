%--------------------------------------------------------------------------
close all;
clear all;

%--------------------------------------------------------------------------
XSize = 640;
YSize = 480;
in_t  = 'double';
out_t = 'double';

in = cast(rand(YSize,XSize)-0.5,in_t);

%--------------------------------------------------------------------------
N1 = 1000;
x1s = 2;
M1 = 512;

x1e = x1s + M1 - 1;
out1 = zeros(1,M1,out_t);

startTime = tic;
for k = 1:N1
    in(1,:) = rand(1,XSize)-0.5;
    out1 = cast(abs(fftshift(fft(in(1,x1s:x1e)))/M1),out_t);
end
totalTime = toc(startTime);
fftTime = totalTime/N1;
fprintf(1,'[fft] total time:   %15.0f us\n',fftTime*1e6);

figure;
plot(out1);
grid;

return;
%--------------------------------------------------------------------------
N2 = 10;
x2s = 4;
y2s = 4;
M2 = 256;

x2e = x2s + M2 - 1;
y2e = y2s + M2 - 1;

out2 = zeros(M2,M2,out_t);

startTime = tic;
for k = 1:N2
    out2 = cast(abs(fftshift(fft2(in(y2s:y2e,x2s:x2e)))/M2^2),out_t);
end
totalTime = toc(startTime);
fft2Time = totalTime/N2;
fprintf(1,'[fft2] total time:  %15.0f us\n',fft2Time*1e6);

figure;
%surf(double(out2))
imshow(double(out2),[])

%--------------------------------------------------------------------------
%w = ones(3);
w = fspecial('laplacian',0);
for k = 1:10
tic
%out3 = imfilter(out2,w);
save('slon.mat','out2');
toc
end
figure;
%imshow(out3,[]);


