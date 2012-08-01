MKDIR %Tmp%\webrtc4ie
XCOPY . %Tmp%\webrtc4ie /S /E /Y
MKDIR %Tmp%\webrtc4ie\vcredist_x86
XCOPY %Tmp%\webrtc4ie\vcredi~1.exe %Tmp%\webrtc4ie\vcredist_x86 /Y
RENAME %Tmp%\webrtc4ie\vcredist_x86\vcredi~1.exe vcredist_x86.exe
%Tmp%\webrtc4ie\setup.exe