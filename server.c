#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define LISTENQ 10

#define MAX_LENGTH 4096
#define EXIT_COMMAND_MESSAGE_FROM_CLIENT "dedmorrided$"

#define LOG_FILE_NAME "connections.log"

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

void executeCommandFromClient(char command[4097], char *messageToClient);

void assertArgumentCount(int argc, char **argv);

void printConnectedClientInfo(struct sockaddr_in *clientInfo);

void disconnectClientAndSaveInfo(struct sockaddr_in *clientInfo, char *connectTime);

void printCommandExecutedByClient(struct sockaddr_in *clientInfo, char command[4097]);

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
    close(connfd);
    exit(0);
}

void handleClientConnection(int connfd, struct sockaddr_in clientInfo) {
    char recvline[MAX_LENGTH + 1];
    printConnectedClientInfo(&clientInfo);
    time_t connectTime;
    time(&connectTime);
    char connectTimeStr[MAX_LENGTH];
    strcpy(connectTimeStr, ctime(&connectTime));

    for (;;) {
        readCommandFromClient(connfd, recvline);

        if (strcmp(recvline, EXIT_COMMAND_MESSAGE_FROM_CLIENT) == 0) {
            disconnectClientAndSaveInfo(&clientInfo, connectTimeStr);
            return;
        }

        printCommandExecutedByClient(&clientInfo, recvline);
        char messageToClient[MAX_LENGTH];
        executeCommandFromClient(recvline, messageToClient);
        sendMessageToClient(connfd, messageToClient);
    }
}

void printCommandExecutedByClient(struct sockaddr_in *clientInfo, char command[4097]) {
    printf("Client %s from port %d is executing command \"%s\"\n",
           inet_ntoa((*clientInfo).sin_addr),
           (int) ntohs((*clientInfo).sin_port),
           command);
}

// TODO: reuse values
void disconnectClientAndSaveInfo(struct sockaddr_in *clientInfo, char *connectTime) {
    char buffer[MAX_LENGTH];
    time_t disconnectTime;
    time(&disconnectTime);

    snprintf(buffer, sizeof(buffer), "IP: %s | PORT: %d | CONNECT TIME: %.24s | DISCONNECT TIME: %.24s",
             inet_ntoa((*clientInfo).sin_addr),
             (int) ntohs((*clientInfo).sin_port),
             connectTime,
             ctime(&disconnectTime));

    char command[MAX_LENGTH];

    strcpy(command, "echo \"");
    strcat(command, buffer);
    strcat(command, "\" >> ");
    strcat(command, LOG_FILE_NAME);
    system(command);

    printf("Client %s from port %d disconnected\n", inet_ntoa((*clientInfo).sin_addr), (int) ntohs((*clientInfo).sin_port));
}

void printConnectedClientInfo(struct sockaddr_in *clientInfo) {
    printf("Client %s connected from his %d port\n", inet_ntoa((*clientInfo).sin_addr),
           (int) ntohs((*clientInfo).sin_port));
}

void executeCommandFromClient(char command[4097], char *messageToClient) {
    char commandResult[MAX_LENGTH];         // TODO CHECK MAX LENGTH;
    char commandToBeExecuted[MAX_LENGTH];
    strcpy(commandToBeExecuted, command);
    strcat(commandToBeExecuted, " 2>&1");

    FILE* filePointer = popen(commandToBeExecuted, "r");
    strcpy(messageToClient, "");
    while (fgets(commandResult, sizeof(commandResult), filePointer) != NULL) {
        strcat(messageToClient, commandResult);
    }
    pclose(filePointer);
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
