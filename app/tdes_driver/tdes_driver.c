#include <ucx.h>
#include <device.h>
#include "tdes_driver.h"

#define DES_BASE			0xe7000000
#define DES_CONTROL			(*(volatile uint32_t *)(DES_BASE + 0x000))
#define DES_KEY1_1			(*(volatile uint32_t *)(DES_BASE + 0x010))
#define DES_KEY1_2			(*(volatile uint32_t *)(DES_BASE + 0x020))
#define DES_KEY2_1			(*(volatile uint32_t *)(DES_BASE + 0x030))
#define DES_KEY2_2			(*(volatile uint32_t *)(DES_BASE + 0x040))
#define DES_KEY3_1			(*(volatile uint32_t *)(DES_BASE + 0x050))
#define DES_KEY3_2			(*(volatile uint32_t *)(DES_BASE + 0x060))
#define DES_IN0			    (*(volatile uint32_t *)(DES_BASE + 0x070))
#define DES_IN1			    (*(volatile uint32_t *)(DES_BASE + 0x080))
#define DES_OUT0			(*(volatile uint32_t *)(DES_BASE + 0x090))
#define DES_OUT1			(*(volatile uint32_t *)(DES_BASE + 0x0A0))

// Bits de controle
#define DES_ENCRYPT			(1 << 4)
#define DES_DECRYPT			(0 << 4)
#define DES_RESET			(1 << 3)
#define DES_LDKEY			(1 << 2)
#define DES_LDDATA			(1 << 1)
#define DES_READY			(1 << 0)
#define BLOCKLEN_BYTES	        8
#define BLOCKLEN_BITS           64

#define NO_TASK 0xFFFF
static uint16_t tarefa_que_chamou = NO_TASK;

e_STATE state = OFF;

static void enc_block(uint32_t block[2]){
	DES_IN0 = block[0];
	DES_IN1 = block[1];
	DES_CONTROL |= DES_LDDATA;
	DES_CONTROL &= (~DES_LDDATA);
	_delay_us(1);
	block[0] = DES_OUT0;
	block[1] = DES_OUT1;
}

static void dec_block(uint32_t block[2]){
	DES_IN0 = block[0];
	DES_IN1 = block[1];
	DES_CONTROL |= DES_LDDATA;
	DES_CONTROL &= (~DES_LDDATA);
	_delay_us(1);
	block[0] = DES_OUT0;
	block[1] = DES_OUT1;
}

static void enc_str(s_CONFIG_tDES* config){
	DES_CONTROL = 0;
	DES_CONTROL |= (DES_ENCRYPT);
	DES_CONTROL |= (DES_RESET);
	DES_CONTROL &= (~DES_RESET);
	DES_KEY1_1 = config->KEY[0]; // Primeira parte da chave 1
	DES_KEY1_2 = config->KEY[1]; // Segunda parte da chave 1
	DES_KEY2_1 = config->KEY[2]; // Primeira parte da chave 2
	DES_KEY2_2 = config->KEY[3]; // Segunda parte da chave 2
	DES_KEY3_1 = config->KEY[4]; // Primeira parte da chave 3
	DES_KEY3_2 = config->KEY[5]; // Segunda parte da chave 3
	DES_CONTROL |= DES_LDKEY;
	DES_CONTROL &= (~DES_LDKEY);
}

static void dec_str(s_CONFIG_tDES* config) {
	DES_CONTROL = 0;
	DES_CONTROL |= (DES_DECRYPT);
	DES_CONTROL |= (DES_RESET);
	DES_CONTROL &= (~DES_RESET);
	DES_KEY1_1 = config->KEY[0]; // Primeira parte da chave 1
	DES_KEY1_2 = config->KEY[1]; // Segunda parte da chave 1
	DES_KEY2_1 = config->KEY[2]; // Primeira parte da chave 2
	DES_KEY2_2 = config->KEY[3]; // Segunda parte da chave 2
	DES_KEY3_1 = config->KEY[4]; // Primeira parte da chave 3
	DES_KEY3_2 = config->KEY[5]; // Segunda parte da chave 3
	DES_CONTROL |= DES_LDKEY;
	DES_CONTROL &= (~DES_LDKEY);
}

