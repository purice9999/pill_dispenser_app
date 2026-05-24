/*
 * Pill Dispenser - PIC16F887 Firmware
 * Microcontroller: PIC16F887
 * Communication: UART with HM-10 BLE Module
 * 
 * Features:
 * - Receive alarm commands from mobile app via BLE
 * - Parse command format: "DAY MOMENT TIME"
 * - Set internal alarm
 * - Trigger alarm at scheduled time
 * - Send confirmation back to app
 * - Control LED and Buzzer
 * 
 * Date: May 24, 2026
 * Version: 1.0
 */

#include <xc.h>
#include <string.h>
#include <stdio.h>

// Configuration bits
#pragma config FOSC = INTRC_NOCLKOUT
#pragma config WDTE = OFF
#pragma config PWRTE = OFF
#pragma config MCLRE = ON
#pragma config CP = OFF
#pragma config CPD = OFF
#pragma config BOREN = OFF
#pragma config IESO = OFF
#pragma config FCMEN = OFF
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000

// Pin definitions
#define LED_DAY1 RB0
#define LED_DAY2 RB1
#define LED_DAY3 RB2
#define LED_DAY4 RB3
#define LED_DAY5 RB4
#define LED_DAY6 RB5
#define LED_DAY7 RB6
#define BUZZER RB7

// UART definitions
#define UART_BAUD 9600

// Alarm structure
typedef struct {
    unsigned char day;      // 0-6 (Monday-Sunday)
    unsigned char moment;   // 0-2 (Morning, Afternoon, Evening)
    unsigned char hour;
    unsigned char minute;
    unsigned char active;
} Alarm;

// Global variables
Alarm alarms[7];            // One alarm per day
unsigned char uart_buffer[50];
unsigned char uart_index = 0;
unsigned char current_day = 0;
unsigned char current_hour = 0;
unsigned char current_minute = 0;

// Function prototypes
void UART_Init(void);
void UART_Send_Char(unsigned char c);
void UART_Send_String(const char *str);
void UART_Receive_Interrupt(void);
void Parse_Command(void);
void Set_Alarm(unsigned char day, unsigned char moment, unsigned char hour, unsigned char minute);
void Check_Alarm(void);
void Trigger_Alarm(void);
void LED_Indicate_Day(unsigned char day);
void Buzzer_On(void);
void Buzzer_Off(void);
void Timer_Init(void);
void Update_Time(void);

/*
 * UART Initialization
 * Baud rate: 9600
 * 8 data bits, 1 stop bit, no parity
 */
void UART_Init(void) {
    // Set UART pins
    TRISC6 = 1;  // TX as input (UART handles it)
    TRISC7 = 1;  // RX as input
    
    // UART configuration
    SPBRG = (_XTAL_FREQ / (16 * UART_BAUD)) - 1;  // Baud rate
    TXSTA = 0x24;  // TX enabled, 8-bit, async mode
    RCSTA = 0x90;  // RX enabled, 8-bit
    
    // Enable UART interrupts
    PIE1bits.RCIE = 1;  // RX interrupt enable
    INTCONbits.PEIE = 1;  // Peripheral interrupt enable
    INTCONbits.GIE = 1;   // Global interrupt enable
}

/*
 * Send single character via UART
 */
void UART_Send_Char(unsigned char c) {
    while (!TXSTAbits.TRMT);  // Wait for transmit buffer empty
    TXREG = c;
}

/*
 * Send string via UART
 */
void UART_Send_String(const char *str) {
    while (*str) {
        UART_Send_Char(*str++);
    }
}

/*
 * UART Receive Interrupt Handler
 * Called when character received from HM-10
 */
void interrupt UART_ISR(void) {
    if (PIR1bits.RCIF) {  // RX interrupt
        unsigned char received = RCREG;
        
        if (received == '\n' || received == '\r') {
            // End of command
            uart_buffer[uart_index] = '\0';
            Parse_Command();
            uart_index = 0;
        } else if (uart_index < 49) {
            uart_buffer[uart_index++] = received;
        }
        
        PIR1bits.RCIF = 0;  // Clear interrupt flag
    }
}

