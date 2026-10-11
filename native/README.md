# Motor nativo C++

La biblioteca exporta `NormalizeNumberPrefix`, llamada desde C# mediante P/Invoke.

- Entrada: nombre sin extensión.
- Devuelve 1 si encuentra un prefijo numérico válido; devuelve 0 si no debe cambiarse.
- Normaliza el número a un mínimo de tres cifras sin usar el orden de la lista.
- No escribe en archivos ni toca el sistema de archivos: solo procesa texto.
- Conserva números largos sin convertirlos a un entero de tamaño fijo.