static int tdes_init(const struct device_s *dev) {
    // TRATAMENTO DE ERRO ---------------------------------------------------
	if(state == ON){
        printf("[TDES_DRIVER]: Driver em ja em uso\n");
        return -1;
    }
    // ----------------------------------------------------------------------
	NOSCHED_ENTER();
    s_DATA_tDES *pdata = (s_DATA_tDES *)dev->data;
	pdata->BUFFER = NULL;
	pdata->size = 0;
	tarefa_que_chamou = NO_TASK;
    state = ON;
    printf("[TDES_DRIVER]: Driver ligado\n");
	NOSCHED_LEAVE();
	return 0;
}

static int tdes_open(const struct device_s *dev, int mode) {
    // TRATAMENTO DE ERRO ---------------------------------------------------
	if(state == OFF){
        printf("[TDES_DRIVER]: Driver nao esta ligado\n");
        return -1;
    }
    if (tarefa_que_chamou != NO_TASK) {
        tarefa_que_chamou = ucx_task_id();
        printf("[TDES_DRIVER]: DRIVER IN USE\n");
        return -1;
    }
    // ----------------------------------------------------------------------
	NOSCHED_ENTER();
    s_DATA_tDES *pdata = (s_DATA_tDES *)dev->data;
    printf("[TDES_DRIVER]: Usuario atual: %d\n", (int)tarefa_que_chamou);
    printf("[TDES_DRIVER]: Usuario que chamou: %d\n", (int)ucx_task_id());
	if (tarefa_que_chamou != NO_TASK) {
        printf("[TDES_DRIVER]: INVALIDA: %d\n", (int)tarefa_que_chamou);
        NOSCHED_LEAVE();
		return -1;  
	} 
    tarefa_que_chamou = ucx_task_id();
    state = ON;
	NOSCHED_LEAVE();
	return 0;
}

static int tdes_close(const struct device_s *dev) {
    // TRATAMENTO DE ERRO: ----------------------------------------------------
	if (tarefa_que_chamou != ucx_task_id()) {
        printf("[TDES_DRIVER]: Nao es o user correto\n");
        printf("[TDES_DRIVER]: Usuario atual: %d\n", (int)tarefa_que_chamou);
        printf("[TDES_DRIVER]: Usuario que chamou: %d\n", (int)ucx_task_id());
        return -1;
    }
    if(state == OFF){
        printf("[TDES_DRIVER]: Driver nao esta ligado\n");
        return -2;
    }
    // ----------------------------------------------------------------------
	NOSCHED_ENTER();
    s_DATA_tDES *pdata = (s_DATA_tDES *)dev->data;
    if(pdata->BUFFER) {
        free(pdata->BUFFER);
        pdata->BUFFER = NULL;
        pdata->size = 0;
    }
	tarefa_que_chamou = NO_TASK;
    state = OFF;
    printf("[TDES_DRIVER]: Closing driver\n");
	NOSCHED_LEAVE();
	return 0;
}


static size_t tdes_read(const struct device_s *dev, void *buf, size_t count) {
	// TRATAMENTO DE ERRO ----------------------------------------------------
    if (buf == NULL) {
        //printf("[TDES_DRIVER]: Buffer vazio\n");
        return -1;
    }
	if (tarefa_que_chamou != ucx_task_id()) {
        //printf("[TDES_DRIVER]: Nao es o user correto\n");
        //printf("[TDES_DRIVER]: Usuario atual: %d\n", (int)tarefa_que_chamou);
        //printf("[TDES_DRIVER]: Usuario que chamou: %d\n", (int)ucx_task_id());
        return -2;
    }
    if(state == OFF){
        //printf("[TDES_DRIVER]: Driver nao esta ligado\n");
        return -3;
    }
    // ----------------------------------------------------------------------
    NOSCHED_ENTER();
    s_DATA_tDES *pdata = (s_DATA_tDES *)dev->data;
    size_t max_size = (count < pdata->size) ? count : pdata->size;
	if (max_size > 0) {
		memcpy(buf, pdata->BUFFER, max_size);
		free(pdata->BUFFER);
		pdata->BUFFER = NULL;
		pdata->size = 0;
	}
    NOSCHED_LEAVE();
	return max_size;
}

