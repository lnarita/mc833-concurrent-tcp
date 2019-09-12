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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    // socket file descriptor
    int sockfd;
    // endereço de conexão do socket
    struct sockaddr_in servaddr;

    // verifica a quantidade de argumentos do programa
    assertArgumentCount(argc, argv);


    // cria um socket para estabeler uma conexão com o servidor
    // em caso de qualquer erro, o cliente será encerrado
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    // limpa a variável 'servaddr' antes da sua utilização,
    // escrevendo zeros na região de memória da mesma
    bzero(&servaddr, sizeof(servaddr));
    // usando ipv4
    servaddr.sin_family = AF_INET;
    // configura a porta do servidor como sendo 13182
    servaddr.sin_port = htons(atoi(argv[2]));
    // formata / converte o endereço do servidor de string para binário
    // e atribui este valor para o campo sin_addr da variáveo servaddr

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(1);
    }

    for (;;) {
        char commandFromKeyboard[MAXLINE];
        readCommandFromInput(commandFromKeyboard);
        if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
            perror("connect error");
            exit(1);
        }

        sendCommandToServer(sockfd, &servaddr, commandFromKeyboard);
        printf("Command sent \n");
        char stringFromServer[MAXLINE];
        handleServerInput(sockfd, stringFromServer);
        printStringFromServer(stringFromServer);
    }
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
    printf("%s\n", stringFromServer);
}

void handleServerInput(int sockfd, char *stringFromServer) {
    ssize_t n;

    while ((n = read(sockfd, stringFromServer, MAXLINE)) > 0) {
        stringFromServer[n] = 0;
        // TODO HANDLE ERROR ON READ
    }
}

void sendCommandToServer(int sockfd, struct sockaddr_in *servaddr, char *command) {
    strcat(command, "\r\n");
    write(sockfd, command, strlen(command));
}

void readCommandFromInput(char *commandFromKeyboard) {
    scanf("%s", commandFromKeyboard);
}

#pragma clang diagnostic pop



// TODO: DECIDE: open new connection everytime client sends a message <?> or keep connection alive while on program loop