#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 4096

int main(int argc, char **argv) {
    // socket file descriptor
    int    sockfd;
    // byte atual (lido do server)
    int    n;
    // dados recebidos (string)
    char   recvline[MAXLINE + 1];
    char   error[MAXLINE + 1];
    // endereço de conexão do socket
    struct sockaddr_in servaddr;
    // endereço do socket local
    struct sockaddr_in localaddr;
    // tamanho do endereço do socket local
    socklen_t localaddr_len;

    // verifica a quantidade de argumentos do programa
    if (argc != 2) {
        // se o endereço do servidor não foi fornecido
        // exibe a forma correta de usar o programa na saída
        // padrão de erro
        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error," <IPaddress>");
        perror(error);
        // finaliza a execução com um código de erro
        exit(1);
    }

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
    servaddr.sin_port   = htons(13182);
    // formata / converte o endereço do servidor de string para binário
    // e atribui este valor para o campo sin_addr da variáveo servaddr
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(1);
    }

    // tenta estabelecer uma conexão com o servidor (em caso de erro, a mesma
    // é finalizada)
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(1);
    }

    // inicializa a variável localaddr_len com o tamanho da variável local_addr
    localaddr_len = sizeof(localaddr);
    // obtém o endereço do socket local
    if (getsockname(sockfd, (struct sockaddr *) &localaddr, &localaddr_len) != 0) {
        perror("error getting socket name");
        exit(1);
    }
    printf("getsocketname: %s:%d\n", inet_ntoa(localaddr.sin_addr), ntohs(localaddr.sin_port));

    // lê "chunks" (pedaços) dos dados recebidos do servidor
    // repete a leitura até que não há nada mais para receber ou que
    // um erro aconteça
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;
        // imprime o "chunk" recebido na saída padrão
        if (fputs(recvline, stdout) == EOF) {
            perror("fputs error");
            exit(1);
        }
    }

    // se ocorreu algum erro na leitura, finaliza a execução
    if (n < 0) {
        perror("read error");
        exit(1);
    }

    exit(0);
}
