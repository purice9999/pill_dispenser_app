// ============================================================
//  Dozator de Pastile — PIC16F887 + HM-10 BLE
//  Comunicare cu aplicatia Flutter prin UART @ 9600 baud
//
//  RTC: DS3231 (I2C, adresa 0x68, compatibil cu DS1307)
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
//  I2C + RTC DS3231
//  Adresa I2C: 0x68 (write=0xD0, read=0xD1)
//  Registri identici cu DS1307:
//    0x00=sec, 0x01=min, 0x02=ora, 0x03=dow, 0x04=dom, 0x05=luna, 0x06=an
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
    return (unsigned char)(((v / 10) << 4) | (v % 10));
}

// Scrie data/ora in DS3231
// dow_rtc: 1=Duminica, 2=Luni, 3=Marti, 4=Miercuri, 5=Joi, 6=Vineri, 7=Sambata
static void RTC_SetTime(unsigned char h, unsigned char m, unsigned char s,
                        unsigned char dow_rtc, unsigned char dom,
                        unsigned char mon, unsigned char yr) {
    I2C_Start();
    I2C_Write(0xD0);
    I2C_Write(0x00);
    I2C_Write(Dec2BCD(s));
    I2C_Write(Dec2BCD(m));
    I2C_Write(Dec2BCD(h));
    I2C_Write(dow_rtc);
    I2C_Write(Dec2BCD(dom));
    I2C_Write(Dec2BCD(mon));
    I2C_Write(Dec2BCD(yr));
    I2C_Stop();
}

