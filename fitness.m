function [Fspot, auxOutput] = fitness(X,extPar)
auxOutput = cell(1,size(X,2));
% This version is for comparison with Slavko's synthetic data
for i=size(X,2):-1:1  % we want to count backwards here
    % Apply constraints
    isPhysical=applyConstraints(X(:,i),extPar);
    disp('------ Fitness ------');
    if isPhysical
        % Parameters from vector X.
        tic;
        radius=X(1,i);
        mass=X(2,i);
        inclination=X(3,i);
        emission=X(4,i);
        timeShift=X(5,i);
        %        ObsTime=X(6,i);
        ObsTime=extPar.fixed.obstime;
        rho=X(6,i);
        %rho = extPar.fixed.rho;
        temperature=X(7,i);
        %temperature=extPar.fixed.spot_temperature;
        
        % Initialize background to zero
        %background1=extPar.fixed.background(1);
        %background2=extPar.fixed.background(2);
        %background3=extPar.fixed.background(3);
        %background4=extPar.fixed.background(4);
        %background5=extPar.fixed.background(5);
        %background6=extPar.fixed.background(6);
        %background7=extPar.fixed.background(7);
        %background8=extPar.fixed.background(8);
        %background9=extPar.fixed.background(9);
        %background10=extPar.fixed.background(10);
        %background11=extPar.fixed.background(11);
        %background12=extPar.fixed.background(12);
        %background13=extPar.fixed.background(13);
        %background14=extPar.fixed.background(14);
        %background15=extPar.fixed.background(15);
      
        
        background1=X(8,i);
        background2=X(9,i);
        background3=X(10,i);
        background4=X(11,i);
        background5=X(12,i);
        background6=X(13,i);
       	background7=X(14,i);
        background8=X(15,i);
        background9=X(16,i);
        background10=X(17,i);
        background11=X(18,i);
        background12=X(19,i);
        background13=X(20,i);
        background14=X(21,i);
        background15=X(22,i);
       
     

	%ObsTime=X(36,i);

        %radius=extPar.fixed.radius;
        %mass=extPar.fixed.mass;
        %inclination=extPar.fixed.inclination;
        %emission=extPar.fixed.emission;
        %timeShift=extPar.fixed.phaseshift;
        
        %{
        background1=extPar.fixed.background(1);
        background2=extPar.fixed.background(2);
        background3=extPar.fixed.background(3);
        background4=extPar.fixed.background(4);
        background5=extPar.fixed.background(5);
        %}
        
% Call Mex function in order of fixed quantities
        %{
        cmd = '[Fspot(i),auxOutput{i}] = spotMex_trial(mass, radius, extPar.fixed.freq, inclination, emission, timeShift, extPar.obsdata2.numbins, extPar.modelchoice, rho, extPar.fixed.spot_temperature, extPar.fixed.distance, extPar.fixed.numtheta, extPar.spectral_model, extPar.fixed.numbands, extPar.fixed.E_band_lower_1, extPar.fixed.E_band_upper_1, extPar.fixed.beaming_model, extPar.spots_2, extPar.obsdata2.t';
        for j = 1:extPar.fixed.numbands
            cmd = [cmd,', extPar.obsdata2.f(',num2str(j),',:), extPar.obsdata2.err(',num2str(j),',:)'];
        end
        cmd = [cmd,');'];
        disp(cmd);
        eval(cmd);
        %}
        
        cmd = '[Fspot(i),auxOutput{i}] = spotMex_trial(mass, radius, extPar.fixed.freq, inclination, emission, timeShift, extPar.fixed.numbins, extPar.fixed.modelchoice, rho, temperature, extPar.fixed.distance, extPar.fixed.numtheta, extPar.fixed.spectral_model, extPar.fixed.numbands, extPar.fixed.E_band_lower_1, extPar.fixed.E_band_upper_1, extPar.fixed.beaming_model, extPar.fixed.spots_2, extPar.obsdata2.t, extPar.fixed.bend_file_is, extPar.fixed.mr, extPar.fixed.b, extPar.fixed.psi, extPar.fixed.dcosa, extPar.fixed.toa, extPar.fixed.spotshape, ObsTime, extPar.fixed.inst_curve, extPar.fixed.attenuation';
        for j = 1:extPar.fixed.numbands
            cmd = [cmd,', extPar.obsdata2.f(',num2str(j),',:), extPar.obsdata2.err(',num2str(j),',:), background',num2str(j),''];
        end
        cmd = [cmd,');'];
        disp(cmd)
        disp(i);
        %disp(timeShift);
        eval(cmd);
        disp(Fspot(i));
        time=toc
    
    else
        Fspot(i)=Inf;
        auxOutput{i}=[];
        % disp('not physical');
        % disp('');
    end
    
    pause(0.001);
end

function isPhysical=applyConstraints(X, extPar)

parameterNames=extPar.QubistPar.general.XLabels;
m=X(strcmpi(parameterNames,'mass (M_{sun})'));  % the label here needs to match the one in FerretSetup
r=X(strcmpi(parameterNames,'radius (km)'));     % the label here needs to match the one in FerretSetup
t=X(strcmpi(parameterNames,'temperature'));
%mr_ratio=extPar.fixed.mass/extPar.fixed.radius;
%mr_ratio=m/r;
mr_ratio=1.477*m/r;
tsquare=t*t;
%tsquare constraint comes from ML2015
thing=12.0/(5.0*(1.0-2.0*mr_ratio));
%isPhysical=mr_ratio > 0.0677 && mr_ratio < 0.203;
%isPhysical=mr_ratio > 0.1 && mr_ratio < 0.284 && tsquare<thing+0.1 && tsquare > thing-0.1;
isPhysical=mr_ratio > 0.1 && mr_ratio < 0.284;
%isPhysical=mr_ratio > 0.0677 && mr_ratio < 0.175;
