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
FOR %%p IN (2 4 8 16 32 48 64 80 96 112 128) DO (
	FOR %%s IN (1000) DO (
		mpiexec.exe -n %%p multi-node.exe %%s %%s %max_gen% density-60-2400x2400.lif multi-node.lif %result%
	)
)

echo Finished!