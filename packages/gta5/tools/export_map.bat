@echo off
setlocal enabledelayedexpansion
rem ===================================================================
rem  Export the GTA V map into the form the gta5 package loads.
rem
rem  You run this against your own extracted game files. Nothing here
rem  decrypts anything.
rem
rem  Verified against GTAUtil 2.2.12. Four things about exportmeta that
rem  are not obvious and cost a while to work out:
rem
rem    * -i takes ONE input: a single file, or a single wildcard. Name
rem      two files and only the first is converted, silently.
rem    * -i wants real FILES. An install folder or a bare ymap name
rem      matches nothing, also silently.
rem    * it writes <name>.ymap.xml NEXT TO each input; -o does not
rem      redirect that.
rem    * it needs the cache built first, and asks "GTAV folder :" once.
rem      That prompt is why GTAUtil.exe --help appears to hang.
rem
rem  Everything except the ymap step is the package's own Python, which
rem  reads the RSC7 resources directly.
rem ===================================================================

rem --- edit these ------------------------------------------------------
set "GTAUTIL=D:\gtautil-2.2.13\GTAUtil.exe"

rem The extracted map. Every tool below walks it recursively, so this is
rem the whole of Los Santos and Blaine County: about 3,900 useful ymaps,
rem 62,000 drawables and 19,000 texture dictionaries. Point it at one
rem district, such as levels\gta5\_citye\downtown_01, for a quick look.
set "MAP_SRC=D:\gtautil-2.2.13\levels\gta5"

rem Where everything converted goes.
set "EXPORT=D:\gta5_export"
set "YMAP_STAGE=%EXPORT%\ymaps"
set "TEXTURE_SRC=%EXPORT%\textures"
set "MODEL_SRC=%EXPORT%\models"
set "VEHICLE_STAGE=%EXPORT%\vehicle_src"

rem The taxi the package spawns, and its paint. The _hi model is the
rem detailed one; it is converted under the plain name the workflow
rem loads. GTA V paint colours live in carcols.ymt, which is not in the
rem extract, hence the explicit colour.
set "TAXI_SRC=%MAP_SRC%\vehicles.rpf\taxi_hi.yft"
set "TAXI_PAINT=1.0,0.72,0.05"

rem A vehicle's .yft has no wheels in it -- only a brake-disc hub per
rem corner -- because the game instances one from the wheel pack.
set "WHEEL_MODEL=%MAP_SRC%\vehiclemods\wheels_mods.rpf\wheel_spt_01.ydr"

rem Answer to the "GTAV folder :" prompt. Must be a LEGACY install:
rem GTAUtil predates the Enhanced edition and identifies a game folder by
rem GTA5.exe, which Enhanced does not ship (it has GTA5_Enhanced.exe).
set "GTAV_DIR=D:\SteamLibrary\steamapps\common\Grand Theft Auto V"
rem --------------------------------------------------------------------

set "REPO=%~dp0..\..\.."
set "TOOLS=%REPO%\packages\gta5\tools"
for %%D in ("%GTAUTIL%") do set "GTAUTIL_DIR=%%~dpD"
set "CACHE=%GTAUTIL_DIR%cache.json"

echo.
echo === GTA V map export ===
echo   GTAUtil : %GTAUTIL%
echo   map     : %MAP_SRC%
echo   output  : %EXPORT%
echo.

if not exist "%GTAUTIL%" (
    echo ERROR: GTAUtil not found at "%GTAUTIL%".
    exit /b 1
)
if not exist "%MAP_SRC%" (
    echo ERROR: map source "%MAP_SRC%" does not exist.
    exit /b 1
)

rem --- 1. cache --------------------------------------------------------
if not exist "%CACHE%" (
    echo [1/6] Building GTAUtil cache. Slow, and only happens once.
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
    echo [1/6] Cache already built, skipping.
)

rem --- 2. placements: ymap -> XML --------------------------------------
rem Since -i takes one wildcard, the ymaps worth converting are COPIED
rem into one staging folder -- your extract is not touched -- and done in
rem a single call, which loads the cache once. Left out: grass, which is
rem 315 files of ~40 MB XML at ~30 s each and places no buildings or
rem roads; and LOD lights and occlusion, which place nothing drawable.
rem ymap names are unique -- the game streams them by name -- so the
rem copy can be flat.
echo [2/6] Staging ymaps and converting them to XML...
if not exist "%YMAP_STAGE%" mkdir "%YMAP_STAGE%"
for /r "%MAP_SRC%" %%F in (*.ymap) do (
    set "KEEP=1"
    echo %%~nxF| findstr /i "_grass_" >nul && set "KEEP="
    echo %%~dpF| findstr /i "lodlights _occl" >nul && set "KEEP="
    if defined KEEP copy /y "%%F" "%YMAP_STAGE%\" >nul
)
echo.| "%GTAUTIL%" exportmeta -i "%YMAP_STAGE%\*.ymap" >nul

set /a XMLCOUNT=0
for %%F in ("%YMAP_STAGE%\*.ymap.xml") do set /a XMLCOUNT+=1
if !XMLCOUNT!==0 (
    echo.
    echo ERROR: no .ymap.xml files appeared in "%YMAP_STAGE%".
    echo        Check that "%MAP_SRC%" holds loose .ymap files.
    exit /b 1
)
echo       !XMLCOUNT! ymap XML files written.

rem --- 3. textures: ytd -> PNG ------------------------------------------
echo [3/6] Extracting textures...
python "%TOOLS%\ytd_to_png.py" --in "%MAP_SRC%" --out "%TEXTURE_SRC%" ^
    --max-size 512
if errorlevel 1 (
    echo ERROR: texture extraction failed.
    exit /b 1
)

rem --- 4. meshes: ydr/ydd -> glTF ---------------------------------------
rem --ymap-dir names the entries of each .ydd, which store only a hash;
rem without it every .ydd drawable is skipped. --texture-dir is what
rem makes the meshes textured at all.
echo [4/6] Converting drawables to glTF...
python "%TOOLS%\ydr_to_gltf.py" --in "%MAP_SRC%" --out "%MODEL_SRC%" ^
    --texture-dir "%TEXTURE_SRC%" --ymap-dir "%YMAP_STAGE%"
if errorlevel 1 (
    echo ERROR: drawable conversion failed.
    exit /b 1
)

rem --- 5. the taxi --------------------------------------------------------
echo [5/6] Converting the taxi...
if not exist "%VEHICLE_STAGE%" mkdir "%VEHICLE_STAGE%"
copy /y "%TAXI_SRC%" "%VEHICLE_STAGE%\taxi.yft" >nul
python "%TOOLS%\ydr_to_gltf.py" --in "%VEHICLE_STAGE%" ^
    --out "%MODEL_SRC%" --texture-dir "%TEXTURE_SRC%" ^
    --wheel-model "%WHEEL_MODEL%" --paint "%TAXI_PAINT%"
if errorlevel 1 (
    echo ERROR: vehicle conversion failed.
    exit /b 1
)

rem --- 6. tiles -----------------------------------------------------------
echo [6/6] Building tiles...
python "%TOOLS%\import_codewalker_export.py" ^
    --ymap-dir "%YMAP_STAGE%" ^
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
