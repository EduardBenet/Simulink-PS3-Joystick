classdef PS3 < realtime.internal.SourceSampleTime & ...
        coder.ExternalDependency
    %
    % System object template for a source block.
    % 
    % This template includes most, but not all, possible properties,
    % attributes, and methods that you can implement for a System object in
    % Simulink.
    %
    % NOTE: When renaming the class name Source, the file name and
    % constructor name must be updated to use the class name.
    %
    
    % Copyright 2016-2018 The MathWorks, Inc.
    %#codegen
    %#ok<*EMCA>

    properties(Hidden, Access=private)
        fd
        joystick_state
    end
    
    methods
        % Constructor
        function obj = PS3(varargin)
            %This would allow the code generation to proceed with the
            %p-files in the installed location of the support package.
            coder.allowpcode('plain');

            % Support name-value pair arguments when constructing the object.
            setProperties(obj,nargin,varargin{:});
        end
    end
    
    methods (Access=protected)
        function setupImpl(obj)
            if isempty(coder.target)
                % Place simulation setup code here
            else
                % Call C-function implementing device initialization
                % coder.cinclude('source.h');
                coder.cinclude('ps3joystick.h');
                % coder.ceval('source_init');
                obj.fd = int32(0);
                obj.fd = coder.ceval('joystickSetup');
                
                j_state = struct(...
                    'buttons', zeros(1, 17, 'uint8'), ...
                    'axis', zeros(1,6, 'int16'));

                coder.cstructname(j_state, 'joystick_state', 'extern','HeaderFile','ps3joystick.h');
                obj.joystick_state = j_state;
            end
        end
        
        function [buttons, axis] = stepImpl(obj)
            buttons = zeros(1,17, 'uint8');
            axis = zeros(1,6, 'int16');

            if isempty(coder.target)
                % Place simulation output code here
            else
                % Call C-function implementing device output
                coder.cinclude('ps3joystick.h');

                % Get the current state and pass it by reference
                j_state = obj.joystick_state;
                coder.cstructname(j_state, 'joystick_state', 'extern','HeaderFile','ps3joystick.h');
                coder.ceval('readJoystick',obj.fd, coder.ref(j_state));

                % Store the state back
                obj.joystick_state = j_state;

                buttons = j_state.buttons;
                axis = j_state.axis;
            end
        end
        
        function releaseImpl(obj)
            if isempty(coder.target)
                % Place simulation termination code here
            else
                % Call C-function implementing device termination
                coder.cinclude('ps3joystick.h');
                coder.ceval('joystickTerminate',obj.fd);
            end
        end
    end
    
    methods (Access=protected)
        %% Define output properties
        function num = getNumInputsImpl(~)
            num = 0;
        end
        
        function num = getNumOutputsImpl(~)
            num = 2;
        end
        
        function varargout = isOutputFixedSizeImpl(~,~)
            varargout{1} = true;
            varargout{2} = true;
        end
        
        
        function varargout = isOutputComplexImpl(~)
            varargout{1} = false;
            varargout{2} = false;
        end
        
        function varargout = getOutputSizeImpl(~)
            varargout{1} = [1,17];
            varargout{2} = [1,6];
        end
        
        function varargout = getOutputDataTypeImpl(~)
            varargout{1} = 'uint8';
            varargout{2} = 'int16';
        end
        
        function icon = getIconImpl(~)
            % Define a string as the icon for the System block in Simulink.
            icon = 'PS3';
        end    
    end
    
    methods (Static, Access=protected)
        function simMode = getSimulateUsingImpl(~)
            simMode = 'Interpreted execution';
        end
        
        function isVisible = showSimulateUsingImpl
            isVisible = false;
        end
    end
    
    methods (Static)
        function name = getDescriptiveName()
            name = 'PS3';
        end
        
        function b = isSupportedContext(context)
            b = context.isCodeGenTarget('rtw');
        end
        
        function updateBuildInfo(buildInfo, context)
            if context.isCodeGenTarget('rtw')
                % Update buildInfo
                srcDir = fullfile(fileparts(mfilename('fullpath')),'src');
                includeDir = fullfile(fileparts(mfilename('fullpath')),'include');
                addIncludePaths(buildInfo,includeDir);
                % Use the following API's to add include files, sources and
                % linker flags
                buildInfo.addSourceFiles('ps3joystick.c', srcDir);
            end
        end
    end
end
