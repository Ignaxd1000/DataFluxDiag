# DataFluxDiag

DataFluxDiag es una herramienta de diagnóstico para línea de comandos escrita en C11. El proyecto implementa pruebas relacionadas con:

- Lectura y copia de archivos utilizando buffers administrados por el programa.
- Escaneo de puertos TCP mediante conexiones IPv4.
- Transferencia de archivos sobre TCP mediante un protocolo binario simple.

Actualmente, la implementación utiliza una capa POSIX para archivos, sockets y temporización.

## Estado actual

### Funcionalidades implementadas

- Benchmark de lectura de archivos con distintos tamaños de buffer.
- Benchmark de copia de archivos con distintos tamaños de buffer.
- Escaneo de puertos TCP IPv4.
- Conexiones TCP con medición de latencia.
- Detección de puertos abiertos, cerrados y con timeout.
- Transferencia de archivos entre cliente y servidor.
- Modo interactivo mediante un menú.
- Interfaz de línea de comandos.
- Pruebas offline para las funciones auxiliares y parsers.

### Funcionalidades no implementadas

- Soporte para Windows.
- UDP.
- IPv6.
- FTP.
- GUI.
- Raw sockets.
- Captura o creación de paquetes.
- Cifrado y autenticación.
- Transferencias reanudables.
- Servicios o daemons.
- Funcionalidades avanzadas similares a Nmap.

## Compatibilidad

- **Linux/POSIX:** implementado.
- **Windows:** no implementado actualmente.

El soporte para Windows está preparado como una futura capa de plataforma, pero los archivos correspondientes generan un error de compilación intencional:

```text
Windows backend is planned for a future release and is not implemented yet.
```

## Requisitos

- CMake 3.16 o superior.
- Compilador compatible con C11.
- Sistema operativo Linux/POSIX.

El proyecto utiliza:

- `-Wall`
- `-Wextra`
- `-Wpedantic`

en compiladores que no sean MSVC.

## Compilación

```bash
cmake -S . -B build
cmake --build build
```

El ejecutable generado se encuentra normalmente en:

```bash
./build/datafluxdiag
```

Las pruebas se compilan por defecto. Para deshabilitarlas:

```bash
cmake -S . -B build -DDATAFLUXDIAG_BUILD_TESTS=OFF
cmake --build build
```

## Uso

### Ayuda

```bash
./build/datafluxdiag --help
```

La ayuda incorporada muestra los comandos disponibles:

```text
streamdiag --help
streamdiag buffer benchmark <file> [--copy-out <file>] [--buffers <list>]
streamdiag scan <host> <start-end> [--timeout <ms>]
streamdiag server --port <port>
streamdiag send <host:port> <file>
```

> Nota: el nombre del ejecutable generado por CMake es `datafluxdiag`, aunque los mensajes de ayuda todavía utilizan el nombre `streamdiag`.

### Modo interactivo

Si el programa se ejecuta sin argumentos, muestra un menú interactivo:

```bash
./build/datafluxdiag
```

Menú disponible:

```text
========================================
             StreamDiag
========================================
1. Buffer Diagnostic
2. TCP Socket Scanner
3. TCP File Transfer
0. Exit
```

El modo interactivo permite:

1. Ejecutar un benchmark de buffers.
2. Escanear un rango de puertos TCP.
3. Iniciar un servidor o enviar un archivo.
4. Salir del programa.

## Diagnóstico de buffers

### Sintaxis

```bash
./build/datafluxdiag buffer benchmark <archivo> [opciones]
```

Opciones disponibles:

```text
--copy-out <archivo>
--buffers <lista_o_rango>
```

### Ejemplos

Benchmark de lectura usando el buffer predeterminado:

```bash
./build/datafluxdiag buffer benchmark data.bin
```

Benchmark con varios tamaños de buffer:

```bash
./build/datafluxdiag buffer benchmark data.bin \
  --buffers 512,1024,4096,65536
```

Benchmark utilizando un rango:

