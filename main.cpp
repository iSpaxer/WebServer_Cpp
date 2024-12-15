#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <filesystem>

const int PORT = 8080;
std::string path = std::string(std::filesystem::current_path().parent_path()) + "/resources/";

// Функция для отправки HTML-файла
void send_html_file(int client_socket) {
    std::ifstream file(path + "/index.html");
    if (!file) {
        std::cerr << "Не удалось открыть файл index.html.\n";
        const char* error_response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        send(client_socket, error_response, strlen(error_response), 0);
        return;
    }

    // Считываем содержимое файла
    std::string html_content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    file.close();

    // Формируем HTTP-заголовок
    std::string headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(html_content.size()) + "\r\n"
        "Connection: close\r\n\r\n";

    // Отправляем заголовки и содержимое файла
    send(client_socket, headers.c_str(), headers.size(), 0);
    send(client_socket, html_content.c_str(), html_content.size(), 0);
}

// Функция для отправки изображения
void send_image(int client_socket) {
    std::ifstream file(path + "/image.jpg", std::ios::binary);
    if (!file) {
        std::cerr << "Не удалось открыть файл image.jpg.\n";
        const char* error_response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        send(client_socket, error_response, strlen(error_response), 0);
        return;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[file_size];
    file.read(buffer, file_size);
    file.close();

    std::string headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: " + std::to_string(file_size) + "\r\n"
        "Connection: close\r\n\r\n";

    send(client_socket, headers.c_str(), headers.size(), 0);
    send(client_socket, buffer, file_size, 0);

    delete[] buffer;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Ошибка привязки сокета");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Ошибка при прослушивании");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::filesystem::path current_path = std::filesystem::current_path();
    std::cout << "Сервер запущен на порту " << PORT << " С текущей директорией: " << current_path << std::endl; "\n";

    while (true) {
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
        if (client_socket < 0) {
            perror("Ошибка при подключении клиента");
            continue;
        }

        char request[1024] = {0};
        read(client_socket, request, 1024);

        // Простая маршрутизация
        if (strncmp(request, "GET / ", 6) == 0) {
            send_html_file(client_socket); // Отправляем HTML-файл
        } else if (strncmp(request, "GET /image.jpg", 14) == 0) {
            send_image(client_socket); // Отправляем изображение
        } else {
            const char* response =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n\r\n";
            send(client_socket, response, strlen(response), 0);
        }

        close(client_socket);
    }

    close(server_fd);
    return 0;
}
