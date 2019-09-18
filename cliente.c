#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LENGTH 4096

void readCommandFromInput(char *commandFromKeyboard);

void sendCommandToServer(int sockfd, char *command);

void handleServerInput(int sockfd, char *stringFromServer);

void printStringFromServer(char *stringFromServer);

void assertArgumentCount(int argc, char **argv);

int connectWithServer(struct sockaddr_in *servaddr, char *serverAddress, char *serverPort);

void printConnectionInfo(int sockfd);

void removeNewLineCharacterFromCommand(char *commandFromKeyboard);

void printCommandSent(const char *commandFromKeyboard);

// wrapper functions
void Inet_pton(struct sockaddr_in *servaddr, const char *serverAddress);

void Connect(const struct sockaddr_in *servaddr, int sockfd);

int Socket(int family, int type, int flags);
// end wrapper functions

void finishClient(int sockfd);

int isEmpty(const char *s);

int isExitCommand(const char *commandFromKeyboard);

int isExitCommandValue(const char *commandFromKeyboard);

#define EXIT_COMMAND "exit"
#define EXIT_COMMAND_MESSAGE_TO_SERVER "dedmorrided$"

int main(int argc, char **argv) {
    // verifica a quantidade de argumentos do programa
    assertArgumentCount(argc, argv);

    // endereço de conexão do socket
    struct sockaddr_in servaddr;
    int sockfd = connectWithServer(&servaddr, argv[1], argv[2]);
    printConnectionInfo(sockfd);

    for (;;) {
        char commandFromKeyboard[MAX_LENGTH];
        readCommandFromInput(commandFromKeyboard);

        if (isExitCommand(commandFromKeyboard) || isExitCommandValue(commandFromKeyboard)) {
            finishClient(sockfd);
            break;
        }

        // evita de enviar comandos vazios para o servidor
        if (isEmpty(commandFromKeyboard)) {
            continue;
        }

        sendCommandToServer(sockfd, commandFromKeyboard);
        printCommandSent(commandFromKeyboard);          // imprime vomando enviado ao servidor

        char serverInputDestination[MAX_LENGTH];
        handleServerInput(sockfd, serverInputDestination);
        printStringFromServer(serverInputDestination);      // exibe a mensagem retornada pelo servidor
    }

    return 0;
}

int isExitCommandValue(const char *commandFromKeyboard) {
    return strcmp(commandFromKeyboard, EXIT_COMMAND_MESSAGE_TO_SERVER) == 0;
}

int isExitCommand(const char *commandFromKeyboard) {
    return strcmp(commandFromKeyboard, EXIT_COMMAND) == 0;
}

void finishClient(int sockfd) {
    sendCommandToServer(sockfd, EXIT_COMMAND_MESSAGE_TO_SERVER);
    close(sockfd);
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

    // cria um socket para estabelecer uma conexão com o servidor
    // em caso de qualquer erro, o cliente será encerrado
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);


    // limpa a variável 'servaddr' antes da sua utilização,
    // escrevendo zeros na região de memória da mesma
    bzero(servaddr, sizeof(*servaddr));

    // usando ipv4
    (*servaddr).sin_family = AF_INET;
    // configura a porta do servidor
    (*servaddr).sin_port = htons(atoi(serverPort));

    // formata / converte o endereço do servidor de string para binário
    // e atribui este valor para o campo sin_addr da variáveo servaddr
    Inet_pton(servaddr, serverAddress);

    Connect(servaddr, sockfd);      // conecta com o servidor

    return sockfd;
}

void assertArgumentCount(int argc, char **argv) {
    char error[MAX_LENGTH + 1];
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
    printf("%s", stringFromServer);
}

void handleServerInput(int sockfd, char *stringFromServer) {
    ssize_t n;
    n = read(sockfd, stringFromServer, MAX_LENGTH);
    stringFromServer[n] = '\0';
}

void sendCommandToServer(int sockfd, char *command) {
    write(sockfd, command, strlen(command));
}

void readCommandFromInput(char *commandFromKeyboard) {
    fgets(commandFromKeyboard, MAX_LENGTH, stdin);
    removeNewLineCharacterFromCommand(commandFromKeyboard);         // evita de enviar um \n desnecessário
}

void removeNewLineCharacterFromCommand(char *commandFromKeyboard) {
    for (int i = 0; i < strlen(commandFromKeyboard); i++) {
        if (commandFromKeyboard[i] == '\n') {
            commandFromKeyboard[i] = '\0';
            return;
        }
    }
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

void Connect(const struct sockaddr_in *servaddr, int sockfd) {
    if (connect(sockfd, (struct sockaddr *) servaddr, sizeof(*servaddr)) < 0) {
        perror("connect error");
        exit(1);
    }
}

void Inet_pton(struct sockaddr_in *servaddr, const char *serverAddress) {
    if (inet_pton(AF_INET, serverAddress, &((*servaddr).sin_addr)) <= 0) {
        perror("inet_pton error");
        exit(1);
    }
}

int isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int isEmpty(const char *s) {
    while (*s != '\0') {
        if (!isSpace(*s)) {
            return 0;
        }
        s++;
    }
    return 1;
}