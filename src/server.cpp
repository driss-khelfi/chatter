// src/serveur.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <map>
#include <ctime>
#include <fcntl.h>

#define PORT 12345
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket, max_sd, activity;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    // Ensemble de descripteurs de fichiers pour select()
    fd_set readfds;

    // Liste des clients connectés
    std::vector<int> client_sockets;

    // Mapping des descripteurs de fichiers aux noms des clients
    std::map<int, std::string> client_names;

    // Création du socket maître
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Erreur de création du socket");
        exit(EXIT_FAILURE);
    }

    // Attacher le socket au port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Erreur de setsockopt");
        exit(EXIT_FAILURE);
    }

    // Définir l'adresse du serveur
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Lier le socket à l'adresse
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Erreur de liaison");
        exit(EXIT_FAILURE);
    }

    // Écouter les connexions entrantes
    if (listen(server_fd, 5) < 0) {
        perror("Erreur d'écoute");
        exit(EXIT_FAILURE);
    }

    std::cout << "Serveur en écoute sur le port " << PORT << std::endl;

    while (true) {
        // Initialiser l'ensemble de descripteurs de fichiers
        FD_ZERO(&readfds);

        // Ajouter le socket maître à l'ensemble
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Ajouter les sockets clients à l'ensemble
        for (int client_socket : client_sockets) {
            FD_SET(client_socket, &readfds);
            if (client_socket > max_sd)
                max_sd = client_socket;
        }

        // Attendre une activité sur l'un des sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("Erreur de select");
        }

        // Si quelque chose se passe sur le socket maître, c'est une nouvelle connexion
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                     (socklen_t *)&addrlen)) < 0) {
                perror("Erreur d'acceptation");
                exit(EXIT_FAILURE);
            }

            // Ajouter le nouveau socket à la liste des clients
            client_sockets.push_back(new_socket);
            std::cout << "Nouvelle connexion, socket fd est " << new_socket
                      << ", IP est : " << inet_ntoa(address.sin_addr)
                      << ", port : " << ntohs(address.sin_port) << std::endl;

            // Demander le nom du client
            const char *request_name = "Veuillez entrer votre nom : ";
            send(new_socket, request_name, strlen(request_name), 0);

            // Recevoir le nom du client
            int valread = read(new_socket, buffer, BUFFER_SIZE);
            buffer[valread - 1] = '\0'; // Enlever le caractère de nouvelle ligne
            client_names[new_socket] = std::string(buffer);

            // Informer tout le monde de la nouvelle connexion
            std::string welcome_msg = client_names[new_socket] + " a rejoint le chat.";
            for (int client_socket : client_sockets) {
                if (client_socket != new_socket) {
                    send(client_socket, welcome_msg.c_str(), welcome_msg.size(), 0);
                }
            }
        }

        // Sinon, c'est une IO sur un des sockets clients
        for (size_t i = 0; i < client_sockets.size(); i++) {
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    // Déconnexion du client
                    getpeername(sd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen);
                    std::cout << "Hôte déconnecté, IP : " << inet_ntoa(address.sin_addr)
                              << ", port : " << ntohs(address.sin_port) << std::endl;

                    // Informer les autres clients
                    std::string disconnect_msg = client_names[sd] + " a quitté le chat.";
                    for (int client_socket : client_sockets) {
                        if (client_socket != sd) {
                            send(client_socket, disconnect_msg.c_str(), disconnect_msg.size(), 0);
                        }
                    }

                    // Fermer le socket et le supprimer de la liste
                    close(sd);
                    client_sockets.erase(client_sockets.begin() + i);
                    client_names.erase(sd);
                    i--;
                } else {
                    // Message reçu, le diffuser à tous les autres clients
                    buffer[valread] = '\0';

                    // Obtenir le timestamp actuel
                    time_t now = time(0);
                    char *dt = ctime(&now);
                    dt[strlen(dt) - 1] = '\0'; // Enlever le caractère de nouvelle ligne

                    std::string message = "[" + std::string(dt) + "] " + client_names[sd] + ": " + std::string(buffer);

                    for (int client_socket : client_sockets) {
                        if (client_socket != sd) {
                            send(client_socket, message.c_str(), message.size(), 0);
                        }
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
