// ============================================================
//  Dozator de Pastile — PIC16F887 + HM-10 BLE
//  Comunicare cu aplicatia Flutter prin UART @ 9600 baud
//
//  Protocol BLE:
//    App -> PIC : "Luni Dimineata 08:30\r\n"
//    PIC -> App : "OK\r\n"              (alarma salvata)
//    PIC -> App : "Pastila luata\r\n"   (buton apasat)
//
//  LCD 16x2 — stari:
//    Normal:          Rand0: "Luni   26/05   "
//                     Rand1: "     14:20:04  "
//    Alarma primita:  Rand0: "Alarma setata: "
//                     Rand1: "Luni Dim 08:00 "
//    Alarma activa:   Rand0: "!! IA PASTILA !!"
//                     Rand1: "Luni Dim 08:00 "
//    Confirmata:      Rand0: "Luni Dim 08:00 "
//                     Rand1: "   Confirmat!  "
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

static const unsigned char ROW_ADDR[2] = {0x80, 0xC0};

static void LCD_SetCursor(unsigned char row, unsigned char col) {
    LCD_Cmd(ROW_ADDR[row] + col);
}

// Scrie exact 16 caractere (completeaza cu spatii daca e mai scurt)
static void LCD_PrintRow(const char *s) {
    unsigned char i;
    for (i = 0; i < 16; i++) {
        LCD_Char(s[i] ? s[i] : ' ');
    }
}

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

