$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$build = Join-Path $root "build"
$project = Join-Path $root "src\RenombrarArchivos.App\RenombrarArchivos.App.csproj"
$publish = Join-Path $root "publish"

Write-Host "== Compilando motor C++ ==" -ForegroundColor Cyan
cmake -S $root -B $build -A x64
if ($LASTEXITCODE -ne 0) { throw "Falló la configuración de CMake." }

cmake --build $build --config Release
if ($LASTEXITCODE -ne 0) { throw "Falló la compilación C++." }

Write-Host "== Publicando aplicación C# ==" -ForegroundColor Cyan
dotnet publish $project -c Release -r win-x64 --self-contained false -o $publish
if ($LASTEXITCODE -ne 0) { throw "Falló la publicación de .NET." }

$dllCandidates = @(
    (Join-Path $build "Release\RenombrarArchivos.Core.dll"),
    (Join-Path $build "RenombrarArchivos.Core.dll")
)
$nativeDll = $dllCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $nativeDll) {
    throw "No se encontró RenombrarArchivos.Core.dll en la carpeta de compilación."
}
Copy-Item -LiteralPath $nativeDll -Destination (Join-Path $publish "RenombrarArchivos.Core.dll") -Force

Write-Host ""
Write-Host "Compilación terminada. Ejecutable:" -ForegroundColor Green
Write-Host (Join-Path $publish "RenombrarArchivos.exe")
Write-Host "La DLL nativa C++ está en la misma carpeta."
