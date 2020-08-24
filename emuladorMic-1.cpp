//São 9 funções para serem implementadas no total;
//PROGRESSO: 8/9;

//ANOTAÇÕES:
//decoded_microcode implementada! (02/06/2020)
//shift implementada! (03/06/2020)
//read_registers implementada! (03/06/2020)
//write_register implementada! (03/06/2020)
//ALU implementada! (04/06/2020)
//mainmemory_io implementada! (05/06/2020)
//next_address em desenvolvimento (05/06/2020)
//next_address implementada! (06/06/2020)
//load_microprog implementada! (06/06/2020)
//main implementada! (08/06/2020)
//alu implementada de forma diferente e correção de bugs (10/06/2020)
//mainmemory_io implementada de forma diferente (11/06/2020)
//função debug modificada (15/06/2020)

//TESTES DE FUNÇÕES:
//load_microprog ok (08/06/2020)
//decode_microcode ok (08/06/2020)

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <math.h>

using namespace std;

//definições de tipos
typedef unsigned char byte; //8 bits
typedef unsigned int  word; //32 bits
typedef unsigned long microcode; //36 bits

//estrutura para guardar uma microinstrução decodificada
struct decoded_microcode //microcódigo separado por partes
{
    word nadd; //next address
    byte jam;
    byte sft; //deve ser o deslocador
    byte alu; 
    word reg_w; //barramento C
    byte mem;
    byte reg_r; //barramento B
};

//Funções utilitárias ======================
void write_microcode(microcode w) //Dado uma microinstrucao, exibe na tela devidamente espaçado pelas suas partes.
{
   unsigned int v[36];
   for(int i = 35; i >= 0; i--)
   {
       v[i] = (w & 1);
       w = w >> 1;
   }

   for(int i = 0; i < 36; i++)
   {
       cout << v[i];
       if(i == 8 || i == 11 || i == 13 || i == 19 || i == 28 || i == 31) cout << " ";
   }
}

void write_word(word w) //Dada uma palavra (valor de 32 bits / 4 bytes), exibe o valor binário correspondente.
{
   unsigned int v[32];
   for(int i = 31; i >= 0; i--)
   {
       v[i] = (w & 1);
       w = w >> 1;
   }

   for(int i = 0; i < 32; i++)
       cout << v[i];
}

void write_byte(byte b) //Dado um byte (valor de 8 bits), exibe o valor binário correspondente na tela.
{
   unsigned int v[8];
   for(int i = 7; i >= 0; i--)
   {
       v[i] = (b & 1);
       b = b >> 1;
   }

   for(int i = 0; i < 8; i++)
       cout << v[i];
}

void write_dec(word d) //Dada uma palavra (valor de 32 bits / 4 bytes), exibe o valor decimal correspondente.
{
   cout << (int)d;// << endl;
}
//=========================================

//sinalizador para desligar máquina
bool halt = false;

//memoria principal
#define MEM_SIZE 0xFFFF+1 //0xFFFF + 0x1; // 64 KBytes = 64 x 1024 Bytes = 65536 (0xFFFF+1) x 1 Byte;
byte memory[MEM_SIZE]; //0x0000 a 0xFFFF (0 a 65535)

//registradores de controle de memória
word mar=0, mdr=0, pc=0; 
byte mbr=0;
//MAR guarda o endereço de uma palavra de memória, a combinação MAR/MDR é usada para ler e escrever palavras de dados
//PC guarda endereços de bytes, a combinação PC/MBR é usada pra ler um programa executável que consiste em uma sequência de bytes

//registradores normais
word sp=0, lv=0, cpp=0, tos=0, opc=0, h=0;

//barramentos
word bus_a=0, bus_b=0, bus_c=0, alu_out=0;

//estado da ALU para salto condicional
byte n=0, z=1;

//registradores de microprograma
word mpc;

