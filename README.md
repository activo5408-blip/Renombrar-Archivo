# Renombrar archivos

Aplicación de escritorio para Windows construida con **C# (WinForms)** y un motor nativo **C++**.

## Qué hace
- Permite elegir una carpeta desde una ventana.
- Muestra una vista previa con nombre original, nombre propuesto y estado.
- Normaliza el prefijo numérico del propio nombre original a un mínimo de tres cifras: `1 - Lolo.mkv` → `001 - Lolo.mkv`, `8 - Lolo.mkv` → `008 - Lolo.mkv`, `25 - Lolo.mkv` → `025 - Lolo.mkv`.
- No usa la posición del archivo en la lista para decidir su número.
- Conserva el resto del nombre y la extensión.
- Los archivos sin número al principio se dejan intactos.
- Comprueba conflictos antes de cambiar nombres y nunca sobrescribe un destino existente.
- No borra archivos ni modifica su contenido. No cambia nada hasta que confirmes.

## Estructura
- `src/RenombrarArchivos.App/`: interfaz de Windows en C#.
- `native/`: motor C++ que interpreta el prefijo numérico.
- `CMakeLists.txt`: compilación de la biblioteca C++.

## Requisitos
- Windows 10/11 x64.
- .NET 8 SDK.
- Visual Studio Build Tools con compilador C++ y CMake.

## Compilar
Desde una terminal de desarrollador de Visual Studio, en la raíz del proyecto:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
 dotnet build .\src\RenombrarArchivos.App\RenombrarArchivos.App.csproj -c Release
```

Copia `build\Release\RenombrarArchivos.Core.dll` junto al ejecutable de la aplicación si el generador de CMake no lo deja allí automáticamente.

## Seguridad de los cambios
La aplicación presenta una vista previa y solicita confirmación. Si detecta nombres de destino duplicados o archivos que ya existen con el nombre propuesto, cancela la operación sin realizar cambios. El renombrado se realiza en dos fases para evitar choques entre nombres que se intercambian.
