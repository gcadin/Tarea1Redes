#include <iostream>
#include <vector>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <cstdlib>
#include <ctime>
#include <pthread.h>

#define FILAS 6
#define COLUMNAS 7
#define PORT 7777

std::mutex mtx;
int clientes_conectados = 0;

struct Juego {
    int id;
    char tablero[FILAS][COLUMNAS];
    int cliente1;
    int cliente2;
    bool turno_cliente1;

    Juego(int id, int c1, int c2, bool turno_inicial) : id(id), cliente1(c1), cliente2(c2), turno_cliente1(turno_inicial) {
        memset(tablero, ' ', sizeof(tablero));
    }

    void enviar_mensaje(int cliente, const std::string& mensaje) {
        send(cliente, mensaje.c_str(), mensaje.length(), 0);
    }

    bool colocar_ficha(int columna, char ficha) {
        if (columna < 0 || columna >= COLUMNAS || tablero[0][columna] != ' ') {
            return false;
        }
        for (int i = FILAS - 1; i >= 0; --i) {
            if (tablero[i][columna] == ' ') {
                tablero[i][columna] = ficha;
                return true;
            }
        }
        return false;
    }

    bool verificar_ganador(char ficha) {
        for (int i = 0; i < FILAS; ++i) {
            for (int j = 0; j < COLUMNAS - 3; ++j) {
                if (tablero[i][j] == ficha && tablero[i][j+1] == ficha && tablero[i][j+2] == ficha && tablero[i][j+3] == ficha) {
                    return true;
                }
            }
        }

        for (int i = 0; i < FILAS - 3; ++i) {
            for (int j = 0; j < COLUMNAS; ++j) {
                if (tablero[i][j] == ficha && tablero[i+1][j] == ficha && tablero[i+2][j] == ficha && tablero[i+3][j] == ficha) {
                    return true;
                }
            }
        }

        for (int i = 0; i < FILAS - 3; ++i) {
            for (int j = 0; j < COLUMNAS - 3; ++j) {
                if (tablero[i][j] == ficha && tablero[i+1][j+1] == ficha && tablero[i+2][j+2] == ficha && tablero[i+3][j+3] == ficha) {
                    return true;
                }
            }
        }

        for (int i = 3; i < FILAS; ++i) {
            for (int j = 0; j < COLUMNAS - 3; ++j) {
                if (tablero[i][j] == ficha && tablero[i-1][j+1] == ficha && tablero[i-2][j+2] == ficha && tablero[i-3][j+3] == ficha) {
                    return true;
                }
            }
        }

        return false;
    }
};

std::vector<Juego> juegos;

void enviar_tablero(const Juego& juego, int cliente1, int cliente2) {
    std::string tablero_str = "\n\n      TABLERO\n";
    for (int i = 0; i < FILAS; ++i) {
        tablero_str += std::to_string(i + 1) + " ";
        for (int j = 0; j < COLUMNAS; ++j) {
            tablero_str += juego.tablero[i][j];
            tablero_str += " ";
        }
        tablero_str += "\n";
    }
    tablero_str += "  1 2 3 4 5 6 7\n\n";
    send(cliente1, tablero_str.c_str(), tablero_str.length(), 0);
    send(cliente2, tablero_str.c_str(), tablero_str.length(), 0);
}