//memória de microprograma: 512 x 64 bits = 512 x 8 bytes = 4096 bytes = 4 KBytes.
//Cada microinstrução é armazenada em 8 bytes (64 bits), mas apenas os 4,5 bytes (36 bits) de ordem mais baixa são de fato decodificados.
//Os 28 bits restantes em cada posição da memória são ignorados, mas podem ser utilizados para futuras melhorias nas microinstruções para controlar microarquiteturas mais complexas.
microcode microprog[512];

//carrega microprograma
//Escreve um microprograma de controle na memória de controle (array microprog, declarado logo acima)
void load_microprog()
{
    FILE *microprograma;
	microprograma = fopen("microprog.rom", "rb");

	if (microprograma != NULL) {
		fread(microprog, sizeof(microcode), 512, microprograma);
		fclose(microprograma);

	}
}

//carrega programa na memória principal para ser executado pelo emulador.
//programa escrito em linguagem de máquina (binário) direto na memória principal (array memory declarado mais acima).
void load_prog()
{
    FILE *prog;
	word tamanho;
	byte tam_arquivo[4];

	prog = fopen("soma.exe", "rb");
	
	if (prog != NULL) {
		//Carrega os primeiros 4 bytes que contém o tamanho do arquivo para um vetor e depois carrega esse vetor na variável tamanho.
		fread(tam_arquivo, sizeof(byte), 4, prog);
		memcpy(&tamanho, tam_arquivo, 4);

		//Carrega os 20 primeiros bytes que contém a inicialização do programa para os primeiros 20 bytes da memória
		fread(memory, sizeof(byte), 20, prog);

		//Carrega o programa na memória a partir da posição PC
		fread(&memory[0x0401], sizeof(byte), tamanho-20, prog);
	
		fclose(prog);

	}

}

//exibe estado da máquina
void debug(bool clr = true)
{
    if(clr) system("clear");

    cout << "Microinstrução: ";
    write_microcode(microprog[mpc]);

    cout << "\n\nMemória principal: \nPilha: \n";
    for(int i = lv*4; i <= sp*4; i+=4)
    {
        write_byte(memory[i+3]);
        cout << " ";
        write_byte(memory[i+2]);
        cout << " ";
        write_byte(memory[i+1]);
        cout << " ";
        write_byte(memory[i]);
        cout << " : ";
        if(i < 10) cout << " ";
        //cout << i << " | " << memory[i+3] << " " << memory[i+2] << " " << memory[i+1] << " " << memory[i];
        cout << i << " | "; 
        write_dec(memory[i+3]);
        cout << " "; 
        write_dec(memory[i+2]);
        cout << " ";
        write_dec(memory[i+1]);
        cout << " "; 
        write_dec(memory[i]);
        word w;
        memcpy(&w, &memory[i], 4);
        cout << " | " << i/4 << " : " << w << endl;
    }

    cout << "\n\nPC: \n";
    for(int i = (pc-1); i <= pc+20; i+=4)
    {
        write_byte(memory[i+3]);
        cout << " ";
        write_byte(memory[i+2]);
        cout << " ";
        write_byte(memory[i+1]);
        cout << " ";
        write_byte(memory[i]);
        cout << " : ";
        if(i < 10) cout << " ";
        //cout << i << " | " << memory[i+3] << " " << memory[i+2] << " " << memory[i+1] << " " << memory[i];
        cout << i << " | "; 
        write_dec(memory[i+3]);
        cout << " "; 
        write_dec(memory[i+2]);
        cout << " ";
        write_dec(memory[i+1]);
        cout << " "; 
        write_dec(memory[i]);
        word w;
        memcpy(&w, &memory[i], 4);
        cout << " | " << i/4 << " : " << w << endl;
    }

    cout << "\nRegistradores - \nMAR: " << mar << " ("; write_word(mar);
    cout << ") \nMDR: " << mdr << " ("; write_word(mdr);
    cout << ") \nPC : " << pc << " ("; write_word(pc);
    cout << ") \nMBR: " << (int) mbr << " ("; write_byte(mbr);
    cout << ") \nSP : " << sp << " (";  write_word(sp);
    cout << ") \nLV : " << lv << " ("; write_word(lv);
    cout << ") \nCPP: " << cpp << " ("; write_word(cpp);
    cout << ") \nTOS: " << tos << " ("; write_word(tos);
    cout << ") \nOPC: " << opc << " ("; write_word(opc);
    cout << ") \nH  : " << h << " ("; write_word(h);
    cout << ")" << endl;
}

