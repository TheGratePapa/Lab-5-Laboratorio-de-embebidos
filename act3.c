#include <bcm2835.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>

#define NUMEVENTS 6
#define LINESIZE 60
#define STARTING_YEAR 22

char wbuf[1];
char temp[1];

uint8_t rtcsa = 0x68;
uint8_t tempsa = 0x4d;

char time[7];

char *daystr[] = {"Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat"};
char *ampm_str[] = {"AM", "PM"};
int seconds, minutes, hours, date, month, year;

//masks
uint8_t ampm_mask;

int n = 0;

bool kbhit(void)
{
    struct termios original;
    tcgetattr(STDIN_FILENO, &original);
    struct termios term;
    memcpy(&term, &original, sizeof(term));
    term.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    int characters_buffered = 0;
    ioctl(STDIN_FILENO, FIONREAD, &characters_buffered);
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    bool pressed = (characters_buffered != 0);
    return pressed;
}

int get_format(char var){
    int dec = (var & 0b01110000) >> 4;
    int uni = (var & 0b00001111);
    int sec = (dec * 10)+ uni;
    return sec;
}

uint8_t get_ampm(char var){
    int uni = (var & 0b00100000) >> 5;

    return uni;
}


void get_time() {
    // Slave address del RTC
    bcm2835_i2c_setSlaveAddress(rtcsa);
    // Address de los registors
    wbuf[0] = 0;
    bcm2835_i2c_write(wbuf, 1);
    // Leer 7 registros de tiempo
    bcm2835_i2c_read(time, 7);
}

void get_temp() {
    // Slave address del RTC
    bcm2835_i2c_setSlaveAddress(tempsa);
    // Address de los registors
    wbuf[0] = 0;
    bcm2835_i2c_write(wbuf, 1);
    // Leer temperatura
    bcm2835_i2c_read(temp, 1);
}

void fill_events(FILE *file, char events[][LINESIZE]) {
    for (int i = 0; i < NUMEVENTS; i++) {
        fgets(events[i], LINESIZE, file);
    }
}

void log_event(FILE **file, char events[][LINESIZE], char event[LINESIZE]) {
    //printf("%s", event);
    memmove(&events[0], &events[1], (NUMEVENTS - 1) * sizeof(events[0]));
    strcpy(events[NUMEVENTS - 1], event);
    *file = freopen("logs.txt", "w", *file);
    for (int i = 0; i < NUMEVENTS; i++) {
        fputs(events[i], *file);
    }
}


int main() {
    FILE *file;
    char events[NUMEVENTS][LINESIZE];
    char event[LINESIZE];
    uint8_t values[10];
    int sec = 0;
    char out_temp[30];
    char out_date[30];
 

    if (!bcm2835_init()) {
        printf("bcm2835_init failed. Are you running as root??\n");
        return 1;
    }

    if (!bcm2835_i2c_begin()) {
        printf("bcm2835_i2c_begin failed. Are you running as root??\n");
        return 1;
    }

    file = fopen("logs.txt", "r");

    if (file == NULL) {
        printf("Error al abrir archivo");
        return 1;
    }
    
  
    
     


    //Function to log and print
    printf("Press '2' to exit\n");
    while(1)
    {
       
        get_temp();
        get_time();
        seconds = get_format(time[0]);
        minutes = get_format(time[1]);
        hours = get_format(time[2]);
        date = get_format(time[4]);
        month = get_format(time[5]);
        year = get_format(time[6]) + STARTING_YEAR;
        ampm_mask = get_ampm(time[2]);

        if( temp[0] > 30 || sec == 0)
        {
           
            sprintf(out_temp, "RECEIVER> Temperature: %d C\n", temp[0]);  
            printf("%s",out_temp); 
            fill_events(file, events);  // Guardar eventos de archivo en array
            strcpy(event, out_temp);  // reemplazar con nuevo evento
            log_event(&file, events, event);
            
            sprintf(out_date, "RECEIVER> Record %d: %d/%d/%d %s %x:%d:%d %s\n\n", n++, date, month, year, daystr[time[3]], hours, minutes, seconds, ampm_str[ampm_mask]);
            printf("%s",out_date);
            fill_events(file, events);  // Guardar eventos de archivo en array
            strcpy(event, out_date);  // reemplazar con nuevo evento
            log_event(&file, events, event);
            
            sec = 10;
            delay(1000);    
        }
        else
        {
            delay(1000);  
            sec --;
        }
        
        
        if (kbhit()) 
        {
            int k = getchar();
                if(k=='2') break;
        }
    }

    
    
    bcm2835_close();
    fclose(file);
    return 0;
}
