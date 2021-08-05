REM # =========================
REM #
REM # Titel : HPC Game Of Life
REM # Author: Dimitri Dening
REM # Date  : 28.05.2021
REM #
REM # =========================

@echo off

echo Running perfomance evaluation ...
set max_gen=20000

setlocal EnableDelayedExpansion
set "string=ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
set "result="
for /L %%i in (1,1,15) do call :add
REM echo %result%
goto :eval

:add
set /a x=%random% %% 52 
set result=%result%!string:~%x%,1!
goto :eof

:eval
FOR %%s IN (200 400 600 800 1000) DO (
	mpiexec.exe -n 1 seriell.exe %%s %%s %max_gen% density-60-2400x2400.lif os1.lif os2.lif 2 %result%
)

echo Finished!