#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N_PONTOS 1000LL // Número de pontos aleatórios que serão utilizados para o cálculo

double gera_coord();
void montecarlo_pi()

/* Gera coordenadas para o Método de Monte Carlo */
double gera_coord(){ // Utiliza somente a parte decimal, visto que 0<x,y<1
	double aux = 100.0*((double)(rand())/RAND_MAX);
	aux = aux-(int)(aux);

	return aux; // Retorna coordenada com valor entre 0 e 1
}

/* Realiza o cálculo do valor PI com o Método de Monte Carlo */
void montecarlo_pi(){
	int i = 0;
	double x = 0,y = 0; // Coordenadas
	long long int pdentro = 0,pfora = 0; // Pontos dentro e fora do círculo
	double valor_pi = 0;

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
valor_pi = 4.0*(((double)pdentro)/((double)N_PONTOS)); //calcula valor aproximado de PI

// Saída dos resultados
printf("Pontos dentro: %llu", pdentro);
printf("\nPontos fora: %llu\n", pfora);
printf("\n[#]Valor de PI calculado = %.8lf\n", valor_pi);

}

int main(){
    printf("##MÉTODO DE MONTE CARLO - CÁLCULO DE PI##\n\n");

    srand(time(NULL)); // Inicializa gerador de numeros com a semente
    clock_t start_time; // Variável para cálculo do tempo de execução
    start_time = clock(); // Inicia contagem de tempo

    montecarlo_pi(); // Chamada da função para o cálculo de PI

    double tempo = (clock() - start_time) / (double)CLOCKS_PER_SEC; // Finaliza contagem do tempo
    printf("[#]TEMPO DE EXECUÇÃO: %lf seg\n\n", tempo);
    puts("[#]Cálculo realizado com sucesso!\n");

    return 0;
}