/*
 * Parse command from mobile app
 * Format: "DAY MOMENT TIME"
 * Example: "Luni Dimineata 08:30"
 * 
 * Days: Luni(0), Marti(1), Miercuri(2), Joi(3), Vineri(4), Sambata(5), Duminica(6)
 * Moments: Dimineata(0), Pranz(1), Seara(2)
 * Time: HH:MM
 */
void Parse_Command(void) {
    unsigned char day = 0xFF;
    unsigned char moment = 0xFF;
    unsigned char hour = 0;
    unsigned char minute = 0;
    
    // Parse day
    if (strstr((char*)uart_buffer, "Luni")) day = 0;
    else if (strstr((char*)uart_buffer, "Marti")) day = 1;
    else if (strstr((char*)uart_buffer, "Miercuri")) day = 2;
    else if (strstr((char*)uart_buffer, "Joi")) day = 3;
    else if (strstr((char*)uart_buffer, "Vineri")) day = 4;
    else if (strstr((char*)uart_buffer, "Sambata")) day = 5;
    else if (strstr((char*)uart_buffer, "Duminica")) day = 6;
    
    // Parse moment
    if (strstr((char*)uart_buffer, "Dimineata")) moment = 0;
    else if (strstr((char*)uart_buffer, "Pranz")) moment = 1;
    else if (strstr((char*)uart_buffer, "Seara")) moment = 2;
    
    // Parse time (HH:MM)
    unsigned char *time_ptr = strchr((char*)uart_buffer, ':');
    if (time_ptr) {
        hour = (*(time_ptr - 2) - '0') * 10 + (*(time_ptr - 1) - '0');
        minute = (*(time_ptr + 1) - '0') * 10 + (*(time_ptr + 2) - '0');
    }
    
    // Validate and set alarm
    if (day != 0xFF && moment != 0xFF) {
        Set_Alarm(day, moment, hour, minute);
        UART_Send_String("Alarmă setată\r\n");
    } else {
        UART_Send_String("Eroare: Format invalid\r\n");
    }
}

/*
 * Set alarm for specific day and time
 */
void Set_Alarm(unsigned char day, unsigned char moment, unsigned char hour, unsigned char minute) {
    alarms[day].day = day;
    alarms[day].moment = moment;
    alarms[day].hour = hour;
    alarms[day].minute = minute;
    alarms[day].active = 1;
    
    // Indicate on LED
    LED_Indicate_Day(day);
}

/*
 * Check if alarm should trigger
 * Called periodically (every minute)
 */
void Check_Alarm(void) {
    // Check if today's alarm matches current time
    if (alarms[current_day].active) {
        if (alarms[current_day].hour == current_hour && 
            alarms[current_day].minute == current_minute) {
            Trigger_Alarm();
        }
    }
}

/*
 * Trigger alarm
 * - Turn on buzzer
 * - Send confirmation to app
 * - Wait for user confirmation
 */
void Trigger_Alarm(void) {
    Buzzer_On();
    __delay_ms(500);
    Buzzer_Off();
    __delay_ms(500);
    Buzzer_On();
    __delay_ms(500);
    Buzzer_Off();
    
    // Send confirmation
    UART_Send_String("Pastila luata\r\n");
    
    // Deactivate alarm for today
    alarms[current_day].active = 0;
}

/*
 * Indicate day on LED
 * Turn on LED corresponding to day
 */
void LED_Indicate_Day(unsigned char day) {
    // Turn off all LEDs
    PORTB = 0x00;
    
    // Turn on LED for specific day
    switch (day) {
        case 0: LED_DAY1 = 1; break;  // Monday
        case 1: LED_DAY2 = 1; break;  // Tuesday
        case 2: LED_DAY3 = 1; break;  // Wednesday
        case 3: LED_DAY4 = 1; break;  // Thursday
        case 4: LED_DAY5 = 1; break;  // Friday
        case 5: LED_DAY6 = 1; break;  // Saturday
        case 6: LED_DAY7 = 1; break;  // Sunday
    }
}

