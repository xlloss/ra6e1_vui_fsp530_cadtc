#ifndef INC_AHT10_H_
#define INC_AHT10_H_

#include <stdint.h>
#include <stdbool.h>

#define AHT10_ADDRESS              0x38  //Direccion del dispositivo
#define AHT10_INIT_CMD             0xE1  //Comando de inicializacion AHT10 '1110'0001'
#define AHT10_START_MEASURMENT_CMD 0xAC  //Comando de inicio de medición
#define AHT10_NORMAL_CMD           0xA8  //Comando de modo normal
#define AHT10_SOFT_RESET_CMD       0xBA  //Comando para reset por software
#define AHT10_ERROR                0xFF  //Retorno de error de comunicacion
#define AHT10_DATA_MEASURMENT_CMD  0x33
#define AHT10_DATA_NOP             0x00

//####Declaración de funciones para el manejo del sesnor AHT10#####//

void AHT10_Init(void);                      //Funcion para inicializar sensor//
int AHT10_Read_Temp(void);                  //Funcion para leer valor de la temperatura//
int AHT10_Read_Hum(void);                   //Función para leer valor de la humedad//
void AHT10_Reset(void);
#endif /* INC_AHT10_H_ */
