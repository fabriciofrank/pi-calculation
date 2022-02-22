/* COMPILAÇÃO:
mpicc pi_mpi.c -o pi_mpi
*/

/* EXECUÇÃO:
mpirun -np [numero_processos] pi_mpi [numero_pontos]
OU
mpirun --oversubscribe -np [numero_processos] pi_mpi [numero_pontos] */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <mpi.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

double gera_coord();
double montecarlo_pi(long long int, int, int);

/* Gera coordenadas para o Método de Monte Carlo */
double gera_coord(){ // Utiliza somente a parte decimal, visto que 0<x,y<1
    double aux = 100.0*((double)(rand())/RAND_MAX);
    aux = aux-(int)(aux);

    return aux; // Retorna coordenada com valor entre 0 e 1
}

/* Realiza o cálculo do valor PI com o Método de Monte Carlo */
double montecarlo_pi(long long int N_PONTOS, int rank, int size){
    printf("Processo %d de %d, pontos sorteados: %llu", rank+1, size, N_PONTOS);

    double x = 0, y = 0; // Coordenadas
    long long int i = 0, pdentro = 0, pfora = 0; // Pontos dentro e fora do círculo
    double valor_pi = 0; // Valor do PI que será retornado

    for(i=0; i<N_PONTOS; i++){
        x = gera_coord(); // Gera coordenada x do ponto
        y = gera_coord(); // Gera coordenada y do ponto
        
        if((x*x + y*y) <= 1) // Soma os quadrados das coordenadas
            pdentro++; // Se a soma dos quadrados <= 1, ponto caiu dentro
        else
            pfora++; // Se a soma dos quadrados > 1, ponto caiu fora
    }

    // Cálculo do PI
    valor_pi = 4.0*(((double)pdentro)/((double)N_PONTOS));

    return valor_pi; // Retorna o valor de PI 
}

int main(int argc, char *argv[]){
    setlocale(LC_ALL,"Portuguese");

    long long int n_pontos; // Quantidade de pontos que serão sorteados

    int rank, // Identificador de processo
        size, // Número de processos
        namelen; // Comprimento (em caracteres) do nome do processador

    double pi_local, // Valor de PI de cada processo
           soma_pi, // Valor de PI somado (soma de todos os PI calculados pelos processos)
           media_pi, // Média do PI (valor resultante)
           tempo_inicio, // Tempo inicial
           tempo_fim, // Tempo final
           tempo_decorrido; // Diferença entre o tempo final e inicial

    char processor_name[MPI_MAX_PROCESSOR_NAME]; // Nome do processador

    MPI_Init(&argc, &argv); // Inicialização dos processos MPI
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Determinação do número de processos no comunicador
    tempo_inicio = MPI_Wtime(); // Inicia contagem do tempo
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Identifica o processo dentro do grupo de processos iniciados pelo comunicador
    MPI_Get_processor_name(processor_name, &namelen); // Obtém o nome do processador

    /* Apenas o processo 0 conhece o número de pontos e o tempo execução */
    if (rank == 0){
        n_pontos = atol(argv[1]); // Atribui o número de pontos a serem sorteados à variável n_pontos
        puts("#=== CÁLCULO DE PI COM MPI - MÉTODO DE MONTE CARLO ===#");
        fprintf(stdout,"#Processador: %s\n", processor_name); // Imprime o nome do processador
        printf("#Quantidade total de pontos que serão sorteados: %lld\n\n", n_pontos); // Imprime o número total de pontos
        puts("Calculando...\n");
    }

    srand(time(NULL) + rank); // Inicializa gerador de números com semente aleatória para geração de coordenadas (Monte Carlo)

    /* O processo 0 distribui para o resto dos processos o número de iterações que calcularemos para a estimativa de PI */
    MPI_Bcast(&n_pontos, // Ponteiro para os dados que serão enviados
               1, // Número de dados para os quais o ponteiro aponta
               MPI_LONG_LONG_INT, // Tipo de dado que será enviado (long long int)
               0, // Identificação do processo que envia os dados
               MPI_COMM_WORLD);
     
    /* Encerra caso quantidade de pontos <= 0 */
    if (n_pontos <= 0){
        puts("Insira uma quantidade positiva de pontos!");
        MPI_Finalize();
    } else{
        // Cálculo de PI (para cada processo)
        pi_local = montecarlo_pi(n_pontos/size, rank, size); // Chama a função que calcula o PI pelo Método de Monte Carlo
        printf(" - PI calculado = %.8f\n", pi_local); // Imprime o valor de PI calculado para cada processo
    }

    /* Todos os processos agora compartilham seu valor de PI local */ 
    MPI_Reduce(&pi_local, // Valor local de PI
               &soma_pi, // Valor resultante de PI
               1, // Número de dados que serão reduzidos
               MPI_DOUBLE, // Tipo de dado que será reduzido
               MPI_SUM, // Operação que será aplicada 
               0, // Processo que receberá os dados reduzidos
               MPI_COMM_WORLD);

    /* Apenas o processo 0 imprime a mensagem com o valor resultante de PI e o tempo de execução, 
    pois é o único que conhece esses valores */
    if (rank == 0){
        tempo_fim = MPI_Wtime(); // Finaliza contagem do tempo
        media_pi = soma_pi/size; // Calcula a média dos valores de PI calculados pelos processos
        tempo_decorrido = tempo_fim - tempo_inicio; // Calcula o tempo decorrido
        sleep(1); // Sleep para mostrar os resultados somente ao final
        // Exibe o valor final de PI e o tempo de execução em segundos
        puts("\n[#]Cálculo realizado com sucesso!");
        printf("\n[#]VALOR FINAL DO PI (média) = %.8f\n", media_pi); 
        printf("[#]TEMPO DE EXECUÇÃO (em segundos): %lf\n\n", tempo_decorrido);
    }

    MPI_Finalize();
}