decoded_microcode decode_microcode(microcode code) //Recebe uma microinstrução binária e separa suas partes preenchendo uma estrutura de microinstrução decodificada, retornando-a.
{
    decoded_microcode dec;
    //dec.nadd = (code & 0b111111111000000000000000000000000000) >> 27;
    //                   0b000000000111000000000000000000000000
    //                   0b000000000000110000000000000000000000
    //                   0b000000000000001111110000000000000000
    //                   0b000000000000000000001111111110000000
    //                   0b000000000000000000000000000001110000
    //                   0b000000000000000000000000000000001111
    dec.nadd = (code & 0xff8000000) >> 27;
    dec.jam = (code & 0x7000000) >> 24;
    dec.sft = (code & 0xc00000) >> 22;
    dec.alu = (code & 0x3f0000) >> 16;
    dec.reg_w = (code & 0xff80) >> 7;
    dec.mem = (code & 0x70) >> 4;
    dec.reg_r = (code & 0xf);

    return dec;
}

//alu
//recebe uma operação de alu binária representada em um byte (ignora-se os 2 bits de mais alta ordem, pois a operação é representada em 6 bits)
//e duas palavras (as duas entradas da alu), carregando no barramento alu_out o resultado da respectiva operação aplicada é as duas palavras.
void alu(byte func, word a, word b){
	switch(func){
		//Cada operação da ULA é representado pela sequencia dos bits de operação. Cada operação útil foi convertida para inteiro para facilitar a escrita do switch
		case 12: alu_out = a & b;		break;
		case 17: alu_out = 1;			break;
		case 18: alu_out = -1;			break;
		case 20: alu_out = b;			break;
		case 24: alu_out = a;			break;
		case 26: alu_out = ~a;			break;
		case 28: alu_out = a | b;		break;
		case 44: alu_out = ~b;			break;
		case 53: alu_out = b + 1;		break;
		case 54: alu_out = b - 1;		break;
		case 57: alu_out = a + 1;		break;
		case 59: alu_out = -a;			break;
		case 60: alu_out = a + b;		break;
		case 61: alu_out = a + a + 1;	break;
		case 63: alu_out = b - a;		break;

		default: break;
	}
	
	//Verifica o resultado da ula e atribui as variáveis zero e nzero
	
	if(alu_out) { //Se bus_C for diferente de zero
		z = 0;
		n = 1;
	} else { //Se bus_c for igual a zero
		z = 1;
		n = 0;
	}
}

//Deslocamento. Recebe a instrução binária de deslocamento representada em um byte (ignora-se os 6 bits de mais alta ordem(mais a esquerda), pois o deslocador eh controlado por 2 bits apenas)
//e uma palavra (a entrada do deslocador) e coloca o resultado no barramento bus_c.
void shift(byte s, word w)
{
    //A entrada 11 não é válida;
    //01 é pra deslocar 1 bit a direita e 10 é pra deslocar 1 byte a esquerda;
    //byte aux = s & 0b11;
    //Testa se a entrada é 01;
    if(s & 0b01 == 1){
        //bus_c = w >> 1;
        bus_c = w << 8;
    }
    else{
        //aux = aux >> 1;
        //Testa se a entrada é 10;
        if(s & 0b10 == 1){
            //bus_c = w << 8;
            bus_c = w >> 1;
        }
        //Testa se a entrada é 00;
        else{
            bus_c = w;
        }
    }
}

