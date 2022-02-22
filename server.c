/* COMPILAÇÃO:
gcc -pthread -o server server.c

EXECUÇÃO:
./server [port] */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <math.h>
#include <time.h>

#define NUM_CLIENTS 2 // Número de clientes que realizarão o cálculo
#define QTD_PONTOS 1000LL // Quantidade de pontos que serão sorteados (LL para suportar 10^10 pontos)
#define PTS_ENVIO (QTD_PONTOS/NUM_CLIENTS) // Divisão de pontos entre os clientes
#define BUFFER_SZ 2048 // Tamanho do buffer

/* Variáveis globais */
static _Atomic unsigned int cli_count = 0; // Contador de clientes que será manipulado pelas threads
static int uid = 20;
float soma_pi = 0;
int cont = 0;

/* Estrutura do cliente */
typedef struct{
	struct sockaddr_in address;
	long int sockfd; 
	int uid; // ID do cliente
	char name[32]; // Nome do cliente
} client_t;

client_t *clients[NUM_CLIENTS];

/* Inicializa um mutex estático com atributos padrão */
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Tempo */
struct timespec tempo_inicio, tempo_fim;
double tempo_decorrido;

/* Cálculo da média do PI */
void media_pi(float pi){
	soma_pi += pi;
	if(cont == NUM_CLIENTS){ // Exibe o valor final de PI, realizando a média dos valores enviados pelos clientes
		printf("\n[#]VALOR FINAL DO PI = %.8f", (soma_pi/NUM_CLIENTS)); 
		clock_gettime(CLOCK_MONOTONIC, &tempo_fim); // Finaliza contagem do tempo
		tempo_decorrido = (tempo_fim.tv_sec - tempo_inicio.tv_sec);
		tempo_decorrido += (tempo_fim.tv_nsec - tempo_inicio.tv_nsec) / 1000000000.0;
		printf("\n[#]TEMPO DE EXECUÇÃO: %lf seg", tempo_decorrido); // Exibe o tempo de execução do cálculo em segundos
		puts("\n\n[#]Cálculo realizado com sucesso!\n");
	}
}

/* Adiciona o cliente na fila */
void queue_add(client_t *cl){ 
	pthread_mutex_lock(&clients_mutex); /* Adquire a propriedade do mutex especificado */

	for(int i=0; i<NUM_CLIENTS; ++i){ // Percorre a lista de clientes para adicionar um novo cliente
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex); /* Desbloqueia o mutex especificado */
}

/* Remove cliente da fila */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<NUM_CLIENTS; ++i){ // Percorre o vetor de clientes para remover um cliente
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Envia mensagem para todos os clientes */
void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<NUM_CLIENTS; ++i){ // Percorre o vetor de clientes para enviar mensagem 
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Responsabiliza-se por toda a comunicação com o cliente */
void *handle_client(void *arg){
	char buff_out[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	// Recebe o nome do cliente
	if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){
		printf("O nome não foi inserido.\n");
		leave_flag = 1;
	} else{
		strcpy(cli->name, name);
		sprintf(buff_out, "%s conectou-se\n", cli->name);
		printf("%s", buff_out);
	}

	bzero(buff_out, BUFFER_SZ);

	/* Verifica se todos os clientes aguardados se conectaram
	e envia a quantidade de pontos para todos */
	if((cli_count) == NUM_CLIENTS){
		printf("\n[#]Número de clientes alcançado!\n[#]Enviando tarefas...\n");
		clock_gettime(CLOCK_MONOTONIC, &tempo_inicio); // Inicia contagem do tempo
		sprintf(buff_out, "%llu", PTS_ENVIO); // Coloca a quantidade de pontos no buffer
		printf("[#]Enviando %s pontos para cada cliente...\n\n", buff_out);
		send_message(buff_out, cli->sockfd); // Envia a mensagem para os clientes
	}

	bzero(buff_out, BUFFER_SZ);

	while(1){
		if (leave_flag){
			break; // Termina o processo
		}

		/* Recebe o valor de PI calculado pelo cliente */
		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0){
			if(strlen(buff_out) > 0){ // Verifica se está chegando dados
				printf("%s -> %s\n", buff_out, cli->name);
				float pi = atof(buff_out); // Converte o PI string para float
				cont++;
				media_pi(pi); // Chamada da função para cálculo da média do PI
			}
		} 
		/* Verifica se o cliente resolveu sair com "exit" */
		else if (receive == 0 || strcmp(buff_out, "exit") == 0){ // Verifica se o cliente desejou sair com "exit"
			sprintf(buff_out, "%s desconectou-se\n", cli->name); // Coloca mensagem de saída no buffer
			printf("%s", buff_out); 
			leave_flag = 1; // Sinaliza saída do cliente
		} 

		bzero(buff_out, BUFFER_SZ);
	}

  /* Remove o cliente da fila, desliga a thread e libera recursos */
	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv){
	setlocale(LC_ALL,"Portuguese");

	// Execução deve ser ./Server <port>. Ex: ./Server 5000
	if(argc != 2){
		printf("Use: %s <porta>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1"; // Endereço ip do servidor, nesse caso localhost
	int port = atoi(argv[1]); // Recebe a porta informada pelo usuário para a criação do servidor
	int option = 1;
	int listenfd = 0, connfd = 0;
  	struct sockaddr_in serv_addr; 
  	struct sockaddr_in cli_addr; 
  	pthread_t tid; //Valor que permite identificar a thread

	/* Configurações do socket */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

  	/*Atribui parâmetros de comunicação para o socket */
	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("ERROR: setsockopt failed");
    	return EXIT_FAILURE;
	}

	/* Bind */
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}

	printf("#=== SERVIDOR CRIADO - PORTA %d ===#\n", port);
	printf(">Número de clientes aguardados: %d\n", NUM_CLIENTS);
	printf(">Quantidade de pontos que serão sorteados: %llu\n\n", QTD_PONTOS);
	puts("Aguardando conexões...\n");

	/* Listen */
	if (listen(listenfd, 10) < 0) {
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}

	/* Loop infinito para o servidor aceitar um pedido de conexão de um cliente e criar a thread */
	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Configuração do cliente */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* Adiciona o cliente à fila e duplica a thread atual no SO*/
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduz o uso da CPU*/
		sleep(1);
	}

	return EXIT_SUCCESS;
}