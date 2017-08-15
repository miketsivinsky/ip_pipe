%--------------------------------------------------------------------------
function [elemSize] = sizeof(dataType)
    MaxValue = intmax(dataType); %#ok<NASGU>
    s = whos('MaxValue');
    elemSize = s.bytes;
end