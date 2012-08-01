MKDIR %Tmp%\webrtc4all
XCOPY . %Tmp%\webrtc4all /S /E /Y
MKDIR %Tmp%\webrtc4all\vcredist_x86
XCOPY %Tmp%\webrtc4all\vcredi~1.exe %Tmp%\webrtc4all\vcredist_x86 /Y
RENAME %Tmp%\webrtc4all\vcredist_x86\vcredi~1.exe vcredist_x86.exe
%Tmp%\webrtc4all\setup.exe