// Citeste ora, minutul, secunda, ziua saptamanii, ziua lunii, luna
// DS3231 registri: 0=sec, 1=min, 2=ora, 3=dow, 4=dom, 5=luna
// dow: 0=Luni ... 6=Duminica (conventie europeana)
// DS3231 stocheaza 1-7 in registrul DOW (valoarea depinde de cum a fost
// initializat modulul). Conversia (*dow + 5) % 7 presupune 1=Duminica.
static void RTC_Read(unsigned char *h, unsigned char *m, unsigned char *s,
                     unsigned char *dow, unsigned char *dom, unsigned char *mon) {
    I2C_Start();
    I2C_Write(0xD0);   // DS3231 write address (0x68 << 1)
    I2C_Write(0x00);   // pointer la registrul 0 (secunde)
    I2C_Restart();
    I2C_Write(0xD1);   // DS3231 read address
    *s   = BCD2Dec(I2C_Read(1));   // reg 0: secunde
    *m   = BCD2Dec(I2C_Read(1));   // reg 1: minute
    *h   = BCD2Dec(I2C_Read(1) & 0x3F); // reg 2: ora (masca bit 12/24h)
    *dow = I2C_Read(1);            // reg 3: ziua sapt.
    *dow = (*dow + 5) % 7;         // conversie la 0=Luni...6=Duminica
    *dom = BCD2Dec(I2C_Read(1));   // reg 4: ziua lunii
    *mon = BCD2Dec(I2C_Read(0) & 0x1F); // reg 5: luna (masca bit century)
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
//  Receptie prin intrerupere cu buffer circular (32 bytes)
// ============================================================
#define CIRC_SIZE 32
static volatile char          circBuf[CIRC_SIZE];
static volatile unsigned char circHead = 0;
static volatile unsigned char circTail = 0;

void __interrupt() isr(void) {
    if (PIR1bits.RCIF) {
        if (RCSTAbits.OERR) { RCSTAbits.CREN = 0; RCSTAbits.CREN = 1; }
        char c = (char)RCREG;
        unsigned char next = (circHead + 1) & (CIRC_SIZE - 1);
        if (next != circTail) {
            circBuf[circHead] = c;
            circHead = next;
        }
    }
}

static void UART_Init(void) {
    TRISC6 = 0;
    TRISC7 = 1;
    SPBRG          = 51;
    TXSTAbits.BRGH = 1;
    TXSTAbits.TXEN = 1;
    RCSTAbits.SPEN = 1;
    RCSTAbits.CREN = 1;
    PIE1bits.RCIE        = 1;
    INTCONbits.PEIE      = 1;
    INTCONbits.GIE       = 1;
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
    while (circTail != circHead) {
        char c = circBuf[circTail];
        circTail = (circTail + 1) & (CIRC_SIZE - 1);
        if (c == '\r' || c == '\n') {
            if (rxIdx > 0) { rxBuf[rxIdx] = '\0'; rxIdx = 0; return 1; }
            continue;
        }
        if (rxIdx < RX_BUF_SIZE - 1) rxBuf[rxIdx++] = c;
    }
    return 0;
}

// ============================================================
//  EEPROM — stocare evenimente offline (buton/timeout)
//  Layout: addr 0x00 = numar evenimente (0-84)
//          addr 0x01+: evenimente 3 bytes (tip, ora, minut)
//          tip: 0=luata, 1=neluata
// ============================================================
#define EEP_MAX 84

static void EEPROM_Write(unsigned char addr, unsigned char data) {
    EEADR            = addr;
    EEDAT            = data;
    EECON1bits.EEPGD = 0;
    EECON1bits.WREN  = 1;
    INTCONbits.GIE   = 0;
    EECON2           = 0x55;
    EECON2           = 0xAA;
    EECON1bits.WR    = 1;
    while (EECON1bits.WR);
    EECON1bits.WREN  = 0;
    INTCONbits.GIE   = 1;
}

static unsigned char EEPROM_Read(unsigned char addr) {
    EEADR            = addr;
    EECON1bits.EEPGD = 0;
    EECON1bits.RD    = 1;
    return EEDAT;
}

static void EEPROM_SaveEvent(unsigned char taken, unsigned char ev_h, unsigned char ev_m) {
    unsigned char cnt  = EEPROM_Read(0x00);
    if (cnt == 0xFF) cnt = 0;
    if (cnt >= EEP_MAX) return;
    unsigned char base = (unsigned char)(1 + cnt * 3);
    EEPROM_Write(base,     taken);
    EEPROM_Write(base + 1, ev_h);
    EEPROM_Write(base + 2, ev_m);
    EEPROM_Write(0x00, cnt + 1);
}

static void EEPROM_SendAll(void) {
    unsigned char cnt = EEPROM_Read(0x00);
    if (cnt == 0xFF || cnt == 0) { EEPROM_Write(0x00, 0); return; }
    unsigned char i, type, ev_h, ev_m, base;
    char d2[3];
    d2[2] = '\0';
    for (i = 0; i < cnt; i++) {
        base  = (unsigned char)(1 + i * 3);
        type  = EEPROM_Read(base);
        ev_h  = EEPROM_Read(base + 1);
        ev_m  = EEPROM_Read(base + 2);
        UART_SendStr(type == 0 ? "SYNC luata " : "SYNC neluata ");
        d2[0] = (char)('0' + ev_h / 10);
        d2[1] = (char)('0' + ev_h % 10);
        UART_SendStr(d2);
        UART_SendStr(":");
        d2[0] = (char)('0' + ev_m / 10);
        d2[1] = (char)('0' + ev_m % 10);
        UART_SendStr(d2);
        UART_SendStr("\r\n");
        __delay_ms(100);
    }
    EEPROM_Write(0x00, 0);
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

static const char * const DAYS_LCD[7] = {
    "Luni","Mar.","Mie.","Joi ","Vin.","Sam.","Dum."
};
static const char * const MOMS_LCD[3] = {
    "Dim","Prz","Sea"
};
static const char * const DAYS_ROW0[7] = {
    "Luni  ","Marti ","Mier. ","Joi   ","Vineri","Samb. ","Dum.  "
};

typedef struct { signed char h; unsigned char m; } Alarm;
static Alarm alarms[21];

static void Alarms_Init(void) {
    unsigned char i;
    for (i = 0; i < 21; i++) alarms[i].h = -1;
}

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

static void LCD_ShowNormal(unsigned char h, unsigned char m, unsigned char s,
                           unsigned char dow, unsigned char dom, unsigned char mon) {
    char buf[17];
    sprintf(buf, "%s%02d/%02d     ", DAYS_ROW0[dow], dom, mon);
    buf[16] = '\0';
    LCD_SetCursor(0, 0);
    LCD_PrintRow(buf);
    sprintf(buf, "    %02d:%02d:%02d    ", h, m, s);
    buf[16] = '\0';
    LCD_SetCursor(1, 0);
    LCD_PrintRow(buf);
}

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

#define LCD_NORMAL   0
#define LCD_RECEIVED 1
#define LCD_ACTIVE   2
#define LCD_DONE     3

void main(void) {
    ANSEL = ANSELH = 0;
    CM1CON0 = CM2CON0 = 0;

    LCD_Init();
    I2C_Init();
    UART_Init();

    // Reseteaza RTC daca valorile sunt invalide (corupte)
    // 23:41:00, Joi (5), 04/06/2026
    {
        unsigned char th, tm, ts, tdow, tdom, tmon;
        RTC_Read(&th, &tm, &ts, &tdow, &tdom, &tmon);
        if (th > 23 || tm > 59 || ts > 59 || tdom > 31 || tmon > 12 || tmon == 0) {
            RTC_SetTime(23, 41, 0, 5, 4, 6, 26);
        }
    }

    // Initializeaza EEPROM daca e prima pornire (0xFF = sters din fabrica)
    if (EEPROM_Read(0x00) == 0xFF) EEPROM_Write(0x00, 0);

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

    LCD_SetCursor(0, 0); LCD_PrintRow("  Se initializeaz");
    LCD_SetCursor(1, 0); LCD_PrintRow("      RTC...    ");
    __delay_ms(1000);

    unsigned char h = 0, m = 0, s = 0;
    unsigned char dow = 0, dom = 1, mon = 1;
    unsigned char lastSec      = 255;
    signed char   activeIdx    = -1;
    unsigned char blinkOn      = 0;
    unsigned char loopCnt      = 0;
    unsigned char lcdState     = LCD_NORMAL;
    unsigned char lcdTimer     = 0;
    unsigned char lastFiredH   = 255;
    unsigned char lastFiredM   = 255;
    unsigned char lastFiredD   = 255;
    unsigned int  ledOffTimer  = 0;
    signed char   confirmedIdx = -1;
    // Timeout alarma: 1200 loop-uri * 50ms = 60 secunde
    unsigned int  alarmTimeout = 0;

    while (1) {

        // ── Comanda noua de la aplicatie ──────────────────────────────────
        if (UART_ReadLine()) {
            if (strcmp(rxBuf, "TEST") == 0) {
                if (alarms[0].h < 0) { alarms[0].h = 12; alarms[0].m = 0; }
                activeIdx = 0;
                blinkOn   = 1;
                loopCnt   = 0;
                lcdState  = LCD_ACTIVE;
                LCD_ShowAlarmActive(0);
                UART_SendStr("TEST OK\r\n");
            } else if (strcmp(rxBuf, "SYNC") == 0) {
                EEPROM_SendAll();
            } else {
                unsigned char idx = ParseCommand(rxBuf);
                if (idx != 255) {
                    UART_SendStr("OK\r\n");
                    lcdState = LCD_RECEIVED;
                    lcdTimer = 3;
                    LCD_ShowAlarmSet(idx);
                }
            }
        }

        // ── Citire DS3231 ─────────────────────────────────────────────────
        RTC_Read(&h, &m, &s, &dow, &dom, &mon);

        if (s != lastSec) {
            lastSec = s;

            if (lcdTimer > 0) {
                lcdTimer--;
                if (lcdTimer == 0 && lcdState != LCD_ACTIVE) {
                    lcdState = LCD_NORMAL;
                }
            }

            if (ledOffTimer > 0) {
                ledOffTimer--;
                if (ledOffTimer == 0 && confirmedIdx >= 0) {
                    pState[LEDS[confirmedIdx].port_id] &= ~LEDS[confirmedIdx].mask;
                    confirmedIdx = -1;
                    LED_Apply(-1, 0);
                }
            }

            if (lcdState == LCD_NORMAL) {
                LCD_ShowNormal(h, m, s, dow, dom, mon);
            }

            // ── Verificare alarme la secunda 0 ────────────────────────────
            // FIX: nu mai comparam cu ziua din RTC (alarms[i].h >= 0 verifica
            // ca slotul este setat). Alarma se declanseaza in orice zi la ora
            // potrivita, la fel ca in build-ul initial care functiona.
            if (s == 0 && activeIdx < 0) {
                unsigned char i;
                for (i = 0; i < 21; i++) {
                    if (alarms[i].h >= 0 &&
                        alarms[i].h == (signed char)h &&
                        alarms[i].m == m) {
                        if (h != lastFiredH || m != lastFiredM || dow != lastFiredD) {
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

        // ── Gestionare alarma activa ──────────────────────────────────────
        if (activeIdx >= 0) {
            if (BTN == 1) {
                pState[LEDS[activeIdx].port_id] |= LEDS[activeIdx].mask;
                confirmedIdx = activeIdx;
                ledOffTimer  = 1800;
                lastFiredH = h;
                lastFiredM = m;
                lastFiredD = dow;
                BUZZER     = 0;

                LCD_ShowConfirmed((unsigned char)activeIdx);
                lcdState = LCD_DONE;
                lcdTimer = 3;

                UART_SendStr("Pastila luata\r\n");
                EEPROM_SaveEvent(0, h, m);  // 0 = luata

                activeIdx    = -1;
                blinkOn      = 0;
                alarmTimeout = 0;
                LED_Apply(-1, 0);
                __delay_ms(200);
            } else {
                loopCnt++;
                if (loopCnt >= 10) { loopCnt = 0; blinkOn = !blinkOn; }
                BUZZER = blinkOn;
                if (loopCnt == 0) LCD_ShowAlarmActive((unsigned char)activeIdx);
                LED_Apply(activeIdx, blinkOn);

                // Timeout 60 secunde — pastila nu a fost luata
                alarmTimeout++;
                if (alarmTimeout >= 1200) {
                    lastFiredH = h;
                    lastFiredM = m;
                    lastFiredD = dow;
                    BUZZER     = 0;
                    activeIdx    = -1;
                    blinkOn      = 0;
                    alarmTimeout = 0;
                    lcdState   = LCD_NORMAL;
                    LED_Apply(-1, 0);
                    UART_SendStr("Pastila neluata\r\n");
                    EEPROM_SaveEvent(1, h, m);  // 1 = neluata
                }
            }
        } else {
            alarmTimeout = 0;
            BUZZER = 0;
            LED_Apply(-1, 0);
        }

        __delay_ms(50);
    }
}
