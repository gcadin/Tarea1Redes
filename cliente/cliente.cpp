#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#define BUFFER_SIZE 1024

void imprimir_tablero(const std::string& tablero_str) {
    std::cout << tablero_str << std::endl;
}

int conectar_servidor(const char* direccion_ip, int puerto) {
    int socket_cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_cliente < 0) {
        std::cerr << "Error al crear el socket\n";
        return -1;
    }

    struct sockaddr_in direccion_servidor;
    direccion_servidor.sin_family = AF_INET;
    direccion_servidor.sin_port = htons(puerto);

    if (inet_pton(AF_INET, direccion_ip, &direccion_servidor.sin_addr) <= 0) {
        std::cerr << "Dirección no válida o no soportada\n";
        close(socket_cliente);
        return -1;
    }

    if (connect(socket_cliente, (struct sockaddr *)&direccion_servidor, sizeof(direccion_servidor)) < 0) {
        std::cerr << "Error al conectar\n";
        close(socket_cliente);
        return -1;
    }

    return socket_cliente;
}

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <dirección IP> <puerto>\n";
        return -1;
    }

    const char* direccion_ip = argv[1];
    int puerto = std::atoi(argv[2]);

    int socket_cliente = conectar_servidor(direccion_ip, puerto);
    if (socket_cliente < 0) return -1;

    char buffer[BUFFER_SIZE] = {0};
    std::string entrada;

    while (true) {
        int valread = read(socket_cliente, buffer, BUFFER_SIZE);
        if (valread <= 0) break;

        std::string mensaje(buffer, valread);
        imprimir_tablero(mensaje);

        if (mensaje.find("Elige una columna") != std::string::npos || mensaje.find("Movimiento inválido") != std::string::npos) {
            std::cin >> entrada;
            send(socket_cliente, entrada.c_str(), entrada.length(), 0);
        }

        memset(buffer, 0, BUFFER_SIZE);  // Limpiar el buffer
    }

    close(socket_cliente);
    return 0;
}
