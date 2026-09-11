@echo off
setlocal enabledelayedexpansion
rem ===================================================================
rem  Export GTA V map placements into the form the gta5 package loads.
rem
rem  You run this against your own extracted game files. Nothing here
rem  decrypts anything.
rem
rem  Verified against GTAUtil 2.2.12. Three things about exportmeta that
rem  are not obvious and cost a while to work out:
rem
rem    * -i takes real FILES, and accepts a wildcard. An install folder
rem      or a bare ymap name silently matches nothing.
rem    * it writes <name>.ymap.xml NEXT TO each input. -o does not
rem      redirect that, so the XML stays with the ymaps.
rem    * it needs the cache built first, and asks "GTAV folder :" once.
rem      That prompt is why GTAUtil.exe --help appears to hang.
rem
rem  Meshes no longer need CodeWalker: ydr_to_gltf.py reads the .ydr
rem  resources directly.
rem ===================================================================

rem --- edit these ------------------------------------------------------
set "GTAUTIL=D:\gtautil-2.2.13\GTAUtil.exe"

rem The extracted map. Every tool below walks it recursively, so this is
rem the whole of Los Santos and Blaine County: 84 folders of ymaps and
rem about 62,000 drawables. Point it at one district, such as
rem levels\gta5\_citye\downtown_01, for a quick look.
set "MAP_SRC=D:\gtautil-2.2.13\levels\gta5"
set "YMAP_SRC=%MAP_SRC%"
set "YDR_SRC=%MAP_SRC%"

rem Where the converted glTF and PNG go.
set "TEXTURE_SRC=D:\gta5_export\textures"
set "MODEL_SRC=D:\gta5_export\models"

rem Vehicles, and the shared wheel pack. A vehicle's .yft has no wheels
rem in it -- only a brake-disc hub per corner -- because the game
rem instances a wheel model from wheels_mods.rpf at each axle.
set "VEHICLE_SRC=D:\gtautil-2.2.13\levels\gta5\vehicles.rpf"
set "WHEEL_MODEL=D:\gtautil-2.2.13\levels\gta5\vehiclemods\wheels_mods.rpf\wheel_spt_01.ydr"

rem Answer to the "GTAV folder :" prompt. Must be a LEGACY install:
rem GTAUtil predates the Enhanced edition and identifies a game folder by
rem GTA5.exe, which Enhanced does not ship (it has GTA5_Enhanced.exe).
set "GTAV_DIR=D:\SteamLibrary\steamapps\common\Grand Theft Auto V"
rem --------------------------------------------------------------------

set "REPO=%~dp0..\..\.."
for %%D in ("%GTAUTIL%") do set "GTAUTIL_DIR=%%~dpD"
set "CACHE=%GTAUTIL_DIR%cache.json"

echo.
echo === GTA V map export ===
echo   GTAUtil    : %GTAUTIL%
echo   ymap source: %YMAP_SRC%
echo   models     : %MODEL_SRC%
echo.

if not exist "%GTAUTIL%" (
    echo ERROR: GTAUtil not found at "%GTAUTIL%".
    exit /b 1
)
if not exist "%YMAP_SRC%" (
    echo ERROR: ymap source "%YMAP_SRC%" does not exist.
    exit /b 1
)

rem --- 1. cache --------------------------------------------------------
if not exist "%CACHE%" (
    echo [1/5] Building GTAUtil cache. Slow, and only happens once.
    echo.
    echo       At the "GTAV folder :" prompt, paste this and press enter:
    echo           %GTAV_DIR%
    echo.
    "%GTAUTIL%" buildcache
    if not exist "%CACHE%" (
        echo ERROR: no cache.json was produced; nothing downstream works.
        exit /b 1
    )
) else (
    echo [1/5] Cache already built, skipping.
)

rem --- 2. placements: ymap -> XML, written beside each input ------------
rem exportmeta takes one folder's wildcard at a time, so walk every
rem folder that holds a ymap. About 14 s each once the cache exists.
echo [2/5] Converting ymaps to XML...
for /f "delims=" %%D in ('dir /s /b /a:-d "%YMAP_SRC%\*.ymap" ^| findstr /v /i "\.xml$"') do (
    if not "%%~dpD"=="!LASTDIR!" (
        set "LASTDIR=%%~dpD"
        echo       %%~dpD
        echo.| "%GTAUTIL%" exportmeta -i "%%~dpD*.ymap" >nul
    )
)

set /a XMLCOUNT=0
for /r "%YMAP_SRC%" %%F in (*.ymap.xml) do set /a XMLCOUNT+=1
if !XMLCOUNT!==0 (
    echo.
    echo ERROR: no .ymap.xml files appeared under "%YMAP_SRC%".
    echo        exportmeta matched nothing -- check the path and that it
    echo        contains loose .ymap files.
    exit /b 1
)
echo       !XMLCOUNT! ymap XML files written.

rem --- 3. pair with meshes and tile ------------------------------------
rem --- 3. meshes: ydr -> glTF -------------------------------------------
echo [3/5] Converting drawables to glTF...
python "%REPO%\packages\gta5\tools\ydr_to_gltf.py" ^
    --in "%YDR_SRC%" --out "%MODEL_SRC%"
if errorlevel 1 (
    echo ERROR: drawable conversion failed.
    exit /b 1
)

echo [4/5] Converting vehicles to glTF...
python "%REPO%\packages\gta5\tools\ydr_to_gltf.py" ^
    --in "%VEHICLE_SRC%" --out "%MODEL_SRC%" ^
    --wheel-model "%WHEEL_MODEL%"
if errorlevel 1 (
    echo ERROR: vehicle conversion failed.
    exit /b 1
)

echo [5/5] Building tiles...
python "%REPO%\packages\gta5\tools\import_codewalker_export.py" ^
    --ymap-dir "%YMAP_SRC%" ^
    --model-dir "%MODEL_SRC%" ^
    --out "%REPO%\packages\gta5\assets\tiles"
if errorlevel 1 (
    echo ERROR: tile import failed.
    exit /b 1
)

echo.
echo Done. Run the map with:
echo   python python\dev_commands.py run --game gta5
endlocal
