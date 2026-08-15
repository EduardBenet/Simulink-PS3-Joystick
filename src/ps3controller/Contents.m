% PS3Controller
% Version 1.0.0 15-Aug-2026
%
% Simulink device driver blocks for reading a PS3 controller paired over
% Bluetooth to a Raspberry Pi. The blocks read the Linux joystick device
% (/dev/input/js0) on the target and output the raw button and axis
% values, so that a model deployed to the Pi can be driven by the
% controller.
%
% Simulink libraries
%   PS3Controller           - Block library containing the PS3 Controller block
%   slblocks                - Register the library with the Simulink Library Browser
%
% System objects
%   PS3                     - Source System object reading button and axis values
%
% Target source code
%   src/ps3joystick.c       - Joystick open, read and close implementation
%   include/ps3joystick.h   - Joystick interface and joystick_state definition
%
% The PS3 block has no inputs and two outputs:
%   buttons - 1-by-17 uint8, current state of each button (0 or 1)
%   axis    - 1-by-6  int16, current position of each axis
%
% The block only generates code for the Raspberry Pi target; in normal
% simulation both outputs are zero.
%
% See also PS3, slblocks.

% Copyright 2022-2026 Eduard Benet Cerda
