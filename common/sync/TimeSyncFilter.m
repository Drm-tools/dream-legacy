%******************************************************************************\
%* Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
%* Copyright (c) 2003
%*
%* Author:
%*	Alexander Kurpiers
%*
%* Description:
%* 	Hilbert Filter for timing acquisition
%*  Runs at 48 kHz, can be downsampled to 48 kHz / 8 = 6 kHz
%*
%******************************************************************************
%*
%* This program is free software; you can redistribute it and/or modify it under
%* the terms of the GNU General Public License as published by the Free Software
%* Foundation; either version 2 of the License, or (at your option) any later 
%* version.
%*
%* This program is distributed in the hope that it will be useful, but WITHOUT 
%* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
%* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
%* details.
%*
%* You should have received a copy of the GNU General Public License along with
%* this program; if not, write to the Free Software Foundation, Inc., 
%* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
%*
%******************************************************************************/

% Length of hilbert filter and characteristic frequencies
nhil = 81;
fstart = 800;
fstop = 5200;
ftrans = 800; % Size of transition region
f0 = 3000;

PLOT = 0;

fs = 48000;
B = fstop - fstart;

f = [0  B / 2  B / 2 + ftrans  fs / 2];
m = [2 2 0 0];

b = remez(nhil - 1, f * 2 / fs, m, [1 10]);

if (PLOT == 1)
    t = linspace(0, (nhil - 1) / fs, nhil);
    hr = b .* cos(2 * pi * f0 * t);
    hi = b .* sin(2 * pi * f0 * t);
    
    % Complex hilbert filter
    hbp = hr + hi * j;
    
    [h1,f]= freqz(hbp, 1, 512, 'whole', fs);
    plot(f - fs / 2, 20 * log10(abs([h1(257:512); h1(1:256)])));
    axis([-1000 6000 -90 10]);
    grid;
    zoom on;
    title('Hilbert-transformer');
    xlabel('Frequency [Hz]');
    ylabel('Attenuation [dB]');
end


% Export coefficiants to file ****************************************
fid = fopen('TimeSyncFilter.h', 'w');

fprintf(fid, '/* Automatically generated file with MATLAB */\n');
fprintf(fid, '/* File name: "TimeSyncFilter.m" */\n');
fprintf(fid, '/* Filter taps in time-domain */\n\n');

fprintf(fid, '#ifndef _TIMESYNCFILTER_H_\n');
fprintf(fid, '#define _TIMESYNCFILTER_H_\n\n');


fprintf(fid, '#define NO_TAPS_HILB_FILT             ');
fprintf(fid, int2str(nhil));
fprintf(fid, '\n');
fprintf(fid, '#define HILB_FILT_BNDWIDTH            ');
fprintf(fid, int2str(fstop - fstart + ftrans));
fprintf(fid, '\n\n\n');

% Write filter taps
fprintf(fid, '/* Low pass prototype for Hilbert-filter */\n');
fprintf(fid, 'static float fHilLPProt[NO_TAPS_HILB_FILT] =\n');
fprintf(fid, '{\n');
fprintf(fid, '	%.20ff,\n', b(1:end - 1));
fprintf(fid, '	%.20ff\n', b(end));
fprintf(fid, '};\n\n\n');

fprintf(fid, '#endif	/* _TIMESYNCFILTER_H_ */\n');
fclose(fid);