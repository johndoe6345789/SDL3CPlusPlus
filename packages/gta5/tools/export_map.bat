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
rem GTAUtil scans the game install on first use and can sit silent for
rem minutes before it prints anything. That is normal; let it finish.
rem
rem NOTE: these flags are the documented shape of exportmeta but were not
rem verified here, because GTAUtil would not return --help until its
rem cache had been built. If it rejects them, run `GTAUtil.exe exportmeta
rem --help` once the cache exists and correct this one line. The XML
rem count check below is what stops a wrong flag passing silently.
echo [1/2] Converting ymaps to XML...
pushd "%WORK%"
"%GTAUTIL%" exportmeta --input "%YMAP_SRC%" --output "%XML_OUT%"
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
