#include "mbed.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "LM75B.h"
#include "C12832.h"
#include <cstdint>
#include <cstdlib>

// Number of records to store
#define NR 20

// LCD
C12832 lcd(p5, p7, p6, p8, p11);

// Temperature sensor
LM75B sensor(p28,p27);

// LEDS
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// PC
Serial pc(USBTX, USBRX);

// Potentiometer
AnalogIn pot1(p19);

// Speaker
PwmOut spkr(p26);

// RGB
PwmOut r (p23);
PwmOut g (p24);
PwmOut b (p25);

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

//Queuess
QueueHandle_t xSensorQueue;
QueueHandle_t xProcessingQueue;
QueueHandle_t xLcdQueue;
QueueHandle_t xTimeStampsQueue;


// Mutexes
SemaphoreHandle_t Clock_Mutex;
SemaphoreHandle_t Record_Mutex;
SemaphoreHandle_t Sensor_Mutex;


TaskHandle_t xSensorHandle = NULL;
TaskHandle_t xProcessingHandle = NULL;
TaskHandle_t xPSensorHandle = NULL;
TaskHandle_t xPProcessingHandle = NULL;



extern void monitor(void);

// Table
volatile uint8_t PMON = 3; // Monitoring Period
volatile uint8_t TALA = 5; // Speaker duration
volatile uint8_t PPROC = 0; // Processing period
volatile uint8_t ALAF = 0; // Alarm enabled
volatile uint8_t ALAH = 0; // Alarm hours
volatile uint8_t ALAM = 0; // Alarm minutes
volatile uint8_t ALAS = 0; // Alarm seconds
volatile uint8_t ALAT = 20; // Temperature thresholds
volatile uint8_t ALAL = 2; // Luminosity threshold
volatile uint8_t CLKH = 0; // Clock hours
volatile uint8_t CLKM = 0; // Clock minutes
volatile uint8_t CLKS = 0; // Clock seconds

// Alarm variables 
volatile uint8_t ALARM = 0;
volatile uint8_t ALARM_TIMER = 0;


// Ring buffer
struct Record records[NR];
// Write index
volatile uint8_t wi = 0;
// Read Index
volatile uint8_t ri = 0;

volatile uint8_t valid_records = 0;


// Perguntas:
// Podemos ter 2 Sensor tasks uma para on-demand outra periodica?
// O user deve dar os comandos do genero: sc\n h\n m\n s\n ou sc h m s?
// COmo e que o monitor.cpp referncia as funcoes sem o import do comando.cpp???

// Returns next write index
uint8_t Get_Next_Wi(uint8_t current_wi) {
    return (current_wi + 1) % NR;
}

void Activate_Alarm(void) {
    ALARM = 1;

    if (!ALARM_TIMER && TALA != 0){
        ALARM_TIMER = 1;
        spkr = 0.2;
    }


    LCD_Message_t message;
    message.x_offset = 99;
    message.y_offset = 0;
    sprintf(message.buffer, "CTL");
    xQueueSendToBack( xLcdQueue, &message, 0 );
    return;
}

uint8_t isWithinTimeRange(uint8_t hour, uint8_t minute, uint8_t second, const TimeStamp_t& timestamp) {
    // Assuming the timestamp defines the full range
    if((hour > timestamp.hours1 || (hour == timestamp.hours1 && minute > timestamp.minutes1) ||
         (hour == timestamp.hours1 && minute == timestamp.minutes1 && second >= timestamp.seconds1)) &&
        (hour < timestamp.hours2 || (hour == timestamp.hours2 && minute < timestamp.minutes2) ||
         (hour == timestamp.hours2 && minute == timestamp.minutes2 && second <= timestamp.seconds2))){
        return 1;
    }
    
    return 0;
}
/*-------------------------------------------------------------------------+
| Function: my_fgets        (called from my_getline / monitor) 
+--------------------------------------------------------------------------*/ 
char* my_fgets (char* ln, int sz, FILE* f)
{
    //  fgets(line, MAX_LINE, stdin);
    //  pc.gets(line, MAX_LINE);
    int i; char c;
    for(i=0; i<sz-1; i++) {
        c = pc.getc();
        ln[i] = c;
        if ((c == '\n') || (c == '\r')) break;
    }
    ln[i] = '\0';

    return ln;
}

void VUI_Task( void *pvParameters ) {
    for( ;; ) {
        monitor(); //does not return
    }
}

static void vLCD_Gatekeeper_Task( void *pvParameters )
{
    LCD_Message_t message;

    for( ;; )
    {
        /* Wait for a message to arrive. An indefinite block time is specified so
        there is no need to check the return value – the function will only return
        when a message has been successfully received. */
        xQueueReceive( xLcdQueue, &message, portMAX_DELAY );

        lcd.locate(message.x_offset, message.y_offset);
        lcd.printf( "%s", message.buffer );
        
        /* Loop back to wait for the next message. */
    }
}