void* manejar_juego(void* arg) {
    Juego* juego = static_cast<Juego*>(arg);

    std::cout << "Juego " << juego->id << " iniciado" << std::endl;

    struct sockaddr_in client_addr1, client_addr2;
    socklen_t client_len1 = sizeof(client_addr1);
    socklen_t client_len2 = sizeof(client_addr2);

    getpeername(juego->cliente1, (struct sockaddr*)&client_addr1, &client_len1);
    getpeername(juego->cliente2, (struct sockaddr*)&client_addr2, &client_len2);

    while (true) {
        int jugador_actual = juego->turno_cliente1 ? juego->cliente1 : juego->cliente2;
        int otro_jugador = juego->turno_cliente1 ? juego->cliente2 : juego->cliente1;

        struct sockaddr_in client_addr_actual;
        socklen_t client_len_actual = sizeof(client_addr_actual);
        getpeername(jugador_actual, (struct sockaddr*)&client_addr_actual, &client_len_actual);

        std::string ip_actual = inet_ntoa(client_addr_actual.sin_addr);
        int puerto_actual = ntohs(client_addr_actual.sin_port);

        juego->enviar_mensaje(jugador_actual, "Elige una columna (1-7):\n");

        char buffer[3];
        int valread = read(jugador_actual, buffer, 3);
        if (valread <= 0) break;

        int columna = buffer[0] - '1';
        if (columna < 0 || columna >= COLUMNAS) {
            juego->enviar_mensaje(jugador_actual, "Columna inválida. Intenta de nuevo.\n");
            continue;
        }

        bool ficha_colocada = juego->colocar_ficha(columna, juego->turno_cliente1 ? 'S' : 'C');
        if (!ficha_colocada) {
            juego->enviar_mensaje(jugador_actual, "Columna llena. Intenta de nuevo.\n");
            continue;
        }

        std::cout << "Juego " << juego->id << " [" << ip_actual << ":" << puerto_actual << "] juega en la columna " << (columna + 1) << ".\n";
        enviar_tablero(*juego, juego->cliente1, juego->cliente2);

        if (juego->verificar_ganador(juego->turno_cliente1 ? 'S' : 'C')) {
            juego->enviar_mensaje(jugador_actual, "¡Has ganado!\n");
            juego->enviar_mensaje(otro_jugador, "El otro jugador ha ganado.\n");
            std::cout << "Juego " << juego->id << " [" << ip_actual << ":" << puerto_actual << "] ha ganado " << (juego->turno_cliente1 ? "cliente.\n" : "servidor.\n");
            break;
        }

        juego->turno_cliente1 = !juego->turno_cliente1;
    }

    close(juego->cliente1);
    close(juego->cliente2);
    std::cout << "Juego [" << juego->id << "] finalizado" << std::endl;
    delete juego;
    return nullptr;
}

void* hilo_juego(void* arg) {
    Juego* juego = static_cast<Juego*>(arg);
    manejar_juego((void*)juego);
    delete (Juego*)arg;  // Liberar memoria
    return nullptr;
}

int main() {
    srand(time(0));  // Inicializar la semilla para el generador de números aleatorios

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    std::cout << "Servidor iniciado en el puerto " << PORT << std::endl;

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Socket creado\n";

    // Configurar socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    std::cout << "Socket configurado\n";

    // Asociar socket al puerto
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    std::cout << "Socket asociado al puerto " << PORT << std::endl;

    // Escuchar
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    std::cout << "Esperando conexiones ...\n";   

    while (true) {
        struct sockaddr_in client_addr1, client_addr2;
        socklen_t client_len1 = sizeof(client_addr1);
        socklen_t client_len2 = sizeof(client_addr2);

        // Aceptar conexión del primer jugador
        int cliente1 = accept(server_fd, (struct sockaddr*)&client_addr1, &client_len1);
        if (cliente1 < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        mtx.lock();
        clientes_conectados++;
        mtx.unlock();

        // Mensaje de conexión del primer jugador
        getpeername(cliente1, (struct sockaddr*)&client_addr1, &client_len1);
        int id_juego = juegos.size();
        std::cout << "Juego " << id_juego << " [" << inet_ntoa(client_addr1.sin_addr) << ":" << ntohs(client_addr1.sin_port) << "] se ha unido (cliente 1).\n";

        // Aceptar conexión del segundo jugador
        int cliente2 = accept(server_fd, (struct sockaddr*)&client_addr2, &client_len2);
        if (cliente2 < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        mtx.lock();
        clientes_conectados++;
        mtx.unlock();

        // Mensaje de conexión del segundo jugador
        getpeername(cliente2, (struct sockaddr*)&client_addr2, &client_len2);
        std::cout << "Juego " << id_juego << " [" << inet_ntoa(client_addr2.sin_addr) << ":" << ntohs(client_addr2.sin_port) << "] se ha unido (cliente 2).\n";

        // Determinar aleatoriamente quién inicia el juego
        bool turno_inicial = rand() % 2 == 0;

        // Crear y registrar el nuevo juego
        Juego* nuevo_juego = new Juego(id_juego, cliente1, cliente2, turno_inicial);
        juegos.push_back(*nuevo_juego);

        // Crear hilo para manejar el juego
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, hilo_juego, (void*)nuevo_juego);
    }

    close(server_fd);
    return 0;
}
