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

rem A folder of extracted .ymap files. Must be real files on disk: this
rem cannot read them out of the install's archives.
set "YMAP_SRC=D:\gtautil-2.2.13\levels\gta5\_citye\downtown_01\downtown_01_metadata.rpf"

rem Extracted .ydr drawables, and where the converted glTF goes.
set "YDR_SRC=D:\gtautil-2.2.13\levels\gta5\_citye\downtown_01"
set "MODEL_SRC=D:\gta5_export\models"

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
    echo [1/4] Building GTAUtil cache. Slow, and only happens once.
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
    echo [1/4] Cache already built, skipping.
)

rem --- 2. placements: ymap -> XML, written beside each input ------------
echo [2/4] Converting ymaps to XML...
"%GTAUTIL%" exportmeta -i "%YMAP_SRC%\*.ymap"

set /a XMLCOUNT=0
for %%F in ("%YMAP_SRC%\*.ymap.xml") do set /a XMLCOUNT+=1
if !XMLCOUNT!==0 (
    echo.
    echo ERROR: no .ymap.xml files appeared in "%YMAP_SRC%".
    echo        exportmeta matched nothing -- check the path and that it
    echo        contains loose .ymap files.
    exit /b 1
)
echo       !XMLCOUNT! ymap XML files written.

rem --- 3. pair with meshes and tile ------------------------------------
rem --- 3. meshes: ydr -> glTF -------------------------------------------
echo [3/4] Converting drawables to glTF...
python "%REPO%\packages\gta5\tools\ydr_to_gltf.py" ^
    --in "%YDR_SRC%" --out "%MODEL_SRC%"
if errorlevel 1 (
    echo ERROR: drawable conversion failed.
    exit /b 1
)

echo [4/4] Building tiles...
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