void vDemand_Sensor_Task(void *pvParameters){
    Sensor_Message_t readings;
    readings.temperature =  0.0f;
    readings.luminosity = 0;

    for( ;; ){
        if ( ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 0){

            xSemaphoreTake( Sensor_Mutex, portMAX_DELAY );

            if (sensor.open()) 
                readings.temperature =  (float)sensor.temp();


            readings.luminosity = (uint8_t) (pot1 * 3);
            xSemaphoreGive( Sensor_Mutex );

            xQueueSendToBack( xSensorQueue, &readings, 0 );
        }
    }
}

void vPeriodic_Sensor_Task(void *pvParameters){
    TickType_t xLastWakeTime;   
    BaseType_t xStatus;
    
    // Flags
    uint8_t ACTIVATE_ALARM;

    // LCD message
    LCD_Message_t message;
    message.x_offset = 0;
    message.y_offset = 16;

    // UI message
    Sensor_Message_t readings;
    readings.temperature =  0.0f;
    readings.luminosity = 0;

    for( ;; ){
        xLastWakeTime = xTaskGetTickCount();
        ACTIVATE_ALARM = 0;
        // Se PMON == 0 E se nao for on-demand
        
        // If PMON is 0 keep on verifying || or just use the suspend/resume
        while (PMON == 0)
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( 1000 ) );
        
        xSemaphoreTake( Sensor_Mutex, portMAX_DELAY );
        if (sensor.open()) 
            readings.temperature =  (float)sensor.temp();

        readings.luminosity = (uint8_t) (pot1 * 3);

        xSemaphoreGive( Sensor_Mutex );

        xSemaphoreTake( Record_Mutex, portMAX_DELAY );
        if (readings.temperature != records[ri].temperature || readings.luminosity != records[ri].luminosity) {

            // Store record in the write index record
            records[wi].hours = CLKH;
            records[wi].minutes = CLKM;
            records[wi].seconds = CLKS;
            records[wi].temperature = readings.temperature;

            records[wi].luminosity = readings.luminosity;

            // Update read and write indices
            ri = wi;
            wi = Get_Next_Wi(wi);

            if (valid_records < 20)
                valid_records++;
        }
        xSemaphoreGive( Record_Mutex );


        if (readings.temperature >= ALAT) 
            ACTIVATE_ALARM = 1;

        if (readings.luminosity >= ALAL) 
            ACTIVATE_ALARM = 1;

        if (ALAF && ACTIVATE_ALARM && !ALARM)
            Activate_Alarm();

        
        // Print message to the LCD
        sprintf(message.buffer, "%.1f C                 %1d l", readings.temperature, readings.luminosity);
        xQueueSendToBack( xLcdQueue, &message, 0 );

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( PMON * 1000 ) );
    }
}


void vOnDemand_Processing_Task(void *pvParameters){
    Processing_Message_t metrics;
    TimeStamp_t timestamps;

    for( ;; ){

            xQueueReceive( xTimeStampsQueue, &timestamps, portMAX_DELAY );

            metrics.temperature_max = 0;
            metrics.temperature_min = 50;
            metrics.temperature_mean = 0;

            metrics.luminosity_max = 0;
            metrics.luminosity_min = 3;
            metrics.luminosity_mean = 0;

            xSemaphoreTake( Record_Mutex, portMAX_DELAY );

            uint8_t record_count = 0;
            for (uint8_t i = 0; i < valid_records; ++i) {

                // Check if the record is within the time range
                if (isWithinTimeRange(records[i].hours, records[i].minutes, records[i].seconds , timestamps)) {
                    metrics.temperature_mean += records[i].temperature;
                    metrics.luminosity_mean += records[i].luminosity;

                    printf("Index %d belongs to the timeframe\n", i);

                    if (metrics.temperature_max <= records[i].temperature)
                        metrics.temperature_max = records[i].temperature;
                    
                    if (metrics.temperature_min >= records[i].temperature)
                        metrics.temperature_min = records[i].temperature;

                    if (metrics.luminosity_max <= records[i].luminosity)
                        metrics.luminosity_max = records[i].luminosity;

                    if (metrics.luminosity_min >= records[i].luminosity)
                        metrics.luminosity_min = records[i].luminosity;

                    record_count++;
                }
            }

            if (record_count > 0) {
                metrics.temperature_mean = metrics.temperature_mean / record_count;
                metrics.luminosity_mean = metrics.luminosity_mean / record_count;
            }

            xSemaphoreGive( Record_Mutex );

            xQueueSendToBack( xProcessingQueue, &metrics, 0 );
        
    }
}

void vPeriodic_Processing_Task(void *pvParameters){
    TickType_t xLastWakeTime;   
    
    for( ;; ){
        xLastWakeTime = xTaskGetTickCount();
        
        // If PMON is 0 keep on verifying || or just use the suspend/resume
        while (PPROC == 0)
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( 1000 ) );
        

        xSemaphoreTake( Record_Mutex, portMAX_DELAY );

        if (valid_records != 0){
            if (records[ri].temperature > 25){
                g = 1.0;
                b = 1.0;
                r = 0.0;
            }
            else{
                g = 1.0;
                b = 0.0;
                r = 1.0;
            }

            led1 = 0;
            led2 = 0;
            led3 = 0;
            led4 = 0;

            switch (records[ri].luminosity) {
                case 0:
                    led1 = !led1;
                    break;

                case 1:
                    led2 = !led2;
                    break;

                case 2:
                    led3 = !led3;
                    break;

                case 3:
                    led4 = !led4;
                    break;

                default:
                    break;
            }

        }

        xSemaphoreGive( Record_Mutex );

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(  1000 ) );
    }
}