static size_t tdes_write(const struct device_s *dev, void *buf, size_t count) {
   	// TRATAMENTO DE ERRO ----------------------------------------------------------------------
    if (!dev || !dev->data || !dev->config) {
        //printf("[TDES_DRIVER]: Ponteiros de device inválidos\n");
        return -1;
    }
    if (buf == NULL) {
        //printf("[TDES_DRIVER]: Buffer vazio\n");
        return -2;
    }
	if (tarefa_que_chamou != ucx_task_id()) {
        //printf("[TDES_DRIVER]: Nao é o user correto\n");
        return -3;
    }
    if(state == OFF){
        //printf("[TDES_DRIVER]: Driver nao esta ligado\n");
        return -4;
    }
    // ----------------------------------------------------------------------------------------
    NOSCHED_ENTER();
    s_CONFIG_tDES *pconfig = (s_CONFIG_tDES *)dev->config;
    s_DATA_tDES *pdata = (s_DATA_tDES *)dev->data;

    size_t count_padding, QUANTITY_of_BLOCKS, QUANTIDADE_DE_BLOCKS_CHEIOS, resto;
    uint32_t keystream[2];
    uint32_t temp_IV[2];
    uint32_t block[2];
    uint8_t *out_buf, *in_buf;
    //-----------------------------------------------------------------------------------------
    //printf("[TDES_DRIVER] Tarefa %u esta tentando %s dados com:\n", ucx_task_id(), (pconfig->type == ENCRYPT) ? "ENCRYPT" : "DECRYPT"); //DEBUG 

    //switch (pconfig->mode) { //DEBUG 
    //     case ECB: printf("- Modo: ECB\n"); break;
    //     case CBC: printf("- Modo: CBC\n"); break;
    //     case CTR: printf("- Modo: CTR\n"); break;
    //     default:  printf("- Modo: DESCONHECIDO\n"); break;
    // }

    if (count == 0) { count = strlen((const char *)buf);}
    //printf("- Tamanho original:  %u bytes\n", count); //DEBUG 

    QUANTIDADE_DE_BLOCKS_CHEIOS = (count / BLOCKLEN_BYTES);
    //printf("- Blocos completos: %d blocos\n", (int)QUANTIDADE_DE_BLOCKS_CHEIOS); //DEBUG 

    resto = count % BLOCKLEN_BYTES;
    //printf("- Resto: %d bytes\n", (int)resto); //DEBUG 

    if(resto){
        count_padding = BLOCKLEN_BYTES - resto;
        QUANTITY_of_BLOCKS = QUANTIDADE_DE_BLOCKS_CHEIOS +1;
    }
    else{
        count_padding = 0;
        QUANTITY_of_BLOCKS = QUANTIDADE_DE_BLOCKS_CHEIOS;
    }
    //printf("- Padding adicionado: %d bytes\n", (int)count_padding); //DEBUG 

    size_t count_total = QUANTITY_of_BLOCKS * BLOCKLEN_BYTES;
    //printf("- Total final: %d bytes (%d blocos)\n", (int)count_total, (int)QUANTITY_of_BLOCKS); //DEBUG 
    pdata->size = count_total;
    //-----------------------------------------------------------------------------------------
    // ALOCA ESPAÇOS
    in_buf = malloc(count_total);
    if (!in_buf) { // TRATAMENTO DE ERRO
        //printf("[TDES_DRIVER]: Out of memory\n");
        NOSCHED_LEAVE();
        return (size_t)-1;
    }
    memset(in_buf, 0, count_total);

    if (pdata->BUFFER) free(pdata->BUFFER); // Caso buffer ja tiver conteudo dentro
    pdata->BUFFER = malloc(count_total);
    if (pdata->BUFFER == NULL) { // TRATAMENTO DE ERRO
        free(in_buf);
        //printf("[TDES_DRIVER]: Out of memory\n");
        NOSCHED_LEAVE();
        return -1;
    }
    memset(pdata->BUFFER, 0, count_total);

    out_buf = malloc(count_total);
    if (out_buf == NULL) { // TRATAMENTO DE ERRO
        free(in_buf);
        free(pdata->BUFFER);
        //printf("[TDES_DRIVER]: Out of memory\n");
        NOSCHED_LEAVE();
        return -1;
    }
    memset(out_buf, 0, count_total);
    //-----------------------------------------------------------------------------------------
    // <Inicio do real processo>

    temp_IV[0] = pconfig->iv[0]; // Salva para reaproveitar
    temp_IV[1] = pconfig->iv[1]; // Salva para reaproveitar

    //printf("BUFFER @ %p, OUT_BUF @ %p, size = %u\n", pdata->BUFFER, out_buf, (unsigned)count_total);
    memcpy(in_buf, buf, count);
    if (resto > 0) memset(in_buf + count, 0x03, count_padding); // adiciona somente  a quantidade de pading necessaria count_padding

    /// Tipo de operacao___________________________________________
    if (pconfig->mode == CTR)         ///CTR usa SEMPRE encrypt 
        enc_str(pconfig);            
    else if (pconfig->type == ENCRYPT)
        enc_str(pconfig);
    else
        dec_str(pconfig);
    //____________________________________________________________
    
    // Logida da escolha do modulo
    for (size_t j = 0; j < QUANTITY_of_BLOCKS; ++j) {
        size_t base = j * BLOCKLEN_BYTES;
        memset(block, 0, sizeof(block));

        memcpy(&block[0], in_buf + base, 4);
        memcpy(&block[1], in_buf + base + 4, 4);

        if (pconfig->mode == ECB) {
            if(pconfig->type == ENCRYPT)
                enc_block(block);
            else 
                dec_block(block);
        } 
        else if (pconfig->mode == CBC) {
            if (pconfig->type == ENCRYPT) { // ENCRYPT
               
                block[0] ^= pconfig->iv[0];
                block[1] ^= pconfig->iv[1];
                enc_block(block);           
                pconfig->iv[0] = block[0];             
                pconfig->iv[1] = block[1];
            } else { // DECRYPT
                uint32_t next_iv[2] = {block[0], block[1]};  
                dec_block(block);               
                block[0] ^= pconfig->iv[0];       
                block[1] ^= pconfig->iv[1];
                pconfig->iv[0] = next_iv[0];   
                pconfig->iv[1] = next_iv[1];
            }
        } 
        else if (pconfig->mode == CTR) {
            keystream[0] = pconfig->iv[0];
            keystream[1] = pconfig->iv[1];
            /// Gera keystream cifrando o IV
            enc_block(keystream);
            block[0] ^= keystream[0];/// XOR do bloco de entrada com o keystream
            block[1] ^= keystream[1];/// XOR do bloco de entrada com o keystream
            /// Incrementa o contador
            pconfig->iv[1]++;
            if (pconfig->iv[1] == 0) pconfig->iv[0]++;
        }

        memcpy(out_buf + base, block, BLOCKLEN_BYTES);
    }

    pconfig->iv[0] = temp_IV[0];
    pconfig->iv[1] = temp_IV[1];
    //hex_dump_uint8(out_buf, count_total);// DEGUG
    memcpy(pdata->BUFFER, out_buf, count_total);
    free(in_buf);
    free(out_buf);
    
    NOSCHED_LEAVE();
    return count_total;
}

struct device_api_s dev_tdes = {
	.dev_init = tdes_init,
	.dev_open = tdes_open,
	.dev_close = tdes_close,
	.dev_read = tdes_read,
	.dev_write = tdes_write,
};