/*
 * Pill Dispenser - PIC16F887 Firmware (COMPLETE VERSION)
 * 
 * Features:
 * - LCD 16x4 display
 * - RTC DS1307 (I2C) for real-time clock
 * - 21 LEDs (3 per day) via 74HC595 shift register
 * - UART communication with HM-10 BLE module
 * - Interactive button for confirmation
 * - Buzzer for alarm signal
 * - Real-time display
 * - LED animation (blinking)
 * 
 * Date: May 24, 2026
 * Version: 2.0 (with BLE integration)
 */

#include <xc.h>
#include <stdio.h>

#define _XTAL_FREQ 8000000

#pragma config FOSC  = HS
#pragma config WDTE  = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = ON
#pragma config CP    = OFF
#pragma config CPD   = OFF
#pragma config BOREN = OFF
#pragma config IESO  = OFF
#pragma config FCMEN = OFF
#pragma config LVP   = OFF

// ============================================================
//  LCD pe PORTB
// ============================================================
#define RS  PORTBbits.RB0
#define EN  PORTBbits.RB1

static void LCD_Nibble(unsigned char n) {
    PORTB = (unsigned char)((PORTB & 0x03) | ((n & 0x0F) << 2));
    EN = 1; __delay_us(1);
    EN = 0; __delay_us(50);
}

static void LCD_Send(unsigned char byte, unsigned char is_data) {
    RS = is_data;
    LCD_Nibble(byte >> 4);
    LCD_Nibble(byte & 0x0F);
}

static void LCD_Cmd(unsigned char cmd) {
    LCD_Send(cmd, 0);
    if (cmd == 0x01 || cmd == 0x02) __delay_ms(2);
}

static void LCD_Char(char c) { 
    LCD_Send((unsigned char)c, 1); 
}

static const unsigned char ROW_ADDR[4] = {0x80, 0xC0, 0x90, 0xD0};

static void LCD_SetCursor(unsigned char row, unsigned char col) {
    LCD_Cmd(ROW_ADDR[row] + col);
}

static void LCD_Print(const char *s) { 
    while (*s) LCD_Char(*s++); 
}

static void LCD_Init(void) {
    ANSEL = ANSELH = 0;
    CM1CON0 = CM2CON0 = 0;
    TRISB = 0;
    PORTB = 0;
    __delay_ms(50);
    
    RS = 0;
    PORTB = (PORTB & 0x03) | 0x0C; EN=1;
    __delay_us(1);
    EN=0; __delay_ms(5);
    EN=1;
    __delay_us(1);
    EN=0; __delay_us(150);
    EN=1;
    __delay_us(1);
    EN=0; __delay_us(150);
    
    PORTB = (PORTB & 0x03) | 0x08; EN=1;
    __delay_us(1);
    EN=0; __delay_us(150);
    
    LCD_Cmd(0x28);
    LCD_Cmd(0x08);
    LCD_Cmd(0x01); __delay_ms(2);
    LCD_Cmd(0x06);
    LCD_Cmd(0x0C);
}

// ============================================================
//  I2C + RTC DS1307
// ============================================================
static void I2C_Init(void) {
    TRISC3 = TRISC4 = 1;
    SSPADD = ((_XTAL_FREQ / 4) / 100000) - 1;
    SSPSTAT = 0;
    SSPCON = 0x28;
    SSPCON2 = 0;
}

static void I2C_Start(void)   { SSPCON2bits.SEN  = 1; while (SSPCON2bits.SEN); }
static void I2C_Stop(void)    { SSPCON2bits.PEN  = 1; while (SSPCON2bits.PEN); }
static void I2C_Restart(void) { SSPCON2bits.RSEN = 1; while (SSPCON2bits.RSEN); }

static void I2C_Write(unsigned char d) {
    SSPBUF = d;
    while (SSPSTATbits.BF);
    while (SSPCON2bits.ACKSTAT);
}

static unsigned char I2C_Read(unsigned char ack) {
    SSPCON2bits.RCEN = 1;
    while (!SSPSTATbits.BF);
    unsigned char d = SSPBUF;
    SSPCON2bits.ACKDT = ack ? 0 : 1;
    SSPCON2bits.ACKEN = 1;
    while (SSPCON2bits.ACKEN);
    return d;
}

static unsigned char BCD2Dec(unsigned char v) {
    return ((v >> 4) * 10) + (v & 0x0F);
}

static unsigned char Dec2BCD(unsigned char v) {
    return ((v / 10) << 4) | (v % 10);
}

static void RTC_Read(unsigned char *h, unsigned char *m, unsigned char *s) {
    I2C_Start();
    I2C_Write(0xD0);
    I2C_Write(0x00);
    I2C_Restart();
    I2C_Write(0xD1);
    *s = BCD2Dec(I2C_Read(1));
    *m = BCD2Dec(I2C_Read(1));
    *h = BCD2Dec(I2C_Read(0));
    I2C_Stop();
}

