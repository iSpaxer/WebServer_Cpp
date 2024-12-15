#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <filesystem>

#include <arpa/inet.h> // Для inet_ntoa()
#include <thread>

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
    // sleep(10);
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


void handle_client(int client_socket, sockaddr_in client_address) {
    // Получаем IP-адрес клиента и порт
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_address.sin_port);

    std::cout << "Получен запрос от IP: " << client_ip << ", Порт: " << client_port << std::endl;

    char buffer[512] = {0};
    std::string request;

    ssize_t bytes_read;
    auto counter = 0;
    while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0) {
        request.append(buffer, bytes_read); // Динамически добавляем данные
        if (request.find("\r\n\r\n") != std::string::npos) {
            break; // Конец заголовков HTTP
        }
    }

    // Простая маршрутизация
    if (strncmp(request.c_str(), "GET / ", 6) == 0) {
        send_html_file(client_socket); // Отправляем HTML-файл
    } else if (strncmp(request.c_str(), "GET /image.jpg", 14) == 0) {
        send_image(client_socket); // Отправляем изображение
    } else {
        const char* response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
    }

    close(client_socket);  // Закрытие соединения с клиентом
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
    std::cout << "Сервер запущен на порту " << PORT << " с текущей директорией: " << current_path << std::endl;

    while (true) {
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
        if (client_socket < 0) {
            perror("Ошибка при подключении клиента");
            continue;
        }

        // Создаем новый поток для обслуживания клиента
        std::thread client_thread(handle_client, client_socket, client_address);
        client_thread.detach(); // Отделяем поток от основного, чтобы он мог работать в фоновом режиме
    }

    close(server_fd);
    return 0;
}