/*
 * Turn on buzzer
 */
void Buzzer_On(void) {
    BUZZER = 1;
}

/*
 * Turn off buzzer
 */
void Buzzer_Off(void) {
    BUZZER = 0;
}

/*
 * Timer initialization for time keeping
 * Uses Timer0 for 1-second interrupt
 */
void Timer_Init(void) {
    // Timer0 configuration
    OPTION_REGbits.T0CS = 0;  // Internal clock
    OPTION_REGbits.PSA = 0;   // Prescaler assigned to Timer0
    OPTION_REGbits.PS = 0b111;  // 1:256 prescaler
    
    // Enable Timer0 interrupt
    INTCONbits.T0IE = 1;
    INTCONbits.GIE = 1;
}

/*
 * Update time every second
 * Called from Timer0 interrupt
 */
void Update_Time(void) {
    current_minute++;
    if (current_minute >= 60) {
        current_minute = 0;
        current_hour++;
        if (current_hour >= 24) {
            current_hour = 0;
            current_day++;
            if (current_day >= 7) {
                current_day = 0;
            }
        }
    }
    
    // Check alarm every minute
    if (current_minute == 0) {
        Check_Alarm();
    }
}

/*
 * Main program
 */
void main(void) {
    // Initialize I/O
    TRISB = 0x00;  // PORTB as output (LEDs and Buzzer)
    PORTB = 0x00;
    
    // Initialize modules
    UART_Init();
    Timer_Init();
    
    // Initialize alarms
    for (unsigned char i = 0; i < 7; i++) {
        alarms[i].active = 0;
    }
    
    // Send startup message
    UART_Send_String("Pill Dispenser Ready\r\n");
    
    // Main loop
    while (1) {
        // Main program runs here
        // Interrupts handle UART and Timer
        __delay_ms(100);
    }
}

/*
 * Timer0 Interrupt Handler
 * Triggers every ~1 second
 */
void interrupt Timer0_ISR(void) {
    if (INTCONbits.T0IF) {
        Update_Time();
        INTCONbits.T0IF = 0;  // Clear interrupt flag
    }
}

/*
 * COMMUNICATION PROTOCOL EXAMPLE
 * 
 * 1. MOBILE APP SENDS:
 *    "Luni Dimineata 08:30\r\n"
 * 
 * 2. PIC RECEIVES AND PARSES:
 *    - Day: Luni (0)
 *    - Moment: Dimineata (0)
 *    - Time: 08:30
 * 
 * 3. PIC RESPONDS:
 *    "Alarmă setată\r\n"
 * 
 * 4. AT 08:30 ON MONDAY:
 *    - Buzzer triggers (3 beeps)
 *    - LED for Monday turns on
 *    - PIC sends: "Pastila luata\r\n"
 * 
 * 5. MOBILE APP RECEIVES:
 *    - Saves to history
 *    - Shows notification
 */

/*
 * NOTES FOR IMPLEMENTATION:
 * 
 * 1. Real-time clock (RTC) module recommended for accurate time keeping
 *    - DS1307 or similar I2C RTC
 *    - Maintains time even when powered off
 * 
 * 2. EEPROM storage for alarm persistence
 *    - Save alarms to EEPROM
 *    - Restore on power-up
 * 
 * 3. Power management
 *    - Use sleep mode when not needed
 *    - Wake up on alarm time
 *    - Reduces power consumption
 * 
 * 4. Error handling
 *    - Validate all received commands
 *    - Send error messages for invalid input
 *    - Log errors to EEPROM
 * 
 * 5. Testing
 *    - Test with HM-10 module
 *    - Verify UART communication
 *    - Test alarm triggering
 *    - Verify LED and Buzzer operation
 */
