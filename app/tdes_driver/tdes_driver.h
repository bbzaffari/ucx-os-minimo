#ifndef TDES_DRIVER_H
#define TDES_DRIVER_H

/* user data */
typedef enum { // SELECIONANDO O TIPO
	DECRYPT = 27,
	ENCRYPT = 54
}e_TYPE;

typedef enum { 
	ON,
	OFF, 
    IDLE
}e_STATE;

typedef enum { // SELECIONANDO O MODO
	ECB = 20,
	CBC = 40,
	CTR = 60
}e_MODE;

typedef struct {
	const uint32_t KEY[6];
	e_TYPE  type;
	e_MODE  mode;
	uint32_t iv[2];
} s_CONFIG_tDES;

typedef struct {
	uint8_t *BUFFER; // INPUT no driver TDES
	size_t size;
} s_DATA_tDES;


extern struct device_api_s dev_tdes;



#endif

// FUNCAO HEXDUMP CRIADA
// void hex_dump_uint8(const uint8_t *data, size_t size);
// void hex_dump_uint8(const uint8_t *data, size_t size) {
//     size_t i, j;

//     for (i = 0; i < size; i += 16) {
//         printf("%08zx  ", i);  // Offset

//         // Parte hexadecimal
//         for (j = 0; j < 16; ++j) {
//             if (i + j < size)
//                 printf("%02x ", data[i + j]);
//             else
//                 printf("   ");
//             if (j == 7) printf(" ");
//         }

//         printf(" |");

//         // Parte ASCII
//         for (j = 0; j < 16 && i + j < size; ++j) {
//             uint8_t byte = data[i + j];
//             printf("%c", isprint(byte) ? byte : '.');
//         }

//         printf("|\n");
//     }
// }
///Ocasionava erros