```bash
./build/datafluxdiag buffer benchmark data.bin \
  --buffers 1024-65536:1024
```

Benchmark de lectura y copia:

```bash
./build/datafluxdiag buffer benchmark input.bin \
  --copy-out output.bin \
  --buffers 4096,16384,65536
```

### Formato de tamaños de buffer

Lista de tamaños:

```text
512,1024,4096
```

Rango con paso:

```text
1024-65536:1024
```

Un rango sin paso utiliza como paso el valor inicial:

```text
1024-4096
```

El tamaño predeterminado es:

```text
65536 bytes
```

El tamaño máximo permitido es:

```text
67108864 bytes
```

equivalente a 64 MiB.

### Resultados

El benchmark muestra:

- Tamaño del archivo.
- Tipo de operación: `READ` o `COPY`.
- Tamaño del buffer.
- Cantidad de operaciones de lectura.
- Tiempo transcurrido en milisegundos.
- Rendimiento en MiB/s.

Ejemplo de formato de salida:

```text
BUFFER DIAGNOSTIC
----------------------------------------
File: data.bin
Size: 1048576 bytes
Operation: READ

Buffer       Operations     Time(ms)   MiB/s
65536        16              2.341      427.166
```

## Escáner de puertos TCP

### Sintaxis

```bash
./build/datafluxdiag scan <host> <inicio-fin> [--timeout <ms>]
```

### Ejemplo

```bash
./build/datafluxdiag scan 127.0.0.1 20-100 --timeout 750
```

El escáner:

- Resuelve el host a una dirección IPv4.
- Crea una tarea por puerto.
- Ejecuta las conexiones utilizando hilos C11.
- Intenta establecer conexiones TCP.
- Mide la latencia.
- Ordena los resultados por número de puerto.

Cada puerto puede aparecer con uno de estos estados:

- `OPEN`
- `CLOSED`
- `TIMEOUT`

La salida incluye la latencia en milisegundos cuando está disponible:

```text
SOCKET SCANNER
----------------------------------------
Host: 127.0.0.1
Protocol: TCP (IPv4)
Ports: 20-100
Timeout: 750 ms

PORT       STATE      LATENCY
20         CLOSED     0.231 ms
22         OPEN       0.487 ms
80         CLOSED     0.198 ms
```

> Utiliza el escáner únicamente contra hosts y redes que tengas autorización para analizar.

## Transferencia de archivos TCP

La transferencia utiliza un cliente y un servidor propios que se comunican mediante TCP IPv4.

### Iniciar el servidor

```bash
./build/datafluxdiag server --port 5000
```

El servidor:

1. Escucha en el puerto indicado.
2. Acepta una conexión de cliente.
3. Recibe y valida la cabecera.
4. Recibe el nombre del archivo.
5. Crea un archivo con el prefijo `received_`.
6. Recibe los datos en bloques de 65536 bytes.
7. Envía un ACK de un byte al cliente.

El servidor acepta una conexión por ejecución.

### Enviar un archivo

```bash
./build/datafluxdiag send 127.0.0.1:5000 archivo.bin
```

El cliente:

1. Abre el archivo local.
2. Obtiene su tamaño.
3. Se conecta al servidor.
4. Envía la cabecera y el nombre base del archivo.
5. Envía el contenido en bloques de 65536 bytes.
6. Muestra el progreso cada 10%.
7. Espera la confirmación del servidor.
8. Muestra latencia, bytes enviados, tiempo y rendimiento.

Ejemplo de salida:

```text
Progress: 10%
Progress: 20%
Progress: 30%
...
Transfer completed
Connection latency: 0.421 ms
Bytes sent: 1048576
Elapsed: 12.832 ms
Throughput: 77.929 MiB/s
```

## Protocolo de transferencia

El protocolo utiliza una cabecera binaria con los siguientes campos, enviados en orden de red:

