#include <stdio.h>
#include <stdlib.h>
#include "lib.h"

#define MEM_TAM 4096
#define NUM_REGS 8

#define TABELA_TAM 32   // TAMANHO TABELA PREDITOR

// MEMORIAS
uint16_t memoria[MEM_TAM];
// uint16_t memoriaInstrucao[MEM_TAM];

int ciclo = 0;
uint16_t PC = 1;


// Banco de Registradores
uint16_t regs[NUM_REGS];


typedef struct{
    uint16_t formato;
    uint16_t opcode;
    uint16_t destino;
    uint16_t reg1;
    uint16_t reg2;
	uint16_t reg;
	uint16_t imediato;
} Instrucao;


typedef struct{
	uint16_t existe;
	uint16_t endereco;
	uint16_t confianca;
	uint16_t alvo;
} Preditor;

Preditor tabela[TABELA_TAM]; // Tabela que armazena as instruções para o preditor

typedef struct{
	uint16_t existe;
	uint16_t pc;
	uint16_t proximoEnd;
	uint16_t instrTotal; 
	Instrucao instr;
} Intermediario;                        // Struct que carrega as informações entre busca -> decode e decode -> Execução/Writeback

uint16_t prever(uint16_t pc){
	Preditor *e = &tabela[pc & 0b11111];

	if(e->existe == 1 && e->endereco == pc && e->confianca >= 2){       // se a mesma instrucao ja existe e grande chance de pular, retorna o alvo do pulo 
		return e->alvo;
	}
	else{                                         // se nao, retorna o proximo endereco da memoria;
		return pc+1;
	}
}

void atualizarTabela(uint16_t pc, uint16_t pulou, uint16_t alvoJump){
	Preditor *e = &tabela[pc & 0b11111];

	if(!e->existe || e->endereco != pc){      // significa que a instrucao ainda nao existe na tabela, ve se existe alguma com os  
		e->existe = 1;
		e->endereco = pc;

		if(pulou == 1){                     // se pulou, o preditor ja comeca em 1, se nao em 0
			e->confianca = 1;
		}else{
			e->confianca = 0;
		}
	}
	else{
		if(pulou == 1 && e->confianca < 3){                // se a instrucao existe na tabela aumenta ou diminui a confianca no intervalo de 0 a 3
			e->confianca++;
		}else if (pulou == 0 && e->confianca > 0){
			e->confianca--;
		}
	}

	e->alvo = alvoJump;
}


void decode(uint16_t instrucao, Instrucao *i){
	*i = (Instrucao){0};
    i->formato = (instrucao >> 15);
	if(i->formato == 0){
		i->opcode = (instrucao >> 9) & 0b111111;
		i->destino = (instrucao >> 6) & 0b111;
		i->reg1 = (instrucao >> 3) & 0b111;
		i->reg2 = instrucao & 0b111;
	}
	else if(i->formato == 1){
		i->opcode = (instrucao >> 13) & 0b11;
		i->reg = (instrucao >> 10) & 0b111;
		i->imediato = instrucao & 0b1111111111;
	}
}

void writeBack(Instrucao *codigos, int ULA, int ciclo){
	if(codigos->formato == 0 && codigos->opcode <= 5){    // iNSTRUCOES ARTIMEDICAS
		regs[codigos->destino] = ULA;
	}
	else if(codigos->formato == 0 && codigos->opcode == 15){   //LOAD
		regs[codigos->destino] = memoria[regs[codigos->reg1]];
	}
	else if(codigos->formato == 0 && codigos->opcode == 16){   // STORE
		memoria[regs[codigos->reg1]] = regs[codigos->reg2];
	}else if(codigos->formato == 1 && codigos->opcode == 3){
		regs[codigos->reg] = codigos->imediato;   //   IMEDIATO MOVE
	}

}

