#include <ucx.h>
#include <device.h>
#include "tdes_driver.h"
//As memorias foram postas com tamanho fixo mas pode ser mudadas
static size_t count=0;
char plain_text[50] = {0};
char texto_cifrado[50]= {0}; 
char texto_retorno_leg[50]= {0};
char print[50]= {0};

//---------------------------------------------------------------
static s_CONFIG_tDES config = {
	.KEY  = {0x88880001, 0x88880002, 0x88880003, 0x88885000, 0x88882000, 0x88883000},
	.type = ENCRYPT, 
	.mode = ECB, 
	.iv   = {0x88880001, 0x88880000}
};

static s_DATA_tDES dados_enc = {
	.BUFFER= NULL,    // INPUT no driver TDES
	.size= 0
};

static s_DATA_tDES dados_dec = {
	.BUFFER= NULL,    // INPUT no driver TDES
	.size= 0
};

static struct device_s dev = {
	.name = "tdes_ECB",
	.config = &config,
	.data = &dados_enc,
	.api = &dev_tdes
};
//---------------------------------------------------

//Funcoes auxiliares --------------------------------
size_t calculo(char *mensagem, size_t modo) {
	// modo == 0 é Debug
    if (!mensagem) {
        printf("[APP_MAIN] Mensagem nula\n");
        return 0;
    }
	size_t BLOCKLEN_BYTES = 8;

    size_t tamanho = strlen((const char *)mensagem);

    size_t blocos_cheios     = tamanho / BLOCKLEN_BYTES;
    size_t resto             = tamanho % BLOCKLEN_BYTES;
    size_t padding_necessario = (resto > 0) ? (BLOCKLEN_BYTES - resto) : 0;
    size_t blocos_totais     = blocos_cheios + (resto > 0 ? 1 : 0);
    size_t tamanho_total     = blocos_totais * BLOCKLEN_BYTES;

	if (modo == 0){ 
		printf(" [APP_MAIN] Pre-calculo:\n");
		printf("- Tamanho original:  %u bytes\n", tamanho);
		printf("- Blocos completos:        %u\n", blocos_cheios);
		printf("- Bytes de padding:        %u\n", padding_necessario);
		printf("- Total final (criptografado): %u bytes (%u blocos)\n", tamanho_total, blocos_totais);
	}
	return tamanho_total;
}
//----------------------------------------------------------------------------------------


void func_encode_decode(void){
	strcpy(plain_text, "the quick brown fox jumps over the lazy dog");

	count = calculo(plain_text, 0);
	
	printf("------------------------------- INICIO -----------------------------------\n");
	printf("\n\n-------------------------------------------------------------------\n \
	MODO DE OPERACAO: <<<<<<<<<<<<EBC>>>>>>>>>>>>>>>\n \
	-------------------------------------------------------------------\n\n");
	printf("[APP_MAIN] TEXTO ORIGINAL: \n%s\n", plain_text);
	hexdump(plain_text, count);
	printf("\n");

	// ABRINDO -----------------------------------------------------------------------
	if (dev_open(&dev, 0)) {
		printf("task %d: driver in use.\n", ucx_task_id());
		return;
	}
	// ABRINDO -----------------------------------------------------------------------
	printf("\n");

	// CIFRANDO ----------------------------------------------------------------------
	dev_write(&dev, plain_text, count);
	dev_read(&dev, texto_cifrado, count);
	printf("[APP_MAIN] TEXTO CIFRADO: \n");
	memset(print, 0, sizeof(char)*50);
    memcpy(print, texto_cifrado, count);
	hexdump(print, count);

	// DECIFRANDO ----------------------------------------------------------------------
	config.type = DECRYPT;
	dev.data = &dados_dec;
	_delay_us(3);
	dev_write(&dev, texto_cifrado, count);
	dev_read(&dev, texto_retorno_leg, count);
	printf("[APP_MAIN] TEXTO RECEBIDO DECIFRADO: \n");
	memset(print, 0, sizeof(char)*50);
    memcpy(print, texto_retorno_leg, count); 
	hexdump(print, count);
	
	printf("--------------------------------- FIM ---------------------------------\n");

	if (memcmp(plain_text, texto_retorno_leg, strlen(plain_text)) == 0) {
		printf("[VERIFICACAO]: PASSOU Texto decifrado BATE com o original.\n");
	} else {
		printf("[VERIFICACAO]: Texto decifrado nao confere com o original.\n");
	}

	free(texto_cifrado);
	free(texto_retorno_leg);
	_delay_us(3);
	// FECHANDO --------------------------------------
	dev_close(&dev);
	// FECHANDO --------------------------------------
}

