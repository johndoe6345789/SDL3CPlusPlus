@echo off
setlocal enabledelayedexpansion
rem ===================================================================
rem  Export GTA V map data into the form the gta5 package loads.
rem
rem  You run this. It reads your own extracted game files and converts
rem  them; nothing here decrypts anything.
rem
rem  Two halves, and only the first is automatable:
rem
rem    placements  ymap -> XML       GTAUtil exportmeta   (this script)
rem    meshes      ydr/ydd -> glTF   CodeWalker           (by hand)
rem
rem  GTAUtil has no drawable export -- see its command list -- so the
rem  meshes have to come from CodeWalker. Run this for the placements,
rem  export the drawables yourself, then let the last step pair them up.
rem ===================================================================

rem --- edit these four ------------------------------------------------
set "GTAUTIL=D:\gtautil-2.2.13\GTAUtil.exe"
set "YMAP_SRC=D:\gtautil-2.2.13\levels\gta5\_citye\downtown_01"
set "MODEL_SRC=D:\gta5_export\models"
set "WORK=D:\gta5_export"
rem Must be a LEGACY install: GTAUtil predates the Enhanced edition and
rem checks for GTA5.exe, which Enhanced does not ship (it has
rem GTA5_Enhanced.exe instead). Point at Enhanced and it rejects the
rem path and asks again forever.
set "GTAV_DIR=D:\SteamLibrary\steamapps\common\Grand Theft Auto V"
rem --------------------------------------------------------------------

set "REPO=%~dp0..\..\.."
set "XML_OUT=%WORK%\ymaps"

echo.
echo === GTA V map export ===
echo   GTAUtil    : %GTAUTIL%
echo   ymap source: %YMAP_SRC%
echo   models     : %MODEL_SRC%
echo   output     : %WORK%
echo.

if not exist "%GTAUTIL%" (
    echo ERROR: GTAUtil not found at "%GTAUTIL%".
    exit /b 1
)
if not exist "%YMAP_SRC%" (
    echo ERROR: ymap source "%YMAP_SRC%" does not exist.
    echo        Point YMAP_SRC at an extracted levels\gta5\... folder.
    exit /b 1
)

if not exist "%XML_OUT%" mkdir "%XML_OUT%"

rem --- 1. placements: ymap -> XML --------------------------------------
rem On its first run GTAUtil prompts "GTAV folder :" and waits. Type the
rem path to your GTA V install and press enter; it remembers afterwards.
rem Do not pipe anything into this script or that prompt cannot be
rem answered. It then scans the install and can sit silent for minutes,
rem which is normal.
rem
rem The XML count check below stays regardless: it turns a run that
rem produced nothing into a loud failure rather than an empty map.
echo [1/2] Converting ymaps to XML...
echo.
echo   If GTAUtil asks "GTAV folder :" it is waiting for input --
echo   it has already been given -i and -o. Paste this and press enter:
echo.
echo       %GTAV_DIR%
echo.
echo   Enhanced will NOT be accepted -- GTAUtil wants a Legacy install
echo   with GTA5.exe in it.
echo.
echo   It only asks once, then scans the install, which is slow and
echo   silent. That is normal.
echo.
pushd "%WORK%"
"%GTAUTIL%" exportmeta -i "%YMAP_SRC%" -o "%XML_OUT%"
popd

set /a XMLCOUNT=0
for %%F in ("%XML_OUT%\*.xml") do set /a XMLCOUNT+=1
if %XMLCOUNT%==0 (
    echo.
    echo ERROR: no .xml files landed in "%XML_OUT%".
    echo        The exportmeta call above did not do what this script
    echo        expects -- check its flags before going further.
    exit /b 1
)
echo       %XMLCOUNT% ymap XML files written.

rem --- 2. pair placements with meshes and tile them --------------------
if not exist "%MODEL_SRC%" (
    echo.
    echo NOTE: "%MODEL_SRC%" does not exist yet, so every placement will
    echo       be written with model=null and skipped at load. Export the
    echo       drawables from CodeWalker as glTF/OBJ/FBX into that folder,
    echo       named after their archetype, then re-run this script.
)

echo [2/2] Building tiles...
python "%REPO%\packages\gta5\tools\import_codewalker_export.py" ^
    --ymap-dir "%XML_OUT%" ^
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
