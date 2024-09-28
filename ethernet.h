#ifndef ETHERNET_H
#define ETHERNET_H

#include <iostream>
#include <cstring>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <errno.h>

class Ethernet {
public:
    static bool stopListeningTCP;
    static bool stopListeningUDP;

    static int sendMessageTCP(const std::string& ip, int port, const std::string& message) {
        int sockfd = createTCPSocket();
        if (sockfd < 0) return -1;

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << strerror(errno) << std::endl;
            close(sockfd);
            return -1;
        }

        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed: " << strerror(errno) << std::endl;
            close(sockfd);
            return -1;
        }

        if (send(sockfd, message.c_str(), message.length(), 0) < 0) {
            std::cerr << "Failed to send message: " << strerror(errno) << std::endl;
            close(sockfd);
            return -1;
        }

        close(sockfd);
        return 0;
    }

   static void listenForMessagesTCPfromIP(const std::string& ip, int port, std::function<void(const std::string&)> onMessageReceived) {
        int sockfd = createTCPSocket();
        if (sockfd < 0) return;

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << strerror(errno) << std::endl;
            close(sockfd);
            return;
        }

        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed: " << strerror(errno) << std::endl;
            close(sockfd);
            return;
        }

        std::thread listenerThread([sockfd, onMessageReceived]() {
            char buffer[1024];
            struct timeval timeout = {10, 0};
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

            while (!stopListeningTCP) {
                int bytesReceived = recv(sockfd, buffer, sizeof(buffer), 0);
                if (bytesReceived <= 0) {
                    if (bytesReceived < 0) {
                        std::cerr << "Error receiving message: " << strerror(errno) << std::endl;
                    } else {
                        std::cerr << "Connection closed" << std::endl;
                    }
                    break;
                }
                std::string message(buffer, bytesReceived);
                onMessageReceived(message);
            }

            close(sockfd);
        });
        listenerThread.detach();
}

    static void listenForMessagesTCP(int port, std::function<void(const std::string&)> onMessageReceived) {
            int server_sockfd = createTCPSocket();
            if (server_sockfd < 0) return;

            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;   //Listen on all interfaces
            server_addr.sin_port = htons(port);

            if (bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Failed to bind TCP socket: " << strerror(errno) << std::endl;
                close(server_sockfd);
                return;
            }

            if (listen(server_sockfd, 5) < 0) {   //Backlog of 5
                std::cerr << "Failed to listen on TCP socket: " << strerror(errno) << std::endl;
                close(server_sockfd);
                return;
            }

            std::thread listenerThread([server_sockfd, onMessageReceived]() {
                while (!stopListeningTCP) {
                    sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &addr_len);
                    if (client_sockfd < 0) {
                        if (errno != EINTR) {
                            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
                        }
                        continue;
                    }

                    char buffer[1024];
                    while (true) {
                        int bytesReceived = recv(client_sockfd, buffer, sizeof(buffer), 0);
                        if (bytesReceived <= 0) {
                            if (bytesReceived < 0) {
                                std::cerr << "Error receiving message: " << strerror(errno) << std::endl;
                            } else {
                                std::cerr << "Connection closed" << std::endl;
                            }
                            break;
                        }
                        std::string message(buffer, bytesReceived);
                        onMessageReceived(message);
                    }
                    close(client_sockfd);
                }
                close(server_sockfd);
            });
            listenerThread.detach();
        }

    static int sendMessageUDP(const std::string& ip, int port, const std::string& message, bool enableBroadcast = false) {
        int sockfd = createUDPSocket();
        if (sockfd < 0) return -1;

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << strerror(errno) << std::endl;
            close(sockfd);
            return -1;
        }

        if (enableBroadcast) {
            int broadcastEnable = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
        }

        if (sendto(sockfd, message.c_str(), message.length(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to send UDP message: " << strerror(errno) << std::endl;
            close(sockfd);
            return -1;
        }

        close(sockfd);
        return 0;
    }

    static void listenForMessagesUDP(int port, std::function<void(const std::string&)> onMessageReceived) {
        int sockfd = createUDPSocket();
        if (sockfd < 0) return;

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to bind UDP socket: " << strerror(errno) << std::endl;
            close(sockfd);
            return;
        }

        std::thread listenerThread([sockfd, onMessageReceived]() {
            char buffer[1024];
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            while (!stopListeningUDP) {
                int bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_len);
                if (bytesReceived <= 0) {
                    std::cerr << "Error receiving UDP message: " << strerror(errno) << std::endl;
                    break;
                }
                std::string message(buffer, bytesReceived);
                onMessageReceived(message);
            }

            close(sockfd);
        });
        listenerThread.detach();
    }

    static void stopListeningThreads() {
        stopListeningTCP = true;
        stopListeningUDP = true;
    }
    static void stopListeningForTCP() {
        stopListeningTCP = true;
    }
    static void stopListeningForUDP() {
        stopListeningUDP = true;
    }


private:
    static int createTCPSocket() {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating TCP socket: " << strerror(errno) << std::endl;
            return -1;
        }
        return sockfd;
    }

    static int createUDPSocket() {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating UDP socket: " << strerror(errno) << std::endl;
            return -1;
        }
        return sockfd;
    }
};

#endif  //ETHERNET_H
