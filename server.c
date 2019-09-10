#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#define LISTENQ 10
#define MAXDATASIZE 100

// wrapper functions (TODO: RENAME PARAMETER NAMES)
int Socket(int family, int type, int flags);

void Bind(int i, const struct sockaddr_in *a, socklen_t t);

void Listen(int i, int j);

void handleClientConnectionOnChildProcess(int connfd, int listenfd);

void handleClientConnection(int connfd);

int createListenfd();

int main(int argc, char **argv) {
    int connfd;
    struct sockaddr_in servaddr;

    int listenfd = createListenfd();

    // Escreve zeros na região de memória de servaddr
    // de forma a "limpar" esta variável antes de utilizá-la.
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);       // configura o IP do servidor (IP da máquina local)
    servaddr.sin_port = htons(13182);                   // configura a porta na qual o servidor rodará

    // conecta o socket ao endereço configurado
    Bind(listenfd, &servaddr, sizeof(servaddr));

    // permite que o processo escute no socked previamente configurados por conexões que podem chegar
    // LISTENQ configura o número de conexões que podem ficar esperando enquanto o processo
    // trata uma conexão particular
    Listen(listenfd, LISTENQ);

    // o servidor fica em um loop permanente, aguardando conexões que podem chegar
    // e tratando-as, respondendo-as da forma apropriada
    for (;;) {
        // wait for client connection
        connfd = Accept(listenfd, (struct sockaddr *) NULL, NULL);

        if (fork() == 0) {
            handleClientConnectionOnChildProcess(connfd, listenfd);
        }
        close(connfd);
    }

    return 0;
}

int createListenfd() { return Socket(AF_INET, SOCK_STREAM, 0); }

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

// wrapper functions
// cria um socket para escutar conexões que chegarão a partir
// de clientes. Caso haja um erro ao criar o socket, o servidor é finalizado.
int Socket(int family, int type, int flags) {
    int sockfd;
    if ((sockfd = socket(family, type, flags)) < 0) {
        perror("socket");
        exit(1);
    }

    return sockfd;
}

void Bind(int i, const struct sockaddr_in *a, socklen_t t) {
    if (bind(i, (struct sockaddr *) a, t) == -1) {
        perror("bind");
        exit(1);
    }
}

void Listen(int i, int j) {
    if (listen(i, j) == -1) {
        perror("listen");
        exit(1);
    }
}

int	Accept(int i, struct sockaddr * __restrict j, socklen_t * __restrict k) {
    int connfd;
    if ((connfd = accept(i, j, k)) == -1) {
        perror("accept");
        exit(1);
    }

    return connfd;
}
