#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define LISTENQ 10

#define MAX_LENGTH 4096

// wrapper functions
int Socket(int family, int type, int flags);

void Bind(int sockfd, const struct sockaddr_in *addr, socklen_t addrlen);

void Listen(int sockfd, int backlog);

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// end wrapper functions

void initializeServAddr(struct sockaddr_in *servaddr, int port);

void initializeServAddr(struct sockaddr_in *servaddr, int port) {
    // Escreve zeros na região de memória de servaddr
    // de forma a "limpar" esta variável antes de utilizá-la.
    bzero(servaddr, sizeof((*servaddr)));
    (*servaddr).sin_family = AF_INET;
    (*servaddr).sin_addr.s_addr = htonl(INADDR_ANY);        // configura o IP do servidor (IP da máquina local)
    (*servaddr).sin_port = htons(port);                     // configura a porta na qual o servidor rodará
}

void handleClientConnectionOnChildProcess(int connfd, int listenfd, struct sockaddr_in clientInfo);

void handleClientConnection(int connfd, struct sockaddr_in clientInfo);

int createListenfd();

void readCommandFromClient(int connfd, char *recvline);

void sendMessageToClient(int connfd, char *message);

void executeCommandFromClient(const char *command);

void assertArgumentCount(int argc, char **argv);

void printConnectedClientInfo(struct sockaddr_in *clientInfo);

int main(int argc, char **argv) {
    int connfd;
    struct sockaddr_in servaddr;

    assertArgumentCount(argc, argv);

    int listenfd = createListenfd();
    initializeServAddr(&servaddr, atoi(argv[1]));

    // conecta o socket ao endereço configurado
    Bind(listenfd, &servaddr, sizeof(servaddr));

    // permite que o processo escute no socked previamente configurados por conexões que podem chegar
    // LISTENQ configura o número de conexões que podem ficar esperando enquanto o processo
    // trata uma conexão particular
    Listen(listenfd, LISTENQ);

    // o servidor fica em um loop permanente, aguardando conexões que podem chegar
    // e tratando-as, respondendo-as da forma apropriada
    for (;;) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);

        // aguarda a conexão de um cliente
        connfd = Accept(listenfd, (struct sockaddr *) &clientAddress, &clientAddressLength);

        if (fork() == 0) {
            // no processo filho, trata a conexão do cliente
            handleClientConnectionOnChildProcess(connfd, listenfd, clientAddress);
        }
        // fecha a conexão do processo pai
        close(connfd);
    }

    return 0;
}

void assertArgumentCount(int argc, char **argv) {
    char error[MAX_LENGTH + 1];
    if (argc != 2) {
        // se o endereço do servidor não foi fornecido
        // exibe a forma correta de usar o programa na saída
        // padrão de erro
        strcpy(error, "uso: ");
        strcat(error, argv[0]);
        strcat(error, "<server port>");
        perror(error);
        // finaliza a execução com um código de erro
        exit(1);
    }
}

int createListenfd() { return Socket(AF_INET, SOCK_STREAM, 0); }

void handleClientConnectionOnChildProcess(int connfd, int listenfd, struct sockaddr_in clientInfo) {
    close(listenfd);
    handleClientConnection(connfd, clientInfo);

    // fecha a conexão com o cliente
//    close(connfd);
//    exit(0);
}

void handleClientConnection(int connfd, struct sockaddr_in clientInfo) {
    char recvline[MAX_LENGTH + 1];
    printConnectedClientInfo(&clientInfo);

    for (;;) {
        readCommandFromClient(connfd, recvline);
        sendMessageToClient(connfd, recvline);
        executeCommandFromClient(recvline);
    }
}

void printConnectedClientInfo(struct sockaddr_in *clientInfo) {
    printf("Client %s connected from his %d port\n", inet_ntoa((*clientInfo).sin_addr),
           (int) ntohs((*clientInfo).sin_port));
}

void executeCommandFromClient(const char *command) {
    system(command);
}

void sendMessageToClient(int connfd, char *message) {
    write(connfd, message, strlen(message));
}

void readCommandFromClient(int connfd, char *recvline) {
    ssize_t n;
    n = read(connfd, recvline, MAX_LENGTH);
    recvline[n] = '\0';
}

// wrapper functions
int Socket(int family, int type, int flags) {
    int sockfd;
    if ((sockfd = socket(family, type, flags)) < 0) {
        perror("socket");
        exit(1);
    }

    return sockfd;
}

void Bind(int sockfd, const struct sockaddr_in *addr, socklen_t addrlen) {
    if (bind(sockfd, (struct sockaddr *) addr, addrlen) == -1) {
        perror("bind");
        exit(1);
    }
}

void Listen(int sockfd, int backlog) {
    if (listen(sockfd, backlog) == -1) {
        perror("listen");
        exit(1);
    }
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int connfd;
    if ((connfd = accept(sockfd, addr, addrlen)) == -1) {
        perror("accept");
        exit(1);
    }

    return connfd;
}
