#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#define LISTENQ 10
#define MAXDATASIZE 100

void handleClientConnectionOnChildProcess(int connfd, int i);

void handleClientConnection(int connfd);

int main(int argc, char **argv) {
    int listenfd, connfd;
    struct sockaddr_in servaddr;

    // cria um socket para escutar conexões que chegarão a partir
    // de clientes. Caso haja um erro ao criar o socket, o servidor é finalizado.
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Escreve zeros na região de memória de servaddr
    // de forma a "limpar" esta variável antes de utilizá-la.
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);       // configura o IP do servidor (IP da máquina local)
    servaddr.sin_port = htons(13182);                   // configura a porta na qual o servidor rodará

    // conecta o socket ao endereço configurado
    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    // permite que o processo escute no socked previamente configurados por conexões que podem chegar
    // LISTENQ configura o número de conexões que podem ficar esperando enquanto o processo
    // trata uma conexão particular
    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen");
        exit(1);
    }

    // o servidor fica em um loop permanente, aguardando conexões que podem chegar
    // e tratando-as, respondendo-as da forma apropriada
    for (;;) {
        // fica esperando por conexões que podem chegar de clientes.
        // enquanto isto,  o processo fica bloqueado, sendo "acordado" novamente
        // quando uma conexão com um cliente for estabelecida
        if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1) {
            perror("accept");
            exit(1);
        }

        if (fork() == 0) {
            handleClientConnectionOnChildProcess(connfd, listenfd);
        }
        close(connfd);
    }

    return 0;
}

void handleClientConnectionOnChildProcess(int connfd, int listenfd) {
    close(listenfd);
    handleClientConnection(connfd);

    // fecha a conexão com o cliente, liberando assim a porta do servidor
    // para que uma nova conexão, com um possivelmente distinto cliente,
    // seja iniciada
    close(connfd);
    exit(0);
}

void handleClientConnection(int connfd) {
    char buf[MAXDATASIZE];
    // obtém um timer
    time_t ticks = time(NULL);

    // escreve em um buffer a data e hora atual, utilizando o timer fornecido
    snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));

    // utilizando o buffer previamente preenchido, escreve no file descriptor
    // do socket "conectado" a data e a hora, fazendo assim com que
    // a data e a hora sejam enviadas para o cliente conectado
    write(connfd, buf, strlen(buf));

    struct sockaddr_in peer;

    // imprime na saída padrão o IP do cliente conectado
    printf("Client IP Address: %s\n", inet_ntoa(peer.sin_addr));

    // imprime na saída padrão a porta do cliente conectado
    printf("Client Port: %d\n", (int) ntohs(peer.sin_port));
}