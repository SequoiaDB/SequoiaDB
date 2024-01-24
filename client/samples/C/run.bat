@echo off&setlocal enabledelayedexpansion

echo "running program..."
for %%a in (connect index insert query sampledb snap sql subArrayLen update update_use_id upsert lob) do (
   echo "***************running %%a..."
   build\%%a.exe 192.168.20.42 11810 "" ""
   echo "###############running %%astatic..."
   build\%%astatic.exe 192.168.20.42 11810 "" ""
)
