@echo off
REM Installe CTS2000.vst3 dans le dossier VST3 standard (clic droit > Executer en tant qu'administrateur)
set DEST=%CommonProgramFiles%\VST3\CTS2000.vst3
xcopy /E /I /Y "%~dp0CTS2000.vst3" "%DEST%"
if errorlevel 1 (echo ECHEC : relance ce fichier en administrateur.) else (echo Installe dans %DEST% & echo Ouvre FL Studio : Options ^> Manage plugins ^> Find installed plugins.)
pause