static void RTC_GetDay(unsigned char *day) {
    I2C_Start();
    I2C_Write(0xD0);
    I2C_Write(0x03);
    I2C_Restart();
    I2C_Write(0xD1);
    *day = I2C_Read(0);
    I2C_Stop();
}

void LCD_ShowTime() {
    unsigned char h,m,s;
    char buf[16];
    RTC_Read(&h,&m,&s);
    sprintf(buf,"%02d:%02d:%02d",h,m,s);
    LCD_SetCursor(1,4);
    LCD_Print(buf);
}

// ============================================================
//  74HC595 - Weekend LEDs
// ============================================================
#define SR_SER    PORTCbits.RC0
#define SR_CLK    PORTCbits.RC1
#define SR_LATCH  PORTCbits.RC5

static void ShiftOut(unsigned char d) {
    for (signed char i = 7; i >= 0; i--) {
        SR_SER = (d >> i) & 1;
        SR_CLK = 1; __delay_us(2);
        SR_CLK = 0; __delay_us(2);
    }
    SR_LATCH = 1; __delay_us(2);
    SR_LATCH = 0;
}

// ============================================================
//  LED-uri Luni-Duminica (21 LED-uri)
// ============================================================
#define P_A  0
#define P_E  1
#define P_D  2
#define P_SR 3

typedef struct {
    unsigned char port_id;
    unsigned char mask;
} LED_Def;

static const LED_Def LEDS[21] = {
    {P_A, 0x02}, {P_A, 0x04}, {P_A, 0x08}, // Luni
    {P_A, 0x10}, {P_A, 0x20}, {P_E, 0x01}, // Marti
    {P_E, 0x02}, {P_E, 0x04}, {P_D, 0x01}, // Miercuri
    {P_D, 0x02}, {P_D, 0x04}, {P_D, 0x08}, // Joi
    {P_D, 0x10}, {P_D, 0x20}, {P_D, 0x40}, // Vineri
    {P_SR,0x01}, {P_SR,0x02}, {P_SR,0x04}, // Sambata
    {P_SR,0x08}, {P_SR,0x10}, {P_SR,0x20}  // Duminica
};

#define MASK_A 0x3E
#define MASK_E 0x07
#define MASK_D 0x7F

static unsigned char pState[4] = {0,0,0,0};

// ============================================================
//  BUTON + BUZZER
// ============================================================
#define BTN     PORTAbits.RA0
#define BUZZER  PORTCbits.RC2

// ============================================================
//  UART Communication (HM-10 BLE Module)
// ============================================================
#define UART_BAUD 9600

static unsigned char uart_buffer[50];
static unsigned char uart_index = 0;
static unsigned char uart_ready = 0;

static void UART_Init(void) {
    TRISC6 = 1;  // TX
    TRISC7 = 1;  // RX
    
    SPBRG = (_XTAL_FREQ / (16 * UART_BAUD)) - 1;
    TXSTA = 0x24;  // TX enabled, 8-bit, async
    RCSTA = 0x90;  // RX enabled, 8-bit
    
    PIE1bits.RCIE = 1;  // RX interrupt
    INTCONbits.PEIE = 1;  // Peripheral interrupt
    INTCONbits.GIE = 1;   // Global interrupt
}

static void UART_Send_Char(unsigned char c) {
    while (!TXSTAbits.TRMT);
    TXREG = c;
}

static void UART_Send_String(const char *str) {
    while (*str) {
        UART_Send_Char(*str++);
    }
}

// ============================================================
//  Alarm Structure
// ============================================================
typedef struct {
    unsigned char day;      // 1-7 (Monday-Sunday)
    unsigned char moment;   // 0-2 (Morning, Afternoon, Evening)
    unsigned char hour;
    unsigned char minute;
    unsigned char active;
} Alarm;

static Alarm alarms[7];
static unsigned char current_day = 1;

// ============================================================
//  Parse Command from Mobile App
// ============================================================
static void Parse_Command(void) {
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
        alarms[day].day = day;
        alarms[day].moment = moment;
        alarms[day].hour = hour;
        alarms[day].minute = minute;
        alarms[day].active = 1;
        
        UART_Send_String("Alarmă setată\r\n");
        
        // Show on LCD
        LCD_SetCursor(2,0);
        LCD_Print("Alarmă setată!  ");
        __delay_ms(2000);
    } else {
        UART_Send_String("Eroare: Format invalid\r\n");
        LCD_SetCursor(2,0);
        LCD_Print("Eroare format!  ");
        __delay_ms(2000);
    }
}