//Leitura de registradores. Recebe o número do registrador a ser lido (0 = mdr, 1 = pc, 2 = mbr, 3 = mbru, ..., 8 = opc) representado em um byte,
//carregando o barramento bus_b (entrada b da ALU) com o valor do respectivo registrador e o barramento bus_a (entrada A da ALU) com o valor do registrador h.
void read_registers(byte reg_end)
{
    //Testes para saber de qual registrador o valor vai ser lido, de acordo com a numeração deles;
    //0 = mdr, 1 = pc, 2 = mbr, 3 = mbru, 4 = sp, 5 = lv, 6 = cpp, 7 = tos, 8 = opc;
    if (reg_end % 2 == 0){
        if(reg_end == 0){
            bus_b = mdr;
        }
        else if(reg_end == 2){
            bus_b = mbr;
        }
        else if(reg_end == 4){
            bus_b = sp;
        }
        else if(reg_end == 6){
            bus_b = cpp;
        }
        else{
            bus_b = opc;
        }
    }
    else{
        if(reg_end == 1){
            bus_b = pc;
        }
        else if(reg_end == 3){
            if(mbr & 0b10000000){
                bus_b = mbr + 0b111111111111111111111111 << 8;
            }
        }
        else if(reg_end == 5){
            bus_b = lv;
        }
        else{
            bus_b = tos;
        }
    }
    bus_a = h;
}

//Escrita de registradores. Recebe uma palavra (valor de 32 bits) cujos 9 bits de ordem mais baixa indicam quais dos 9 registradores que
//podem ser escritos receberão o valor que está no barramento bus_c (saída do deslocador).
void write_register(word reg_end)
{
    //testes para saber quais registradores vão ser escritos;
    word aux = reg_end & 0x1ff;
    if(aux & 1 == 1){
        mar = bus_c;
    }
    aux = aux >> 1;
    if(aux & 1 == 1){
        mdr = bus_c;
    }
    aux = aux >> 1;
    if(aux & 1 == 1){
        pc = bus_c;
    }
    aux = aux >> 1;
    if(aux & 1 == 1){
        sp = bus_c;
    }
    aux = aux >> 1;
    if(aux & 1 == 1){
        lv = bus_c;
    }
    aux = aux >> 1;
    if(aux & 1 == 1){
        cpp = bus_c;
    }
    aux = aux >> 1;
    if(aux & 1 == 1){
        tos = bus_c;
    }
    aux = aux >> 1;
    if(aux & 1 == 1){
        opc = bus_c;
    }
    aux = aux >> 1;
    if(aux & 1 == 1){
        h = bus_c;
    }
}

//Leitura e escrita de memória. Recebe em um byte o comando de memória codificado nos 3 bits de mais baixa ordem (fetch, read e write, podendo executar qualquer conjunto dessas três operações ao
//mesmo tempo, sempre nessa ordem) e executa a respectiva operação na memória principal.
//fetch: lê um byte da memória principal no endereço constando em PC para o registrador MBR. Endereçamento por byte.
//write e read: escreve e lê uma PALAVRA na memória principal (ou seja, 4 bytes em sequência) no endereço constando em MAR com valor no registrador MDR. Nesse caso, o endereçamento é dado em palavras.
//Mas, como a memoria principal eh um array de bytes, deve-se fazer a conversão do endereçamento de palavra para byte (por exemplo, a palavra com endereço 4 corresponde aos bytes 16, 17, 18 e 19).
//Lembrando que esta é uma máquina "little endian", isto é, os bits menos significativos são os de posições mais baixas.
//No exemplo dado, suponha os bytes:
//16 = 00110011
//17 = 11100011
//18 = 10101010
//19 = 01010101
//Tais bytes correspondem à  palavra 01010101101010101110001100110011
//é necessário armazenar na ordem inversa, nesse exemplo, primeiro 19, depois 18, 17 e por ultimo 16;