| Campo | Tipo | Descripción |
|---|---:|---|
| `magic` | `u32` | Valor `SDTF` |
| `version` | `u16` | Versión del protocolo, actualmente `1` |
| `filename_length` | `u16` | Longitud del nombre del archivo |
| `file_size` | `u64` | Tamaño del archivo en bytes |

Después de la cabecera se envían:

1. Los bytes del nombre del archivo.
2. Los bytes del contenido del archivo.
3. Un ACK de un byte enviado por el servidor.

Valores del ACK:

```text
1 = transferencia exitosa
0 = transferencia fallida
```

El nombre del archivo está limitado a 1024 bytes. El servidor utiliza solamente el nombre base y guarda el archivo como:

```text
received_<nombre_base>
```

El servidor valida que la cantidad de bytes recibidos coincida con el tamaño indicado en la cabecera.

## Arquitectura

```text
include/
  buffer_diag.h
  cli.h
  scanner.h
  structs.h
  transfer.h
  utils.h
  platform/
    io.h
    platform.h
    socket.h

src/
  buffer_diag.c
  cli.c
  main.c
  scanner.c
  transfer.c
  utils.c
  platform/
    posix/
      io_posix.c
      platform_posix.c
      socket_posix.c
    windows/
      io_windows.c
      platform_windows.c
      socket_windows.c

tests/
  test_main.c
```

La lógica principal se encuentra separada de las implementaciones específicas de la plataforma:

- `buffer_diag.c`: benchmarks de lectura y copia.
- `scanner.c`: escaneo de puertos TCP.
- `transfer.c`: cliente y servidor de transferencia.
- `cli.c`: comandos y menú interactivo.
- `utils.c`: validaciones, parsers, throughput y checksum.
- `platform/posix/`: implementación POSIX para archivos, sockets y temporización.
- `platform/windows/`: placeholders para soporte futuro.

## Funciones auxiliares

El módulo de utilidades incluye:

- Validación de tamaños de buffer.
- Parseo de listas y rangos de buffers.
- Parseo de rangos de puertos.
- Parseo de endpoints con formato `host:port`.
- Cálculo de rendimiento en MiB/s.
- Cálculo de checksum FNV-1a de 32 bits.
- Extracción del nombre base de una ruta.
- Conversión de texto a enteros positivos.
- Ordenamiento de resultados de puertos.

## Pruebas

Para ejecutar las pruebas:

```bash
ctest --test-dir build --output-on-failure
```

Las pruebas cubren actualmente:

- Validación de tamaños de buffer.
- Parseo de listas de tamaños de buffer.
- Parseo de rangos de buffers.
- Parseo de rangos de puertos.
- Validación de rangos inválidos.
- Parseo de endpoints `host:port`.
- Cálculo de throughput.
- Estabilidad del checksum.

También se puede ejecutar directamente el binario de pruebas:

```bash
./build/datafluxdiag_tests
```

Salida esperada:

```text
All StreamDiag offline tests passed.
```

## Limitaciones actuales

- Solo existe una implementación POSIX.
- El escáner utiliza TCP sobre IPv4.
- La transferencia utiliza TCP sobre IPv4.
- El servidor procesa una conexión por ejecución.
- No se implementa autenticación ni cifrado.
- No se validan checksums durante la transferencia.
- No existe soporte para reanudar transferencias.
- El timeout efectivo del trabajador del escáner está fijado actualmente en 1000 ms.
- El nombre mostrado en varios mensajes de la CLI sigue siendo `StreamDiag`, mientras que el proyecto y el ejecutable se llaman `DataFluxDiag`.

## Posibles mejoras

- Implementar los backends para Windows.
- Añadir soporte para múltiples conexiones en el servidor.
- Implementar reanudación de transferencias.
- Mejorar la validación del protocolo.
- Agregar más pruebas para errores de red, archivos y argumentos.
- Añadir soporte opcional para escaneos con límites de concurrencia.

## Licencia

Consulta el archivo `LICENSE` incluido en el repositorio para conocer los términos de licencia del proyecto.
