%--------------------------------------------------------------------------
function [status, fullName] = ipPipeStart(name, type, chunkSize, chunkNum, initTimeout)
    status = 0;
    fullName = [name '_' type];
    fprintf(1,'[%s] start\n\n',fullName);
    ipPipeCmd('createPipeView',name,type,chunkSize,chunkNum);
    ipPipeCmd('isPipeViewExist',fullName);
    ipPipeCmd('setRdy',fullName);
    if(~ipPipeCmd('isPipeReady',fullName))
        status = ipPipeCmd('waitPeerRdy',fullName,initTimeout);
        if(status ~= 0)
            fprintf(1,'[%s] error status: %d\n',fullName,status);
            clear ipPipeCmd;
            return;
        end
    end
    fprintf(1,'[%s] rdy: %1d, peer rdy: %1d\n\n',fullName,ipPipeCmd('isReady',fullName),ipPipeCmd('isPeerReady',fullName));
end
