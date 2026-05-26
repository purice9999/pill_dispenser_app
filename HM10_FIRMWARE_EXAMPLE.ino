// ============================================================
//  Dozator de Pastile — PIC16F887 + HM-10 BLE
//  Comunicare cu aplicatia Flutter prin UART @ 9600 baud
//
//  Protocol BLE:
//    App -> PIC : "Luni Dimineata 08:30\r\n"
//    PIC -> App : "OK\r\n"              (alarma salvata)
//    PIC -> App : "Pastila luata\r\n"   (buton apasat)
// ============================================================

#include <xc.h>
#include <stdio.h>
#include <string.h>

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
//  LCD pe PORTB (4-bit)
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

static void LCD_Char(char c) { LCD_Send((unsigned char)c, 1); }

static const unsigned char ROW_ADDR[4] = {0x80, 0xC0, 0x90, 0xD0};

static void LCD_SetCursor(unsigned char row, unsigned char col) {
    LCD_Cmd(ROW_ADDR[row] + col);
}

static void LCD_Print(const char *s) { while (*s) LCD_Char(*s++); }

static void LCD_Init(void) {
    ANSEL = ANSELH = 0;
    CM1CON0 = CM2CON0 = 0;
    TRISB = 0;
    PORTB = 0;

    __delay_ms(50);
    RS = 0;

    PORTB = (PORTB & 0x03) | 0x0C; EN=1;__delay_us(1);EN=0; __delay_ms(5);
                                    EN=1;__delay_us(1);EN=0; __delay_us(150);
                                    EN=1;__delay_us(1);EN=0; __delay_us(150);

    PORTB = (PORTB & 0x03) | 0x08; EN=1;__delay_us(1);EN=0; __delay_us(150);

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

// Citeste ora + ziua saptamanii din DS1307
// DS1307 registri: 0=sec, 1=min, 2=ora, 3=zi_sapt
// Conventie europeana: 1=Luni, 2=Marti, ..., 7=Duminica
// *dow = 0..6 (0=Luni, 6=Duminica), identic cu indexul din aplicatie
static void RTC_Read(unsigned char *h, unsigned char *m, unsigned char *s,
                     unsigned char *dow) {
    I2C_Start();
    I2C_Write(0xD0);
    I2C_Write(0x00);
    I2C_Restart();
    I2C_Write(0xD1);
    *s   = BCD2Dec(I2C_Read(1));
    *m   = BCD2Dec(I2C_Read(1));
    *h   = BCD2Dec(I2C_Read(1));
    *dow = I2C_Read(0);          // 1-7, nu BCD
    if (*dow > 0) (*dow)--;      // Transforma in 0-6 (0=Luni)
    I2C_Stop();
}

// ============================================================
//  74HC595 — LED-uri Sambata + Duminica
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
//  LED-uri — 21 pozitii: 7 zile x 3 momente
//  Index = zi(0-6) * 3 + moment(0-2)
//  Moment: 0=Dimineata, 1=Pranz, 2=Seara
// ============================================================
#define P_A  0
#define P_E  1
#define P_D  2
#define P_SR 3

typedef struct { unsigned char port_id; unsigned char mask; } LED_Def;

static const LED_Def LEDS[21] = {
    {P_A, 0x02},{P_A, 0x04},{P_A, 0x08},   // Luni:     Dim/Pranz/Seara
    {P_A, 0x10},{P_A, 0x20},{P_E, 0x01},   // Marti:    Dim/Pranz/Seara
    {P_E, 0x02},{P_E, 0x04},{P_D, 0x01},   // Miercuri: Dim/Pranz/Seara
    {P_D, 0x02},{P_D, 0x04},{P_D, 0x08},   // Joi:      Dim/Pranz/Seara
    {P_D, 0x10},{P_D, 0x20},{P_D, 0x40},   // Vineri:   Dim/Pranz/Seara
    {P_SR,0x01},{P_SR,0x02},{P_SR,0x04},   // Sambata:  Dim/Pranz/Seara
    {P_SR,0x08},{P_SR,0x10},{P_SR,0x20}    // Duminica: Dim/Pranz/Seara
};

#define MASK_A 0x3E
#define MASK_E 0x07
#define MASK_D 0x7F

// Starea permanenta (LED-uri confirmate = pastile luate)
static unsigned char pState[4] = {0, 0, 0, 0};

// ============================================================
//  BUTON + BUZZER
// ============================================================
#define BTN     PORTAbits.RA0   // Active-HIGH
#define BUZZER  PORTCbits.RC2

// ============================================================
//  Aplica starea LED-urilor
//  activeIdx: indexul LED-ului care clipeste (-1 = niciun blink)
// ============================================================
static void LED_Apply(signed char activeIdx, unsigned char blinkOn) {
    unsigned char a  = pState[P_A];
    unsigned char e  = pState[P_E];
    unsigned char d  = pState[P_D];
    unsigned char sr = pState[P_SR];

    if (activeIdx >= 0 && blinkOn) {
        switch (LEDS[activeIdx].port_id) {
            case P_A:  a  |= LEDS[activeIdx].mask; break;
            case P_E:  e  |= LEDS[activeIdx].mask; break;
            case P_D:  d  |= LEDS[activeIdx].mask; break;
            case P_SR: sr |= LEDS[activeIdx].mask; break;
        }
    }

    PORTA = (PORTA & ~MASK_A) | (a & MASK_A);
    PORTE = (PORTE & ~MASK_E) | (e & MASK_E);
    PORTD = (PORTD & ~MASK_D) | (d & MASK_D);
    ShiftOut(sr);
}

// ============================================================
//  UART — HM-10 BLE
//  RC6 = TX (iesire spre HM-10 RX, prin divizor tensiune 1k/2k)
//  RC7 = RX (intrare de la HM-10 TX, conexiune directa)
//  9600 baud @ 8MHz: SPBRG = 51, BRGH = 1
// ============================================================
static void UART_Init(void) {
    TRISC6 = 0;
    TRISC7 = 1;
    SPBRG           = 51;
    TXSTAbits.BRGH  = 1;
    TXSTAbits.TXEN  = 1;
    RCSTAbits.SPEN  = 1;
    RCSTAbits.CREN  = 1;
}

static void UART_SendStr(const char *s) {
    while (*s) {
        while (!TXSTAbits.TRMT);
        TXREG = (unsigned char)(*s++);
    }
}

#define RX_BUF_SIZE 32
static char          rxBuf[RX_BUF_SIZE];
static unsigned char rxIdx = 0;

// Citeste un caracter UART si returneaza 1 cand linia e completa
static unsigned char UART_ReadLine(void) {
    if (!PIR1bits.RCIF) return 0;
    if (RCSTAbits.OERR) { RCSTAbits.CREN = 0; RCSTAbits.CREN = 1; }
    char c = (char)RCREG;
    if (c == '\r' || c == '\n') {
        if (rxIdx > 0) { rxBuf[rxIdx] = '\0'; rxIdx = 0; return 1; }
        return 0;
    }
    if (rxIdx < RX_BUF_SIZE - 1) rxBuf[rxIdx++] = c;
    return 0;
}

// ============================================================
//  Alarme — parsare si stocare
// ============================================================
static const char * const DAYS_STR[7] = {
    "Luni","Marti","Miercuri","Joi","Vineri","Sambata","Duminica"
};
static const char * const MOMENTS_STR[3] = {
    "Dimineata","Pranz","Seara"
};

typedef struct { signed char h; unsigned char m; } Alarm;
static Alarm alarms[21];

static void Alarms_Init(void) {
    unsigned char i;
    for (i = 0; i < 21; i++) alarms[i].h = -1;
}

// Parseaza "Luni Dimineata 08:30" si salveaza in alarms[]
// Returneaza 1 daca comanda e valida
static unsigned char ParseCommand(const char *cmd) {
    signed char   day = -1, moment = -1;
    unsigned char i, l;

    for (i = 0; i < 7; i++) {
        l = (unsigned char)strlen(DAYS_STR[i]);
        if (strncmp(cmd, DAYS_STR[i], l) == 0 && cmd[l] == ' ') {
            day = (signed char)i; cmd += l + 1; break;
        }
    }
    if (day < 0) return 0;

    for (i = 0; i < 3; i++) {
        l = (unsigned char)strlen(MOMENTS_STR[i]);
        if (strncmp(cmd, MOMENTS_STR[i], l) == 0 && cmd[l] == ' ') {
            moment = (signed char)i; cmd += l + 1; break;
        }
    }
    if (moment < 0) return 0;

    if (cmd[2] != ':') return 0;
    unsigned char h = (unsigned char)((cmd[0]-'0')*10 + (cmd[1]-'0'));
    unsigned char m = (unsigned char)((cmd[3]-'0')*10 + (cmd[4]-'0'));
    if (h > 23 || m > 59) return 0;

    unsigned char idx = (unsigned char)day * 3 + (unsigned char)moment;
    alarms[idx].h = (signed char)h;
    alarms[idx].m = m;
    return 1;
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

    TRISA  &= ~MASK_A;
    TRISA0  = 1;                          // RA0 = buton (intrare)
    TRISE  &= ~MASK_E;
    TRISD  &= ~MASK_D;
    TRISC0  = TRISC1 = TRISC5 = 0;       // Shift register
    TRISC2  = 0;                          // Buzzer

    PORTA  &= ~MASK_A;
    PORTE  &= ~MASK_E;
    PORTD  &= ~MASK_D;
    ShiftOut(0);
    BUZZER  = 0;

    Alarms_Init();
    pState[0] = pState[1] = pState[2] = pState[3] = 0;

    LCD_SetCursor(0, 0); LCD_Print("  Ora curenta:  ");
    LCD_SetCursor(1, 0); LCD_Print("   --:--:--     ");

    unsigned char h = 0, m = 0, s = 0, dow = 0;  // dow = day of week (0=Luni)
    unsigned char lastSec    = 255;
    signed char   activeIdx  = -1;
    unsigned char blinkOn    = 0;
    unsigned char loopCnt    = 0;
    unsigned char lastFiredH = 255;
    unsigned char lastFiredM = 255;
    unsigned char lastFiredD = 255;

    while (1) {

        // ── Comanda noua de la aplicatie prin BLE ─────────────
        if (UART_ReadLine()) {
            if (ParseCommand(rxBuf)) {
                UART_SendStr("OK\r\n");
            }
        }

        // ── Citire ceas RTC ───────────────────────────────────
        RTC_Read(&h, &m, &s, &dow);

        if (s != lastSec) {
            lastSec = s;

            char buf[9];
            sprintf(buf, "%02d:%02d:%02d", h, m, s);
            LCD_SetCursor(1, 4);
            LCD_Print(buf);

            // Verifica alarme la secunda 0 si doar daca nu e una activa
            if (s == 0 && activeIdx < 0) {
                unsigned char i;
                for (i = 0; i < 21; i++) {
                    // Index alarma = zi(0-6)*3 + moment(0-2)
                    // Extrage ziua din index si compara cu ziua curenta din RTC
                    unsigned char alarmDay = i / 3;
                    if (alarmDay == dow &&
                        alarms[i].h == (signed char)h &&
                        alarms[i].m == m) {
                        // Anti-retriggering: nu reporneste in acelasi minut+zi
                        if (dow != lastFiredD || h != lastFiredH || m != lastFiredM) {
                            activeIdx = (signed char)i;
                            blinkOn   = 1;
                            loopCnt   = 0;
                            LCD_SetCursor(0, 0);
                            LCD_Print("!! IA PASTILA !!");
                            break;
                        }
                    }
                }
            }
        }

        // ── Gestionare alarma activa ──────────────────────────
        if (activeIdx >= 0) {
            if (BTN == 1) {
                // Pastila confirmata — LED ramane aprins permanent
                pState[LEDS[activeIdx].port_id] |= LEDS[activeIdx].mask;
                lastFiredH = h;
                lastFiredM = m;
                lastFiredD = dow;
                BUZZER     = 0;
                activeIdx  = -1;
                blinkOn    = 0;

                UART_SendStr("Pastila luata\r\n");

                LCD_SetCursor(0, 0);
                LCD_Print("  Ora curenta:  ");

                LED_Apply(-1, 0);
                __delay_ms(200);
            } else {
                loopCnt++;
                if (loopCnt >= 10) { loopCnt = 0; blinkOn = !blinkOn; }
                BUZZER = blinkOn;
                LED_Apply(activeIdx, blinkOn);
            }
        } else {
            BUZZER = 0;
            LED_Apply(-1, 0);
        }

        __delay_ms(50);
    }
}
