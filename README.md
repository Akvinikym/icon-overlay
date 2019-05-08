## Installation

1. OverlayIcon.cpp line 40 - set the path to the file where you want to keep logs
2. Build the solution, strictly in x64 mode
3. Run regsvr32 on x64\Debug\OverlayIcon.dll (cmd should be run as an administrator)
4. User regedit to modify the register following the guide at https://docs.microsoft.com/ru-ru/windows/desktop/shell/how-to-register-icon-overlay-handlers. The GUID can be found
in OverlayIcon.rgs file
5. Restart the explorer after that
6. every file which contains "codeproject" substring should have a custom icon
7. Also explorer should be patched, and patched function should write to a file you specified in step 1.
8. If patching has failed, the dialog with "PATCH FAILED" message should pop up, otherwise "PATCHED" message should be displayed on explorer start.