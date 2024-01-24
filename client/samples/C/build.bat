@echo off&setlocal enabledelayedexpansion

echo "building program..."
for %%a in (connect index insert query sampledb snap sql subArrayLen update update_use_id upsert) do (
   buildApp.bat %%a
)
