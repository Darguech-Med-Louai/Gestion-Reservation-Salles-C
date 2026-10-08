@echo off
setlocal

set "MSYS2=C:\msys64"
set "GCC=%MSYS2%\ucrt64\bin\gcc.exe"
set "WINDRES=%MSYS2%\ucrt64\bin\windres.exe"
set "PATH=%MSYS2%\ucrt64\bin;%PATH%"

if not exist "%GCC%" (
    echo GCC UCRT64 introuvable : %GCC%
    exit /b 1
)

if not exist "%~dp0build" mkdir "%~dp0build"
pushd "%~dp0"

if /I "%~1"=="console" goto console
if /I "%~1"=="gui" goto gui
if /I "%~1"=="test" goto test

echo Utilisation : build.bat console ^| gui ^| test
popd
exit /b 2

:console
"%GCC%" -std=c11 -Wall -Wextra -Isrc src\main.c src\Menu.c src\Facture.c src\FonctionsAux.c src\GestionSalle.c src\Reservation.c src\Stats.c src\Interface.c src\core\Core.c src\core\Persistence.c -o build\console.exe
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" echo Echec de la compilation console.
popd
exit /b %RESULT%

:gui
if not exist "%WINDRES%" (
    echo windres UCRT64 introuvable : %WINDRES%
    popd
    exit /b 1
)
"%WINDRES%" gui\assets\brand\orbite.rc -O coff -o build\orbite_icon.o
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" (
    echo Echec de la compilation de l'icone Windows.
    popd
    exit /b %RESULT%
)
"%GCC%" -std=c11 -Wall -Wextra -Isrc -Igui gui\main.c gui\ui_widgets.c src\core\Core.c src\core\Persistence.c src\FonctionsAux.c build\orbite_icon.o -o executer_orbite.exe -lraylib -lopengl32 -lgdi32 -lwinmm
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" echo Echec de la compilation graphique. Verifiez que raylib UCRT64 est installe.
if not "%RESULT%"=="0" goto gui_done
if not exist "libraylib.dll" copy /Y "%MSYS2%\ucrt64\bin\libraylib.dll" "libraylib.dll" >nul
if not exist "libraylib.dll" (
    echo Impossible de copier libraylib.dll.
    set "RESULT=1"
    goto gui_done
)
if not exist "glfw3.dll" copy /Y "%MSYS2%\ucrt64\bin\glfw3.dll" "glfw3.dll" >nul
if not exist "glfw3.dll" (
    echo Impossible de copier glfw3.dll.
    set "RESULT=1"
)
:gui_done
popd
exit /b %RESULT%

:test
"%GCC%" -std=c11 -Wall -Wextra -Isrc tests\test_core.c src\core\Core.c src\core\Persistence.c src\FonctionsAux.c -lm -o build\test_core.exe
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" (
    echo Echec de la compilation des tests.
    popd
    exit /b %RESULT%
)
if not exist "build\testdata\data" mkdir "build\testdata\data"
pushd "build\testdata"
..\test_core.exe
set "RESULT=%ERRORLEVEL%"
popd
popd
exit /b %RESULT%