void vClock_Task(void *pvParameters){
    TickType_t xLastWakeTime;   

    // LCD Message
    LCD_Message_t message;
    message.x_offset = 0;
    message.y_offset = 0;

    for( ;; ){
        xLastWakeTime = xTaskGetTickCount();

        // Mutex around here
        xSemaphoreTake( Clock_Mutex, portMAX_DELAY );
        
        CLKS = (CLKS + 1) % 60;       

        if (CLKS == 0) {
            CLKM = (CLKM + 1) % 60;

            if (CLKM == 0) {
                CLKH = (CLKH + 1) % 24;
            }
        }
        
        xSemaphoreGive( Clock_Mutex );



        // Count the sound duration
        if (ALARM_TIMER) {
            ALARM_TIMER++;

            //Turn off sound
            if (ALARM_TIMER == TALA + 1 && TALA != 0) {
                spkr=0.0;
                ALARM_TIMER = 0;
            }
        }

        if (ALAF){
            if (ALARM) 
                sprintf(message.buffer, "%02d:%02d:%02d             CTL A", CLKH, CLKM, CLKS);   
            else
                sprintf(message.buffer, "%02d:%02d:%02d                 A", CLKH, CLKM, CLKS);   
        }         
        else{
            if (ALARM) 
                sprintf(message.buffer, "%02d:%02d:%02d             CTL a", CLKH, CLKM, CLKS);  
            else
                sprintf(message.buffer, "%02d:%02d:%02d                 a", CLKH, CLKM, CLKS);  
        }

        xQueueSendToBack( xLcdQueue, &message, 0 );

        if ((CLKH == ALAH) && (CLKM == ALAM)&& (CLKS == ALAS) && (ALAF))
            Activate_Alarm();
        

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( 1000 ) );

    }
}

// vTaskSuspend()
// vTaskResume()

int main( void ) {
    /* Perform any hardware setup necessary. */
    //prvSetupHardware();

    pc.baud(115200);

    /* --- MUTEXES --- */
    Clock_Mutex = xSemaphoreCreateMutex();

    if (Clock_Mutex == NULL)
        exit(EXIT_FAILURE);
    
    Record_Mutex = xSemaphoreCreateMutex();

    if (Record_Mutex == NULL)
        exit(EXIT_FAILURE);

    Sensor_Mutex = xSemaphoreCreateMutex();

    if (Sensor_Mutex == NULL)
        exit(EXIT_FAILURE);

    /* --- QUEUES --- */
    xSensorQueue = xQueueCreate( 6, sizeof( Sensor_Message_t ) );
    xLcdQueue= xQueueCreate( 6, sizeof( LCD_Message_t ) );;
    xProcessingQueue = xQueueCreate( 6, sizeof( Processing_Message_t ) );
    xTimeStampsQueue = xQueueCreate( 6, sizeof( TimeStamp_t ) );

    /* --- RECORD INITIALIZATION --- */
    records[0].hours = 0;
    records[0].minutes = 0;
    records[0].seconds = 0;
    records[0].temperature = 0;
    records[0].luminosity = 0;

    /* --- APPLICATION TASKS CAN BE CREATED HERE --- */
    xTaskCreate( VUI_Task, "Task 1", 2*configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    xTaskCreate( vClock_Task, "Read Clock", 2*configMINIMAL_STACK_SIZE, NULL, 4, NULL );
    xTaskCreate( vLCD_Gatekeeper_Task, "LCD Gatekeeper", 2*configMINIMAL_STACK_SIZE, NULL, 5, NULL );
    xTaskCreate( vPeriodic_Sensor_Task, "Read Sensors", 2*configMINIMAL_STACK_SIZE, NULL, 3, &xPSensorHandle );
    xTaskCreate( vDemand_Sensor_Task, "On-Demand Read Sensors", 2*configMINIMAL_STACK_SIZE, NULL, 2, &xSensorHandle );
    xTaskCreate( vPeriodic_Processing_Task, "Process Information", 2*configMINIMAL_STACK_SIZE, NULL, 3, &xPProcessingHandle );
    xTaskCreate( vOnDemand_Processing_Task, "On-Demand Process Information", 2*configMINIMAL_STACK_SIZE, NULL, 2, &xProcessingHandle );

    //vTaskSuspend(xPProcessingHandle);

    /* Start the created tasks running. */
    vTaskStartScheduler();

    /* Execution will only reach here if there was insufficient heap to
    start the scheduler. */
    for( ;; );
    return 0;
}
