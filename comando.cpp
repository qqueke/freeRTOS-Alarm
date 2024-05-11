//#ifdef notdef

/***************************************************************************
| File: comando.c  -  Concretizacao de comandos (exemplo)
|
| Autor: Carlos Almeida (IST)
| Data:  Nov 2002
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#define NR 20

typedef struct Sensor_Message_t
{
    float temperature;
    uint8_t luminosity;
} Sensor_Message_t;

typedef struct LCD_Message_t
{
    uint8_t x_offset;
    uint8_t y_offset;
    char buffer[32];
} LCD_Message_t;

typedef struct Processing_Message_t
{
    float temperature_max;
    float temperature_min;
    float temperature_mean;
    uint8_t luminosity_max;
    uint8_t luminosity_min;
    float luminosity_mean;

} Processing_Message_t;

typedef struct TimeStamp_t
{
    uint8_t hours1;
    uint8_t minutes1;
    uint8_t seconds1;
    uint8_t hours2;
    uint8_t minutes2;
    uint8_t seconds2;

} TimeStamp_t;


struct Record {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    float temperature;
    uint8_t luminosity;
};


// Sensor reading Queue
extern QueueHandle_t xSensorQueue;
extern QueueHandle_t xLcdQueue;
extern QueueHandle_t xProcessingQueue;
extern QueueHandle_t xTimeStampsQueue;


// Mutexes
extern SemaphoreHandle_t Clock_Mutex;
extern SemaphoreHandle_t Record_Mutex;

// On-demand monitoring handle
extern TaskHandle_t xSensorHandle;
extern TaskHandle_t xProcessingHandle;
extern TaskHandle_t xPSensorHandle;
extern TaskHandle_t xPProcessingHandle;

// Table
extern volatile uint8_t PMON; // Monitoring Period
extern volatile uint8_t TALA; // Speaker duration
extern volatile uint8_t PPROC; // Processing period
extern volatile uint8_t ALAF; // Alarm enabled
extern volatile uint8_t ALAH; // Alarm hours
extern volatile uint8_t ALAM; // Alarm minutes
extern volatile uint8_t ALAS; // Alarm seconds
extern volatile uint8_t ALAT; // Temperature thresholds
extern volatile uint8_t ALAL; // Luminosity threshold
extern volatile uint8_t CLKH; // Clock hours
extern volatile uint8_t CLKM; // Clock minutes
extern volatile uint8_t CLKS; // Clock seconds

//Alarm state
extern volatile uint8_t ALARM;


// Records
extern volatile uint8_t wi;
extern volatile uint8_t ri;
extern volatile uint8_t valid_records;
extern struct Record records[NR];

extern uint8_t Get_Next_Wi (uint8_t current_wi);

/*-------------------------------------------------------------------------+
| Function: cmd_sair - termina a aplicacao
+--------------------------------------------------------------------------*/ 
void cmd_sair (int argc, char **argv)
{
//  exit(0);
}

/*-------------------------------------------------------------------------+
| Function: cmd_test - apenas como exemplo
+--------------------------------------------------------------------------*/ 
void cmd_test (int argc, char** argv)
{
    int i;

    /* exemplo -- escreve argumentos */
    for (i=0; i<argc; i++)
        printf ("\nargv[%d] = %s", i, argv[i]);
}

/*-------------------------------------------------------------------------+
| Function: cmd_send - send message
+--------------------------------------------------------------------------*/ 
void cmd_send (int argc, char** argv)
{
// int32_t lValueToSend;
// BaseType_t xStatus;

//     if (argc == 2) {
//         printf ("msg: %s\n", argv[1]);
//         lValueToSend = atoi(argv[1]);
//         xStatus = xQueueSend( xQueue, &lValueToSend, 0 );
//     }
//     else {
//         printf ("wrong number of arguments!\n");
//     }
return;
}



