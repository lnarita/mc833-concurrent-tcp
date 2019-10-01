#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

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

ssize_t readCommandFromClient(int connfd, char *recvline);

void sendMessageToClient(int connfd, char *message);

void executeCommandFromClient(char *command, char *messageToClient);

void assertArgumentCount(int argc, char **argv);

void printConnectedClientInfo(struct sockaddr_in *clientInfo);

void disconnectClientAndSaveInfo(struct sockaddr_in *clientInfo, char *connectTime);

void printCommandExecutedByClient(struct sockaddr_in *clientInfo, char *command);

int isExitCommand(const char *recvline);

typedef void Sigfunc(int);

Sigfunc *Signal(int signo, Sigfunc *func) {
    struct sigaction act, oact;
    act.sa_handler = func;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    } else {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }
    if (sigaction(signo, &act, &oact) < 0) return (SIG_ERR);
    return (oact.sa_handler);
}

void handleSigChild(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("child %d terminated \n", pid);
    }
}


int main(int argc, char **argv) {
    int connfd;
    struct sockaddr_in servaddr;

    assertArgumentCount(argc, argv);

    int listenfd = createListenfd();
    initializeServAddr(&servaddr, atoi(argv[1]));

    // conecta o socket ao endereço configurado
    Bind(listenfd, &servaddr, sizeof(servaddr));

    // permite que o processo escute no socked previamente configurados por conexões que podem chegar
    Listen(listenfd, atoi(argv[2]));
    sleep(1000);

    // o servidor fica em um loop permanente, aguardando conexões que podem chegar
    // e tratando-as, respondendo-as da forma apropriada
    Signal(SIGCHLD, handleSigChild);
    for (;;) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);

        // aguarda a conexão de um cliente
        connfd = Accept(listenfd, (struct sockaddr *) &clientAddress, &clientAddressLength);
        if (connfd == -4) {
            continue;
        }

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
    if (argc != 3) {
        // se o endereço do servidor não foi fornecido
        // exibe a forma correta de usar o programa na saída
        // padrão de erro
        strcpy(error, "uso: ");
        strcat(error, argv[0]);
        strcat(error, "<server port> ");
        strcat(error, "<backlog>");
        perror(error);
        // finaliza a execução com um código de erro
        exit(1);
    }
}

int createListenfd() { return Socket(AF_INET, SOCK_STREAM, 0); }

void handleClientConnectionOnChildProcess(int connfd, int listenfd, struct sockaddr_in clientInfo) {
    close(listenfd);        // libera o socket da conexão de listen (apenas o pai deve escutar por clientes)
    handleClientConnection(connfd, clientInfo);

    // fecha a conexão com o cliente, depois que o mesmo se desconectou
    printf("morrendo\n");
    close(connfd);
    exit(0);
}

void handleClientConnection(int connfd, struct sockaddr_in clientInfo) {
    char recvline[MAX_LENGTH + 1];
    time_t connectTime;
    time(&connectTime);
    char connectTimeStr[MAX_LENGTH];

    // obtém o horário em que o cliente se conectou ao servidor
    strcpy(connectTimeStr, ctime(&connectTime));
    printConnectedClientInfo(&clientInfo);

    // infinitamente, enquanto o cliente está conectado
    for (;;) {
        // lê um comando do cliente
        ssize_t bytesReadCount = readCommandFromClient(connfd, recvline);

        // se foi o comando de finalização ou se não conseguiu ler nada, fecha a conexão e salva no log
        if (isExitCommand(recvline) || bytesReadCount == 0) {
            disconnectClientAndSaveInfo(&clientInfo, connectTimeStr);
            return;
        }

        printCommandExecutedByClient(&clientInfo, recvline);
        char messageToClient[MAX_LENGTH];
        executeCommandFromClient(recvline, messageToClient);
        sendMessageToClient(connfd, messageToClient);       // envia o resultado do comando ao cliente
    }
}

int isExitCommand(const char *recvline) {
    return strcmp(recvline, EXIT_COMMAND_MESSAGE_FROM_CLIENT) == 0;
}

void printCommandExecutedByClient(struct sockaddr_in *clientInfo, char *command) {
    printf("Client %s from port %d is executing command \"%s\"\n",
           inet_ntoa((*clientInfo).sin_addr),
           (int) ntohs((*clientInfo).sin_port),
           command);
}

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

    printf("Client %s from port %d disconnected\n", inet_ntoa((*clientInfo).sin_addr),
           (int) ntohs((*clientInfo).sin_port));
}

void printConnectedClientInfo(struct sockaddr_in *clientInfo) {
    printf("Client %s connected from his %d port\n", inet_ntoa((*clientInfo).sin_addr),
           (int) ntohs((*clientInfo).sin_port));
}

/**
 * Executa um comando do cliente, colocando o resultado do comando em
 * [messageToClient]
 */
void executeCommandFromClient(char *command, char *messageToClient) {
    char commandResult[MAX_LENGTH];
    char commandToBeExecuted[MAX_LENGTH];
    strcpy(commandToBeExecuted, command);
    strcat(commandToBeExecuted, " 2>&1");       // para pegar os resultados das saídas de erro também

    FILE *filePointer = popen(commandToBeExecuted, "r");
    strcpy(messageToClient, "");
    while (fgets(commandResult, sizeof(commandResult), filePointer) != NULL) {
        strcat(messageToClient, commandResult);
    }
    pclose(filePointer);
}

void sendMessageToClient(int connfd, char *message) {
    write(connfd, message, strlen(message));
}

/**
 * Retorna a quantidade de bytes lidos
 */
ssize_t readCommandFromClient(int connfd, char *recvline) {
    ssize_t n;
    n = read(connfd, recvline, MAX_LENGTH);
    recvline[n] = '\0';
    return n;
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
    printf("Listen value: %d\n", backlog);
    if (listen(sockfd, backlog) == -1) {
        perror("listen");
        exit(1);
    }
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int connfd;
    if ((connfd = accept(sockfd, addr, addrlen)) == -1) {
        if (errno == EINTR) {
            return -4;
        }

        perror("accept");
        exit(1);
    }

    return connfd;
}
