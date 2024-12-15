#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

const int PORT = 8080;
int counter = 0;

void handle_client(int client_socket) {
    // Буфер для запроса
    char buffer[1024] = {0};
    read(client_socket, buffer, 1024);

    std::cout << "Получен запрос:" << counter++ << std::endl;
    // Простой HTTP-ответ
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>Hello, World!</h1></body></html>";

    send(client_socket, response.c_str(), response.size(), 0);

    // Закрыть соединение
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Создать сокет
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    // Настройка сокета
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Ошибка setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Привязка сокета к порту
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Ошибка bind");
        exit(EXIT_FAILURE);
    }

    // Слушать соединения
    if (listen(server_fd, 10) < 0) {
        perror("Ошибка listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Сервер запущен на порту " << PORT << "...\n";

    // Основной цикл обработки клиентов
    while (true) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Ошибка accept");
            exit(EXIT_FAILURE);
        }

        // Обработка клиента в отдельном потоке
        std::thread(handle_client, client_socket).detach();
    }

    return 0;
}