// User interface commands
void cmd_rc(int argc, char **argv){

    if (argc != 1){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    printf("%02d:%02d:%02d\n", CLKH, CLKM, CLKS);
    return;
}

void cmd_sc(int argc, char **argv){
    
    if (argc != 4){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    if ((atoi(argv[1]) < 0 || atoi(argv[1]) > 23) || (atoi(argv[2]) < 0 || atoi(argv[2]) > 59) || (atoi(argv[3]) < 0 || atoi(argv[3]) > 59) ){
        printf("Invalid parameters range!\n");
        return;
    }


    xSemaphoreTake( Clock_Mutex, portMAX_DELAY );

    CLKH = (uint8_t) atoi(argv[1]);       
    CLKM = (uint8_t) atoi(argv[2]);
    CLKS = (uint8_t) atoi(argv[3]);

    printf("Set Clock to: %02d:%02d:%02d\n", CLKH, CLKM, CLKS);            

    xSemaphoreGive( Clock_Mutex );
    
    return;
}


void cmd_rtl(int argc, char **argv){

    if (argc != 1){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    BaseType_t xStatus;

    Sensor_Message_t readings;
    readings.temperature =  0;
    readings.luminosity = 0;

    xTaskNotifyGive( xSensorHandle );

    xStatus = xQueueReceive( xSensorQueue, &readings, pdMS_TO_TICKS( 4000 ) );

    if( xStatus == pdPASS ) 
        printf( "Temperature: %.1f C || Luminosity: %1d l\n", readings.temperature, readings.luminosity);
    else
        printf("Sensor timeout!\n");
    
    return;
}

void cmd_rp(int argc, char **argv){

    if (argc != 1){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    printf("PMON: %d, TALA: %d, PPROC: %d\n", PMON, TALA, PPROC);            

    return;
}

void cmd_mmp(int argc, char **argv){

    if (argc != 2){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }
    
    if ((atoi(argv[1]) < 0 || atoi(argv[1]) > 99)){
        printf("Invalid parameters range!\n");
        return;
    }

    uint8_t new_PMON = (uint8_t) atoi(argv[1]);

    if (PMON == new_PMON){
        return;
    }
  

    if (PMON == 0){
        PMON = new_PMON;  
        //vTaskResume(xPProcessingHandle);
    }
    else if (new_PMON == 0){
        //vTaskSuspend(xPProcessingHandle);
        PMON = new_PMON; 
    }
    else{
        PMON = new_PMON; 
    }
    printf("Set PMON to: %d\n", PMON);

    return;
}


void cmd_mta(int argc, char **argv){
   
    if (argc != 2){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    if ((atoi(argv[1]) < 0 || atoi(argv[1]) > 60)){
        printf("Invalid parameters range!\n");
        return;
    }

    TALA = (uint8_t) atoi(argv[1]);   

    return;
}


void cmd_mpp(int argc, char **argv){

    if (argc != 2){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    if ((atoi(argv[1]) < 0 || atoi(argv[1]) > 99)){
        printf("Invalid parameters range!\n");
        return;
    }

    PPROC = (uint8_t) atoi(argv[1]); 


    uint8_t new_PPROC = (uint8_t) atoi(argv[1]);

    if (PPROC == new_PPROC){
        return;
    }

    if (PPROC == 0){
        PPROC = new_PPROC;  
        vTaskResume(xPProcessingHandle);
    }
    else if (new_PPROC == 0){
        vTaskSuspend(xPProcessingHandle);
        PPROC = new_PPROC; 
    }
    else{
        PPROC = new_PPROC; 
    }

    return;
}


void cmd_rai(int argc, char **argv){
    if (argc != 1){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    printf("Alarm Clock: %02d:%02d:%02d\n", ALAH, ALAM, ALAS);
    
    if (ALAF)            
        printf("Alarm Enabled|: ALAT: %d, ALAL: %d\n", ALAT, ALAL);            
    else
        printf("Alarm Disabled|: ALAT: %d, ALAL: %d\n", ALAT, ALAL);            

    return;
}


void cmd_dac(int argc, char **argv){

    if (argc != 4){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    if ((atoi(argv[1]) < 0 || atoi(argv[1]) > 23) || (atoi(argv[2]) < 0 || atoi(argv[2]) > 59) || (atoi(argv[3]) < 0 || atoi(argv[3]) > 59) ){
        printf("Invalid parameters range!\n");
        return;
    }

    ALAH = (uint8_t) atoi(argv[1]);       
    ALAM = (uint8_t) atoi(argv[2]);
    ALAS = (uint8_t) atoi(argv[3]);

    printf("Set Alarm Clock to: %02d:%02d:%02d\n", ALAH, ALAM, ALAS);  

    return;
}


void cmd_dtl(int argc, char **argv){

    if (argc != 3){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }
    
    if ((atoi(argv[1]) < 0 || atoi(argv[1]) > 50) || (atoi(argv[2]) < 0 || atoi(argv[2]) > 3)){
        printf("Invalid parameters range!\n");
        return;
    }

    ALAT = (uint8_t) atoi(argv[1]);       
    ALAL = (uint8_t) atoi(argv[2]);

    printf("Set temperature threshold to: %d\n", ALAT);
    printf("Set luminosity threshold to: %d\n", ALAL);


    return;
}

void cmd_aa(int argc, char **argv){
    if (argc != 2){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    if (argv[1][0] == 'A'){
        ALAF = 1;
        printf("Alarm Set\n");
    }
    else if (argv[1][0] == 'a'){
        ALAF = 0;
        printf("Alarm Unset\n");
    }
    else
        printf("Error enabling the alarm\n");

    return;
}


void cmd_cai(int argc, char **argv){
    if (argc != 1){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }
   
    ALARM = 0;

    printf("SET ALARM TO: %d\n", ALARM);


    LCD_Message_t message;
    message.x_offset = 99;
    message.y_offset = 0;
    sprintf(message.buffer, "   ");
    xQueueSendToBack( xLcdQueue, &message, 0 );

    return;
}


void cmd_ir(int argc, char **argv){
    if (argc != 1){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    printf("NR: %d, nr: %d, wi %d, ri: %d\n", NR, valid_records, wi, ri);            

    return;
}

void cmd_lr(int argc, char **argv){
    if (argc != 3){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    if ((atoi(argv[1]) < 0 || atoi(argv[1]) > 20) || (atoi(argv[2]) < 0 || atoi(argv[2]) > 19)){
        printf("Invalid parameters range!\n");
        return;
    }

    int n = atoi(argv[1]);
    int count = 0;

    if (valid_records < 20){
        for (uint8_t index = atoi(argv[2]); index < valid_records; ++index){
            if (count == n)
                break;

            printf("Record[%d]: %02u:%02u:%02u %.3fC L %u\n", index, records[index].hours, records[index].minutes, records[index].seconds, records[index].temperature, records[index].luminosity);
            count ++;
        }
    }
    else {
        uint8_t reading_index = (uint8_t) atoi(argv[2]);
        for (uint8_t index = atoi(argv[2]); ; ++index){
            if (count == n)
                break;

            printf("Record[%d]: %02u:%02u:%02u %.3fC L %u\n", reading_index, records[reading_index].hours, records[reading_index].minutes, records[reading_index].seconds, records[reading_index].temperature, records[reading_index].luminosity);
            reading_index = Get_Next_Wi(reading_index);

            count ++;
        }
    }


    
    return;
}


void cmd_dr(int argc, char **argv){
    if (argc != 1){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }

    xSemaphoreTake( Record_Mutex, portMAX_DELAY );
    valid_records = 0;
    wi = 0;
    ri = 0;
    xSemaphoreGive( Record_Mutex );

    return;
}

void cmd_pr(int argc, char **argv){
    if (argc != 7){
        printf("Wrong number of arguments!\n");
        printf("Argc = %d\n", argc);
        return;
    }


    if ((atoi(argv[1]) < 0 || atoi(argv[1]) > 23) || (atoi(argv[2]) < 0 || atoi(argv[2]) > 59) || (atoi(argv[3]) < 0 || atoi(argv[3]) > 59) ){
        printf("Invalid parameters range!\n");
        return;
    }

    if ((atoi(argv[4]) < 0 || atoi(argv[4]) > 23) || (atoi(argv[5]) < 0 || atoi(argv[5]) > 59) || (atoi(argv[6]) < 0 || atoi(argv[6]) > 59) ){
        printf("Invalid parameters range!\n");
        return;
    }



    BaseType_t xStatus;
    Processing_Message_t metrics;
    TimeStamp_t timestamps;
    timestamps.hours1 = (uint8_t) atoi(argv[1]);
    timestamps.minutes1 = (uint8_t) atoi(argv[2]);
    timestamps.seconds1 = (uint8_t) atoi(argv[3]);
    timestamps.hours2 = (uint8_t) atoi(argv[4]);
    timestamps.minutes2 = (uint8_t) atoi(argv[5]);
    timestamps.seconds2 = (uint8_t) atoi(argv[6]);

    xStatus = xQueueSendToBack( xTimeStampsQueue, &timestamps, pdMS_TO_TICKS( 4000 ) );

    if( xStatus != pdPASS ) {
        printf("Processing timeout!\n");
        return;
    }

    xStatus = xQueueReceive( xProcessingQueue, &metrics, pdMS_TO_TICKS( 4000 ) );

    if( xStatus != pdPASS ) {
        printf("Processing timeout!\n");
        return;
    }

    printf( "Temperature Max: %.1f C || Luminosity Max: %1d l\n", metrics.temperature_max, metrics.luminosity_max);
    printf( "Temperature Min: %.1f C || Luminosity Min: %1d l\n", metrics.temperature_min, metrics.luminosity_min);
    printf( "Temperature Mean: %.1f C || Luminosity Mean: %.1f l\n", metrics.temperature_mean, metrics.luminosity_mean);

    return;
}




//#endif //notdef