void task_encode_decode(void){
	func_encode_decode();
	krnl_panic(0);
    for(;;);
}
int32_t app_main(void){
	ucx_task_spawn(task_encode_decode, DEFAULT_STACK_SIZE);
	dev_init(&dev);
	return 1;
}

/// Aqui em baixo se encontra um prototipo de escalonador de IO
/*IO SCHED*/

// #define MAX_FILA 1
// #define TIME_OUT 2000 // ms ou ticks

// #define ON 1
// #define OFF 0

// #define WRITE 1
// #define READ 0

// static volatile uint8_t TDES_DRIVER = OFF;
// static volatile uint8_t index_begin = 0;
// static volatile uint8_t index_end = 0;
// static volatile uint8_t TAM_FILA = 0;

// static volatile int LAST_TIME_IO_tDES_RQST = 0;

// static s_CONFIG_tDES FILA[MAX_FILA] = {0};
// void f_Atualiza_Tam_Fila(void){// FILA CIRCULAR FIFO
	
// 	if (index_end > index_begin)
// 		TAM_FILA = index_end - index_begin;

// 	else if (index_end < index_begin) 
// 		TAM_FILA = MAX_FILA - index_begin + index_end + 1;
	
// 	else 
// 		TAM_FILA = 0;

// 	return;
// };

// uint8_t f_RQST_PARA_IO_SCHED_TDES(s_CONFIG_tDES *RQST_IO){
// 	uint8_t temp_end = 0;
// 	if(MAX_FILA == TAM_FILA) return 0; // INDICA QUE A FILA ESTA CHEIA;

// 	//else{
// 		// ENTRA CRITICAL OU MUTEX OU SPINLOCK;
// 		// ATUALIZA LAST_TIME_IO_tDES_RQST;
// 		FILA[index_end] = &RQST_IO;
// 		// POEM EM ESTADO DE BLOCK;
// 		temp_end = index_end;
// 		index_end = (index_end + 1) % MAX_FILA;
// 		f_Atualiza_Tam_Fila();
// 		// SAI CRITICAL OU MUTEX OU SPINLOCK;
// 		return temp_end;
// 	//}
// };

// void t_IO_SCHAD_tDES(void){
// 	int NOW;
// 	/* LAST_OP 
// 	* Sera para verificar a ultima operacao 
// 	* feita, para tambem fazer com q apos do 
// 	* ....
// 	*/
// 	uint8_t LAST_OP = 0;
// 	uint16_t temp_PID = 0;
// 	// ENTRA CRITICAL OU MUTEX OU SPINLOCK;
// 	if (!TAM_FILA)
// 		if(TIME_OUT < (NOW - LAST_TIME_IO_tDES_RQS)){
// 			dev_close();
// 			yield();
// 			// SAI CRITICAL OU MUTEX OU SPINLOCK;
// 		}
// 	else{
// 		if(!TDES_DRIVER)
// 			dev_init();
// 		if (LAST_OP == WRITE){
// 			dev_read(FILA[index_begin]->OUT_BUFFER);
// 			temp_PID = FILA[index_begin]->PID;
// 			FILA[index_begin] = NULL;
// 			index_begin = (index_begin + 1) % MAX_FILA;
// 			f_Atualiza_Tam_Fila();
// 			//DESVLOQUEIA PID
// 			LAST_OP = READ;
// 		}
// 		else if (LAST_OP == READ && TAM_FILA !=0){
// 			dev_write(FILA[index_begin]->);

// 		}
// 		yield();



// 	}
// }
