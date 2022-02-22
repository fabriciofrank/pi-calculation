/* COMPILAÇÃO:
gcc -pthread -o client client.c

EXECUÇÃO:
./server [port] */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define LENGTH 2048 // Tamanho do buffer

/* Variáveis globais */
volatile sig_atomic_t flag = 0;
long int sockfd = 0;
char name[32]; // Nome do cliente



/* Gera coordenadas para o Método de Monte Carlo */
double gera_coord(){ // Utiliza somente a parte decimal, visto que 0<x,y<1
	double aux = 100.0*((double)(rand())/RAND_MAX);
	aux = aux-(int)(aux);

	return aux; // Retorna coordenada com valor entre 0 e 1
}

/* Realiza o cálculo do valor PI com o Método de Monte Carlo */
double montecarlo_pi(long long int N_PONTOS){
	double x = 0, y = 0; // Coordenadas
	long long int i = 0, pdentro = 0, pfora = 0; // Pontos dentro e fora do círculo
	double valor_pi = 0; // Valor do PI que será retornado

	puts("\n>Calculando valor de PI pelo Método de Monte Carlo...");

	for(i=0; i<N_PONTOS; i++){
		x = gera_coord(); // Gera coordenada x do ponto
		y = gera_coord(); // Gera coordenada y do ponto

		//printf("P(%d) x=%.15f  y=%.15f \n", i,x,y); // Print auxiliar para verificar geração das coordenadas
		
		if((x*x + y*y) <= 1) // Soma os quadrados das coordenadas
			pdentro++; // Se a soma dos quadrados <= 1, ponto caiu dentro
		else
			pfora++; // Se a soma dos quadrados > 1, ponto caiu fora
	}

	// Cálculo do PI
	valor_pi = 4.0*(((double)pdentro)/((double)N_PONTOS));

	// Saída dos resultados
	printf("\nPontos dentro: %llu", pdentro);
	printf("\nPontos fora: %llu", pfora);
	printf("\n[#]Valor de PI calculado = %.8lf", valor_pi);
	puts("\n\n[#]Cálculo realizado com sucesso!\n");

	return valor_pi; // Retorna o valor de PI que será enviado para o servidor
}

/* Insere o terminador de string \0 */
void str_trim_lf (char* arr, int length) {
	int i;
	for (i=0; i<length; i++) { 
		if (arr[i] == '\n') {
			arr[i] = '\0';
			break;
		}
	}
}

// Ctrl+C para sair do programa
void catch_ctrl_c_and_exit (int sig){
    flag = 1;
}

/* Responsabiliza-se pelo envio de mensagens*/
void send_msg_handler(){
	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

  	while(1){
		fflush(stdout);
		fgets(message, LENGTH, stdin); // Recebe entrada do usuário
		str_trim_lf(message, LENGTH);

		if (strcmp(message, "exit") == 0){ // Verifica se o usuário desejou sair com "exit"
			break;
		} 
		
		bzero(message, LENGTH);
		bzero(buffer, LENGTH + 32);
	}
  catch_ctrl_c_and_exit(2);
}

/* Responsabiliza-se pelo recebimento de mensagens */
void recv_msg_handler() {
	char message[LENGTH] = {};
	double pi; // Variável para armazenar o valor de PI
  	while (1){
		long int receive = recv(sockfd, message, LENGTH, 0); //Recebe a mensagem enviada pelo servidor
		long long int QTD_PONTOS = atol(message); // Recebe a quantidade de pontos e converte para long int
		if (QTD_PONTOS != 0){
			printf("Quantidade de pontos recebida: %llu\n", QTD_PONTOS);
			pi = montecarlo_pi(QTD_PONTOS); // Chama a função que calcula o PI pelo Método de Monte Carlo
  			sprintf(message, "%.8lf", pi); // Coloca o valor do PI no buffer
			send(sockfd, message, 32, 0); // Envia o valor de PI para o servidor
			printf("[#]Valor de PI enviado ao servidor.\n");
			break;
   		}
		else if (receive == 0) {
			break;
    	} 
		memset(message, 0, sizeof(message));
  	}
}

int main(int argc, char **argv){
	setlocale(LC_ALL,"Portuguese");

	// Execução deve ser ./Client <port>. Ex: ./Client 5000
	if(argc != 2){
		printf("Use: %s <porta>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1"; // Endereço ip do cliente, nesse caso localhost
	int port = atoi(argv[1]); // Recebe a porta informada pelo usuário para conexão com o servidor

	/* Recebe o nome do cliente do usuário */
	printf(">Nome do cliente: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, sizeof(name));


  	/* Verifica se o nome possui entre 2 e 32 caracteres*/
	if (strlen(name) > 32 || strlen(name) < 2){
		printf("O nome deve ter mais de 2 e menos de 30 caracteres.\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr; 

	/* Configurações do socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  	server_addr.sin_family = AF_INET;
  	server_addr.sin_addr.s_addr = inet_addr(ip);
  	server_addr.sin_port = htons(port);


	/* Conecta-se ao servidor */
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1) {
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	/* Envia o nome para o servidor */
	send(sockfd, name, 32, 0);

	printf("#=== CONECTADO AO SERVIDOR ===#\n");
	srand(time(NULL)); // Inicializa gerador de números com semente aleatória para geração de coordenadas

	// Criação da thread para o envio de mensagens
	pthread_t send_msg_thread; 
  	if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
    	return EXIT_FAILURE;
	}

	// Criação da thread para o recebimento de mensagens
	pthread_t recv_msg_thread; 
  	if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1){
		if(flag){
			printf("\nBye\n"); // Mensagem de saída para "exit"
			break;
    	}
	}

	close(sockfd);

	return EXIT_SUCCESS;
}