void mainmemory_io(byte control){
	if(control & 0b001) mbr = memory[pc];					//FEATCH
	//MDR recebe os 4 bytes referente a palavra MAR 
	if(control & 0b010) memcpy(&mdr, &memory[mar*4], 4);	//READ
	//Os 4 bytes na memória da palavra MAR recebem o valor de MDR
	if(control & 0b100) memcpy(&memory[mar*4], &mdr, 4);	//WRITE

}

//Define próxima microinstrução a ser executada. Recebe o endereço da próxima instrução a ser executada codificado em uma palavra (considera-se, portanto, apenas os 9 bits menos significativos)
//e um modificador (regra de salto) codificado em um byte (considera-se, portanto, apenas os 3 bits menos significativos, ou seja JAMZ (bit 0), JAMN (bit 1) e JMPC (bit 2)), construindo e
//retornando o endereço definitivo (codificado em uma word - desconsidera-se os 21 bits mais significativos (são zerados)) da próxima microinstrução.
word next_address(word next, byte jam)
{ 
    //jmpc jamn jamz
    //z = 1 se  saída da alu for igual a 0, n = 1 se a saída da alu for diferente de 0;
    //se jamz = 1, então é verificado se a saída da alu é igual a 0 (na verdade, se z = 1). Se sim, é adicionado o bit 1 ao bit mais significativo do next adress;
    //da mesma forma, se jamn = 1, é verificado se a saída da alu é diferente de 0 (na verdade, se n = 1). Se sim, é adicionado um bit 1 ao bit mais significativo do next adress;
    //se jmpc = 1, então é feito um OR bit a bit entre os i bits de mais baixa ordem de next adress e de mbr;

    //caso jam = 000, o endereço será o próprio next;
    if(jam & 0b111 == 0){
        return next;
    }
    else{
        //verificando se jamz = 1;
        if(jam & 1 == 1){
            if(z == 1){
                next += 0b100000000;
            }
        }
        //verificando se jamn = 1;
        if(jam >> 1 == 1){
            if(n == 1){
                next += 0b100000000;
            }
        }
        //verificando se jmpc = 1;
        if(jam >> 2 == 1){
            ///cout << "jam!";
            next += mbr;
        }
    }
    return next;
}

int main(int argc, char* argv[])
{
    load_microprog(); //carrega microprograma de controle

    load_prog(); //carrega programa na memória principal a ser executado pelo emulador. Já que não temos entrada e saída, jogamos o programa direto na memória ao executar o emulador.

    decoded_microcode decmcode;

    //laço principal de execução do emulador. Cada passo no laço corresponde a um pulso do clock.
    //o debug mostra o estado interno do processador, exibindo valores dos registradores e da memória principal.
    //o getchar serve para controlar a execução de cada pulso pelo clique de uma tecla no teclado, para podermos visualizar a execução passo a passo.
    //Substitua os comentários pelos devidos comandos (na ordem dos comentários) para ter a implementação do laço.
    while(!halt)
    {
        debug();

        //implementar! Pega uma microinstrução no armazenamento de controle no endereço determinado pelo registrador contador de microinstrução e decodifica (gera a estrutura de microinstrução, ou seja, separa suas partes). Cada parte é devidamente passada como parâmetro para as funções que vêm a seguir.
        decmcode = decode_microcode(microprog[mpc]);
        //implementar! Lê registradores
        read_registers(decmcode.reg_r);
        //implementar! Executa alu
        alu(decmcode.alu, bus_a, bus_b);
        //implementar! Executa deslocamento
        shift(decmcode.sft, alu_out);
        //implementar! Escreve registradores
        write_register(decmcode.reg_w);
        //implementar! Manipula memória principal
        mainmemory_io(decmcode.mem);
        //implementar! Determina endereço da microinstrução (mpc) a ser executada no próximo pulso de clock
        mpc = next_address(decmcode.nadd, decmcode.jam);

	    getchar();
    }

    debug(false);

    return 0;
}