// Citeste ora, minutul, secunda, ziua saptamanii, ziua lunii, luna
// DS1307 registri: 0=sec, 1=min, 2=ora, 3=dow, 4=dom, 5=luna
// dow: 0=Luni ... 6=Duminica (conventie europeana, 1=Luni in DS1307)
static void RTC_Read(unsigned char *h, unsigned char *m, unsigned char *s,
                     unsigned char *dow, unsigned char *dom, unsigned char *mon) {
    I2C_Start();
    I2C_Write(0xD0);
    I2C_Write(0x00);
    I2C_Restart();
    I2C_Write(0xD1);
    *s   = BCD2Dec(I2C_Read(1));   // reg 0: secunde
    *m   = BCD2Dec(I2C_Read(1));   // reg 1: minute
    *h   = BCD2Dec(I2C_Read(1));   // reg 2: ora
    *dow = I2C_Read(1);            // reg 3: ziua sapt. (1-7, nu BCD)
    if (*dow > 0) (*dow)--;        // 0=Luni ... 6=Duminica
    *dom = BCD2Dec(I2C_Read(1));   // reg 4: ziua lunii
    *mon = BCD2Dec(I2C_Read(0));   // reg 5: luna (NACK = ultimul byte)
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

static unsigned char pState[4] = {0, 0, 0, 0};

// ============================================================
//  BUTON + BUZZER
// ============================================================
#define BTN     PORTAbits.RA0
#define BUZZER  PORTCbits.RC2

// ============================================================
//  Aplica starea LED-urilor
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
//  UART — HM-10 BLE (RC6=TX, RC7=RX, 9600 baud @ 8MHz)
// ============================================================
static void UART_Init(void) {
    TRISC6 = 0;
    TRISC7 = 1;
    SPBRG           = 51;     // 9600 baud @ 8MHz, BRGH=1
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
//  Alarme + parsare BLE
// ============================================================
static const char * const DAYS_STR[7] = {
    "Luni","Marti","Miercuri","Joi","Vineri","Sambata","Duminica"
};
static const char * const MOMENTS_STR[3] = {
    "Dimineata","Pranz","Seara"
};

// Nume scurte pentru LCD (4 caractere fixe)
static const char * const DAYS_LCD[7] = {
    "Luni","Mar.","Mie.","Joi ","Vin.","Sam.","Dum."
};
// Momente scurte pentru LCD (3 caractere fixe)
static const char * const MOMS_LCD[3] = {
    "Dim","Prz","Sea"
};
// Zile lungi pentru randul 0 (6 caractere fixe)
static const char * const DAYS_ROW0[7] = {
    "Luni  ","Marti ","Mier. ","Joi   ","Vineri","Samb. ","Dum.  "
};

typedef struct { signed char h; unsigned char m; } Alarm;
static Alarm alarms[21];

static void Alarms_Init(void) {
    unsigned char i;
    for (i = 0; i < 21; i++) alarms[i].h = -1;
}

// Parseaza "Luni Dimineata 08:30" -> salveaza in alarms[]
// Returneaza index (0-20) daca ok, 255 daca eroare
static unsigned char ParseCommand(const char *cmd) {
    signed char   day = -1, moment = -1;
    unsigned char i, l;

    for (i = 0; i < 7; i++) {
        l = (unsigned char)strlen(DAYS_STR[i]);
        if (strncmp(cmd, DAYS_STR[i], l) == 0 && cmd[l] == ' ') {
            day = (signed char)i; cmd += l + 1; break;
        }
    }
    if (day < 0) return 255;

    for (i = 0; i < 3; i++) {
        l = (unsigned char)strlen(MOMENTS_STR[i]);
        if (strncmp(cmd, MOMENTS_STR[i], l) == 0 && cmd[l] == ' ') {
            moment = (signed char)i; cmd += l + 1; break;
        }
    }
    if (moment < 0) return 255;

    if (cmd[2] != ':') return 255;
    unsigned char h = (unsigned char)((cmd[0]-'0')*10 + (cmd[1]-'0'));
    unsigned char m = (unsigned char)((cmd[3]-'0')*10 + (cmd[4]-'0'));
    if (h > 23 || m > 59) return 255;

    unsigned char idx = (unsigned char)day * 3 + (unsigned char)moment;
    alarms[idx].h = (signed char)h;
    alarms[idx].m = m;
    return idx;
}

// ============================================================
//  Functii LCD display
// ============================================================

// Rand 0: "Luni   26/05   "
// Rand 1: "     14:20:04  "
static void LCD_ShowNormal(unsigned char h, unsigned char m, unsigned char s,
                           unsigned char dow, unsigned char dom, unsigned char mon) {
    char buf[17];
    // Rand 0: ziua + data
    sprintf(buf, "%s%02d/%02d     ", DAYS_ROW0[dow], dom, mon);
    buf[16] = '\0';
    LCD_SetCursor(0, 0);
    LCD_PrintRow(buf);
    // Rand 1: ora centrata
    sprintf(buf, "    %02d:%02d:%02d    ", h, m, s);
    buf[16] = '\0';
    LCD_SetCursor(1, 0);
    LCD_PrintRow(buf);
}

// Rand 0: "Alarma setata: "
// Rand 1: "Luni Dim 08:00 "
static void LCD_ShowAlarmSet(unsigned char idx) {
    char buf[17];
    unsigned char dayIdx = idx / 3;
    unsigned char momIdx = idx % 3;
    unsigned char ah     = (unsigned char)alarms[idx].h;
    unsigned char am     = alarms[idx].m;

    LCD_SetCursor(0, 0);
    LCD_PrintRow("Alarma setata:  ");
    sprintf(buf, "%s %s %02d:%02d  ", DAYS_LCD[dayIdx], MOMS_LCD[momIdx], ah, am);
    buf[16] = '\0';
    LCD_SetCursor(1, 0);
    LCD_PrintRow(buf);
}

// Rand 0: "!! IA PASTILA !!"
// Rand 1: "Luni Dim 08:00 "
static void LCD_ShowAlarmActive(unsigned char idx) {
    char buf[17];
    unsigned char dayIdx = idx / 3;
    unsigned char momIdx = idx % 3;
    unsigned char ah     = (unsigned char)alarms[idx].h;
    unsigned char am     = alarms[idx].m;

    LCD_SetCursor(0, 0);
    LCD_PrintRow("!! IA PASTILA !!");
    sprintf(buf, "%s %s %02d:%02d  ", DAYS_LCD[dayIdx], MOMS_LCD[momIdx], ah, am);
    buf[16] = '\0';
    LCD_SetCursor(1, 0);
    LCD_PrintRow(buf);
}

// Rand 0: "Luni Dim 08:00 "
// Rand 1: "   Confirmat!  "
static void LCD_ShowConfirmed(unsigned char idx) {
    char buf[17];
    unsigned char dayIdx = idx / 3;
    unsigned char momIdx = idx % 3;
    unsigned char ah     = (unsigned char)alarms[idx].h;
    unsigned char am     = alarms[idx].m;

    sprintf(buf, "%s %s %02d:%02d  ", DAYS_LCD[dayIdx], MOMS_LCD[momIdx], ah, am);
    buf[16] = '\0';
    LCD_SetCursor(0, 0);
    LCD_PrintRow(buf);
    LCD_SetCursor(1, 0);
    LCD_PrintRow("   Confirmat!   ");
}

// ============================================================
//  MAIN
// ============================================================

// Stari LCD pentru mesaje temporare
#define LCD_NORMAL   0
#define LCD_RECEIVED 1   // "Alarma setata" (3 secunde)
#define LCD_ACTIVE   2   // Alarma activa (pana la buton)
#define LCD_DONE     3   // "Confirmat!" (3 secunde)

void main(void) {
    ANSEL = ANSELH = 0;
    CM1CON0 = CM2CON0 = 0;

    LCD_Init();
    I2C_Init();
    UART_Init();

    TRISA  &= ~MASK_A;
    TRISA0  = 1;
    TRISE  &= ~MASK_E;
    TRISD  &= ~MASK_D;
    TRISC0  = TRISC1 = TRISC5 = 0;
    TRISC2  = 0;

    PORTA  &= ~MASK_A;
    PORTE  &= ~MASK_E;
    PORTD  &= ~MASK_D;
    ShiftOut(0);
    BUZZER  = 0;

    Alarms_Init();
    pState[0] = pState[1] = pState[2] = pState[3] = 0;

    // Afisaj initial
    LCD_SetCursor(0, 0); LCD_PrintRow("  Se initializeaz");
    LCD_SetCursor(1, 0); LCD_PrintRow("      RTC...    ");
    __delay_ms(1000);

    unsigned char h = 0, m = 0, s = 0;
    unsigned char dow = 0, dom = 1, mon = 1;
    unsigned char lastSec    = 255;
    signed char   activeIdx  = -1;
    unsigned char blinkOn    = 0;
    unsigned char loopCnt    = 0;
    unsigned char lcdState   = LCD_NORMAL;
    unsigned char lcdTimer   = 0;    // secunde ramase pentru mesaj temporar
    unsigned char lastAlarmIdx = 255;
    unsigned char lastFiredH = 255;
    unsigned char lastFiredM = 255;
    unsigned char lastFiredD = 255;

    while (1) {

        // ── Comanda noua de la aplicatie ──────────────────────
        if (UART_ReadLine()) {
            unsigned char idx = ParseCommand(rxBuf);
            if (idx != 255) {
                UART_SendStr("OK\r\n");
                lastAlarmIdx = idx;
                lcdState = LCD_RECEIVED;
                lcdTimer = 3;                // afiseaza 3 secunde
                LCD_ShowAlarmSet(idx);
            }
        }

        // ── Citire RTC ────────────────────────────────────────
        RTC_Read(&h, &m, &s, &dow, &dom, &mon);

        if (s != lastSec) {
            lastSec = s;

            // Decrementeaza timer-ul pentru mesaje temporare
            if (lcdTimer > 0) {
                lcdTimer--;
                if (lcdTimer == 0 && lcdState != LCD_ACTIVE) {
                    lcdState = LCD_NORMAL;
                }
            }

            // Actualizeaza LCD in starea normala
            if (lcdState == LCD_NORMAL) {
                LCD_ShowNormal(h, m, s, dow, dom, mon);
            }

            // Verifica alarmele la secunda 0
            if (s == 0 && activeIdx < 0) {
                unsigned char i;
                for (i = 0; i < 21; i++) {
                    unsigned char alarmDay = i / 3;
                    if (alarmDay == dow &&
                        alarms[i].h == (signed char)h &&
                        alarms[i].m == m) {
                        if (dow != lastFiredD || h != lastFiredH || m != lastFiredM) {
                            activeIdx = (signed char)i;
                            blinkOn   = 1;
                            loopCnt   = 0;
                            lcdState  = LCD_ACTIVE;
                            lcdTimer  = 0;
                            LCD_ShowAlarmActive((unsigned char)activeIdx);
                            break;
                        }
                    }
                }
            }
        }

        // ── Gestionare alarma activa ──────────────────────────
        if (activeIdx >= 0) {
            if (BTN == 1) {
                // Confirmare pastila luata
                pState[LEDS[activeIdx].port_id] |= LEDS[activeIdx].mask;
                lastFiredH = h;
                lastFiredM = m;
                lastFiredD = dow;
                BUZZER     = 0;

                LCD_ShowConfirmed((unsigned char)activeIdx);
                lcdState = LCD_DONE;
                lcdTimer = 3;

                UART_SendStr("Pastila luata\r\n");

                activeIdx = -1;
                blinkOn   = 0;
                LED_Apply(-1, 0);
                __delay_ms(200);
            } else {
                loopCnt++;
                if (loopCnt >= 10) { loopCnt = 0; blinkOn = !blinkOn; }
                BUZZER = blinkOn;
                // Reafiseaza la fiecare blink (in caz ca LCD s-a corupt)
                if (loopCnt == 0) LCD_ShowAlarmActive((unsigned char)activeIdx);
                LED_Apply(activeIdx, blinkOn);
            }
        } else {
            BUZZER = 0;
            LED_Apply(-1, 0);
        }

        __delay_ms(50);
    }
}
