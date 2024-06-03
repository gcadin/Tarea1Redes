Compilación

Opción 1) Ejecutando el comando make, se ejecutará las instrucciones 
señaladas en el archivo Makefile que se encuentra en esta carpeta

Opcion 2) g++ cliente.cpp -o cliente

Utilización:
PUna vez ejecutado el Servidor se pueden conectar los clientes, el siguiente
programa recibe dos parametros, siendo el primero la direccion ip del servidor y
el puerto a través del cual quiere realizar la conexión

Modo de Uso:

./cliente <ip_servidor> <puerto>

Ejemplo de ejecución:

./cliente 127.0.0.1 7777