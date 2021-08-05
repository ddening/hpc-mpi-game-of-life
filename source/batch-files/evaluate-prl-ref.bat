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
FOR %%p IN (96 98 100 102 104 106 108 110 112 114 116 118 120 122 124 126 128) DO (
	FOR %%s IN (1000) DO (
		mpiexec.exe -n %%p parallel-reference.exe %%s %%s %max_gen% density-60-2400x2400.lif prl-ref.lif %result%
	)
)

echo Finished!