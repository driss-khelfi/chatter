// src/client.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

#define PORT 12345
#define BUFFER_SIZE 1024

void receive_messages(int sock) {
    char buffer[BUFFER_SIZE];
    int valread;

    while ((valread = read(sock, buffer, BUFFER_SIZE)) > 0) {
        buffer[valread] = '\0';
        std::cout << buffer << std::endl;
    }

    if (valread == 0) {
        std::cout << "Connexion fermée par le serveur." << std::endl;
    } else {
        perror("Erreur de réception");
    }

    exit(0);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Erreur de création du socket" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "Adresse invalide/Non supportée" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connexion échouée" << std::endl;
        return -1;
    }

    // Recevoir la demande de nom du serveur
    int valread = read(sock, buffer, BUFFER_SIZE);
    buffer[valread] = '\0';
    std::cout << buffer;

    // Entrer le nom et l'envoyer au serveur
    std::string name;
    std::getline(std::cin, name);
    name += "\n";
    send(sock, name.c_str(), name.size(), 0);

    // Lancer un thread pour recevoir les messages
    std::thread recv_thread(receive_messages, sock);
    recv_thread.detach();

    // Envoyer des messages au serveur
    while (true) {
        std::string message;
        std::getline(std::cin, message);
        message += "\n";
        send(sock, message.c_str(), message.size(), 0);
    }

    close(sock);
    return 0;
}
