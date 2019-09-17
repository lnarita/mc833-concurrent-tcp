#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 4096

void readCommandFromInput(char *commandFromKeyboard);

void sendCommandToServer(int keyboard, struct sockaddr_in *servaddr, char *command);

void handleServerInput(int sockfd, char *stringFromServer);

void printStringFromServer(char *stringFromServer);

void assertArgumentCount(int argc, char **argv);

int connectWithServer(struct sockaddr_in *pIn, char *serverAddress, char *serverPort);

void printConnectionInfo(int sockfd);

void removeNewLineCharacterFromCommand(char *commandFromKeyboard);

void printCommandSent(const char *commandFromKeyboard);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    // verifica a quantidade de argumentos do programa
    assertArgumentCount(argc, argv);


    for (;;) {
        char commandFromKeyboard[MAXLINE];
        readCommandFromInput(commandFromKeyboard);

        // endereço de conexão do socket
        struct sockaddr_in servaddr;

        // socket file descriptor
        int sockfd = connectWithServer(&servaddr, argv[1], argv[2]);
        printConnectionInfo(sockfd);
        sendCommandToServer(sockfd, &servaddr, commandFromKeyboard);
        printCommandSent(commandFromKeyboard);

        char stringFromServer[MAXLINE];
        handleServerInput(sockfd, stringFromServer);
        printStringFromServer(stringFromServer);
        close(sockfd);
    }
}

void printCommandSent(const char *commandFromKeyboard) {
    printf("Command sent to server: %s\n", commandFromKeyboard);
}

void printConnectionInfo(int sockfd) {
    struct sockaddr_in local_address;
    socklen_t addr_size = sizeof(local_address);
    getsockname(sockfd, (struct sockaddr *) &local_address, &addr_size);
    printf("Connection established using local ip %s and local port %d\n", inet_ntoa(local_address.sin_addr),
           (int) ntohs(local_address.sin_port));
}

int connectWithServer(struct sockaddr_in *servaddr, char *serverAddress, char *serverPort) {
    int sockfd;

    printf("Connecting to server %s on port %s\n", serverAddress, serverPort);

    // cria um socket para estabeler uma conexão com o servidor
    // em caso de qualquer erro, o cliente será encerrado
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }


    // limpa a variável 'servaddr' antes da sua utilização,
    // escrevendo zeros na região de memória da mesma
    bzero(servaddr, sizeof(*servaddr));
    // usando ipv4
    (*servaddr).sin_family = AF_INET;
    // configura a porta do servidor como sendo 13182
    (*servaddr).sin_port = htons(atoi(serverPort));
    // formata / converte o endereço do servidor de string para binário
    // e atribui este valor para o campo sin_addr da variáveo servaddr

    if (inet_pton(AF_INET, serverAddress, &((*servaddr).sin_addr)) <= 0) {
        perror("inet_pton error");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *) servaddr, sizeof(*servaddr)) < 0) {
        perror("connect error");
        exit(1);
    }

    return sockfd;
}

void assertArgumentCount(int argc, char **argv) {
    char error[MAXLINE + 1];
    if (argc != 3) {
        // se o endereço do servidor não foi fornecido
        // exibe a forma correta de usar o programa na saída
        // padrão de erro
        strcpy(error, "uso: ");
        strcat(error, argv[0]);
        strcat(error, "<IP Address>");
        strcat(error, "<server port>");
        perror(error);
        // finaliza a execução com um código de erro
        exit(1);
    }
}

void printStringFromServer(char *stringFromServer) {
    printf("Command received from server: %s\n", stringFromServer);
}

void handleServerInput(int sockfd, char *stringFromServer) {
    ssize_t n;
    n = read(sockfd, stringFromServer, MAXLINE);
    stringFromServer[n] = '\0';
}

void sendCommandToServer(int sockfd, struct sockaddr_in *servaddr, char *command) {
    write(sockfd, command, strlen(command));
}

void readCommandFromInput(char *commandFromKeyboard) {
    fgets(commandFromKeyboard, MAXLINE, stdin);
    removeNewLineCharacterFromCommand(commandFromKeyboard);         // avoid sending an unnecessary extra char
}

void removeNewLineCharacterFromCommand(char *commandFromKeyboard) {
    for (int i = 0; i < strlen(commandFromKeyboard); i++) {
        if (commandFromKeyboard[i] == '\n') {
            commandFromKeyboard[i] = '\0';
            return;
        }
    }
}

#pragma clang diagnostic pop