int execucao(Instrucao *codigos, int ciclo, uint16_t *proximo, uint16_t pcInstr){

	int ULA = 0;
	*proximo = pcInstr + 1;        // Criação de um ponteiro para modificar o valor na memória, ja que só é possível 1 return
	if(codigos->formato == 0){
		switch (codigos->opcode){

		case 0 : // 0    ADD
			ULA = regs[codigos->reg1] + regs[codigos->reg2];
			printf("Ciclo %d: ADD -> R%d = R%d(%d) + R%d(%d) = %d\n", ciclo, codigos->destino, codigos->reg1, regs[codigos->reg1], codigos->reg2, regs[codigos->reg2], ULA);			
			break;

		case 1 : //      SUB
			ULA = regs[codigos->reg1] - regs[codigos->reg2];
			printf("Ciclo %d: SUB -> R%d = R%d(%d) - R%d(%d) = %d\n", ciclo, codigos->destino, codigos->reg1, regs[codigos->reg1], codigos->reg2, regs[codigos->reg2], ULA);
			break;

		case 2 : //     MUL
			ULA = regs[codigos->reg1] * regs[codigos->reg2];
			printf("Ciclo %d: MUL -> R%d = R%d(%d) * R%d(%d) = %d\n", ciclo, codigos->destino, codigos->reg1, regs[codigos->reg1], codigos->reg2, regs[codigos->reg2], ULA);
			break;

		case 3 : //     DIV
			if (regs[codigos->reg2] == 0) {
				printf("Erro Ciclo %d: divisao por zero.\n", ciclo);
				exit(0);
			}
			ULA = regs[codigos->reg1] / regs[codigos->reg2];
			printf("Ciclo %d: DIV -> R%d = R%d(%d) / R%d(%d) = %d\n", ciclo, codigos->destino, codigos->reg1, regs[codigos->reg1], codigos->reg2, regs[codigos->reg2], ULA);
			break;
		
		case 4 : // CMP_EQUAL
			if(regs[codigos->reg1] == regs[codigos->reg2]){
				ULA = 1;
			}else{
				ULA = 0;
			}
			printf("Ciclo %d: CMP EQUAL -> R%d = (R%d == R%d) = %d\n", ciclo, codigos->destino, codigos->reg1, codigos->reg2, ULA);
			break;

		case 5 : // CMP_NEQ
			if(regs[codigos->reg1] != regs[codigos->reg2]){
				ULA = 1;
			}
			else {
				ULA = 0;
			}
			printf("Ciclo %d: CMP NEQ -> R%d = R%d != R%d = %d\n", ciclo, codigos->destino, codigos->reg1, codigos->reg2, ULA);
			break;

		case 15 : //   LOAD
			if (regs[codigos->reg1] >= MEM_TAM) {
     		   printf("Erro Ciclo %d: acesso invalido a memoria (endereco %d)\n", ciclo, regs[codigos->reg1]);
        		exit(1);
    		}
			printf("Ciclo %d: LOAD -> R%d = R%d(%d)\n", ciclo, codigos->destino, codigos->reg1, memoria[regs[codigos->reg1]]);
			break;
		
		case 16 : //    STORE
    	if (regs[codigos->reg1] >= MEM_TAM) {
        	printf("Erro Ciclo %d: acesso invalido a memoria (endereco %d)\n", ciclo, regs[codigos->reg1]);
        	exit(1);
    	}
			printf("Ciclo %d: STORE -> Mem[%d] = R%d(%d)\n", ciclo, regs[codigos->reg1], codigos->reg2, regs[codigos->reg2]);
			break;
		
		case 63 : // 63   syscall END
			if(regs[0] == 0){
				printf("Encerrando processador...\n");
				exit(0);
			}
			printf("R0 != 0, Syscall inválido\n");
			exit(0);
			
		
		default:
			printf("Instrucao desconhecida, Ciclo %d\n", ciclo);
			printf("Encerrando processador...\n");
			exit(0);
		}
	}
	else if(codigos->formato == 1){
		switch (codigos->opcode){

		case 0 : // 0    JUMP
			*proximo = codigos->imediato;
			printf("Ciclo %d: JUMP -> Pulando para o endereco: %d\n", ciclo, *proximo);
			atualizarTabela(pcInstr, 1, codigos->imediato);
			break;

		case 1 : // 1     JUMP COND
		
			if(regs[codigos->reg] == 1){       // se reg =1 ( Siginifica que pulou)
				*proximo = codigos->imediato;
				printf("Ciclo %d: JUMP COND -> Pulando para o endereco: %d\n", ciclo, *proximo);
			}else{
				printf("Ciclo %d: JUMP COND NAO REALIZADO \n", ciclo);
			}
			atualizarTabela(pcInstr, regs[codigos->reg] == 1 ,codigos->imediato);
			break;

		case 3 : // 3    MOV
			printf("Ciclo %d: MOV -> R%d <- %d\n", ciclo, codigos->reg, codigos->imediato);
			break;

		default:
			printf("Instrucao desconhecida, Ciclo %d\n", ciclo);
			exit(0);
			}
	}
	return ULA;

}

void iniciar(){
	while(PC < MEM_TAM){

	}
}

int main (int argc, char **argv){
	if (argc != 2){
		printf("usage: %s [bin_name]\n", argv[0]);
		exit(1);
	}

	load_binary_to_memory(argv[1], memoria, MEM_TAM);

	iniciar();
}