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
FOR %%p IN (2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50 52 54 56 58 60 62 64 66 68 70 72 74 76 78 80 82 84 86 88 90 92 94 96 98 100 102 104 106 108 110 112 114 116 118 120 122 124 126 128) DO (
	FOR %%s IN (1000) DO (
		mpiexec.exe -n %%p parallel.exe %%s %%s %max_gen% density-60-2400x2400.lif odstrb_1.lif odstrb_2.lif 2 %result%
	)
)

echo Finished!