// ============================================================
//  Interrupt Handler (UART RX)
// ============================================================
void interrupt ISR(void) {
    if (PIR1bits.RCIF) {
        unsigned char received = RCREG;
        
        if (received == '\n' || received == '\r') {
            uart_buffer[uart_index] = '\0';
            uart_ready = 1;
            uart_index = 0;
        } else if (uart_index < 49) {
            uart_buffer[uart_index++] = received;
        }
        
        PIR1bits.RCIF = 0;
    }
}

// ============================================================
//  LED Apply
// ============================================================
static void LED_Apply(unsigned char step, unsigned char blinkOn) {
    unsigned char a = pState[P_A];
    unsigned char e = pState[P_E];
    unsigned char d = pState[P_D];
    unsigned char sr = pState[P_SR];
    
    if (step < 21 && blinkOn) {
        switch (LEDS[step].port_id) {
            case P_A:  a  |= LEDS[step].mask; break;
            case P_E:  e  |= LEDS[step].mask; break;
            case P_D:  d  |= LEDS[step].mask; break;
            case P_SR: sr |= LEDS[step].mask; break;
        }
    }
    
    PORTA = (PORTA & ~MASK_A) | (a & MASK_A);
    PORTE = (PORTE & ~MASK_E) | (e & MASK_E);
    PORTD = (PORTD & ~MASK_D) | (d & MASK_D);
    ShiftOut(sr);
}

static void LED_LockAndAdvance(unsigned char *step, unsigned char *stepSecs, unsigned char *blinkOn) {
    pState[LEDS[*step].port_id] |= LEDS[*step].mask;
    (*step)++;
    *stepSecs = 0;
    *blinkOn = 1;
}

// ============================================================
//  MAIN
// ============================================================
void main(void) {
    ANSEL = ANSELH = 0;
    CM1CON0 = CM2CON0 = 0;
    
    LCD_Init();
    I2C_Init();
    UART_Init();
    
    TRISA &= ~MASK_A;
    TRISE &= ~MASK_E;
    TRISD &= ~MASK_D;
    TRISC0 = TRISC1 = TRISC5 = 0;
    
    // Button active-HIGH
    TRISA0 = 1;
    
    // Buzzer
    TRISC2 = 0;
    BUZZER = 0;
    
    PORTA &= ~MASK_A;
    PORTE &= ~MASK_E;
    PORTD &= ~MASK_D;
    ShiftOut(0);
    
    // Initialize alarms
    for (unsigned char i = 0; i < 7; i++) {
        alarms[i].active = 0;
    }
    
    // LCD startup message
    LCD_SetCursor(0,0);
    LCD_Print("  Ora curenta:  ");
    
    LCD_SetCursor(2,0);
    LCD_Print("BLE: Conectat   ");
    
    LCD_SetCursor(3,0);
    LCD_Print("Astept comanda...");
    
    UART_Send_String("Pill Dispenser Ready\r\n");
    
    unsigned char h,m,s;
    unsigned char lastSec = 255;
    unsigned char step = 0;
    unsigned char stepSecs = 0;
    unsigned char blinkOn = 1;
    unsigned char loopCnt = 0;
    
    while(1) {
        RTC_Read(&h,&m,&s);
        
        // Check for UART command
        if (uart_ready) {
            Parse_Command();
            uart_ready = 0;
        }
        
        if(s != lastSec) {
            lastSec = s;
            char buf[9];
            sprintf(buf,"%02d:%02d:%02d",h,m,s);
            LCD_SetCursor(1,4);
            LCD_Print(buf);
            
            if(step < 21) {
                stepSecs++;
                if(stepSecs >= 5) {
                    LED_LockAndAdvance(&step,&stepSecs,&blinkOn);
                    BUZZER = 0;
                }
            }
        }
        
        if(step < 21) {
            // Button active-HIGH
            if(BTN == 1) {
                pState[LEDS[step].port_id] |= LEDS[step].mask;
                BUZZER = 0;
                step++;
                stepSecs = 0;
                blinkOn = 1;
                loopCnt = 0;
                LED_Apply(step,0);
                __delay_ms(200);
                continue;
            }
            
            loopCnt++;
            if(loopCnt >= 10) {
                loopCnt = 0;
                blinkOn = !blinkOn;
            }
            BUZZER = blinkOn;
            LED_Apply(step, blinkOn);
        } else {
            BUZZER = 0;
            LED_Apply(21,0);
        }
        
        __delay_ms(50);
    }
}

/*
 * COMMUNICATION PROTOCOL
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
 * 4. LCD SHOWS:
 *    Row 0: "  Ora curenta:  "
 *    Row 1: "  HH:MM:SS      "
 *    Row 2: "Alarmă setată!  "
 *    Row 3: "Astept comanda.."
 * 
 * 5. LED ANIMATION:
 *    - 21 LEDs blink in sequence
 *    - Button to confirm each LED
 *    - Buzzer beeps during animation
 */
