#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include "rl_net.h"
#include "leds.h"
#include "rtc.h"

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif

extern uint16_t     AD_in(uint32_t ch);
extern bool         LEDrun;
extern char         lcd_text[2][20 + 1];
extern osThreadId_t TID_Display;

static uint8_t P2;
static uint8_t ip_addr[NET_ADDR_IP6_LEN];
static char    ip_string[40];

typedef struct {
    uint8_t idx;
    uint8_t unused[3];
} MY_BUF;
#define MYBUF(p) ((MY_BUF *)(p))

void netCGI_ProcessQuery(const char *qstr)
{
    netIF_Option opt = netIF_OptionMAC_Address;
    int16_t      typ = 0;
    char var[40];

    do {
        qstr = netCGI_GetEnvVar(qstr, var, sizeof(var));
        switch (var[0]) {
            case 'i':
                opt = (var[1] == '4') ? netIF_OptionIP4_Address
                                      : netIF_OptionIP6_StaticAddress;
                break;
            case 'm':
                if (var[1] == '4') opt = netIF_OptionIP4_SubnetMask;
                break;
            case 'g':
                opt = (var[1] == '4') ? netIF_OptionIP4_DefaultGateway
                                      : netIF_OptionIP6_DefaultGateway;
                break;
            case 'p':
                opt = (var[1] == '4') ? netIF_OptionIP4_PrimaryDNS
                                      : netIF_OptionIP6_PrimaryDNS;
                break;
            case 's':
                opt = (var[1] == '4') ? netIF_OptionIP4_SecondaryDNS
                                      : netIF_OptionIP6_SecondaryDNS;
                break;
            default:
                var[0] = '\0';
                break;
        }
        switch (var[1]) {
            case '4': typ = NET_ADDR_IP4; break;
            case '6': typ = NET_ADDR_IP6; break;
            default:  var[0] = '\0';      break;
        }
        if ((var[0] != '\0') && (var[2] == '=')) {
            netIP_aton(&var[3], typ, ip_addr);
            netIF_SetOption(NET_IF_CLASS_ETH, opt, ip_addr, sizeof(ip_addr));
        }
    } while (qstr);
}

void netCGI_ProcessData(uint8_t code, const char *data, uint32_t len)
{
    char var[40];
    char passw[12];

    if (code != 0) return;

    P2     = 0;
    LEDrun = true;

    if (len == 0) {
        LED_SetOut_stm(P2);
        return;
    }

    passw[0] = 1;
    do {
        data = netCGI_GetEnvVar(data, var, sizeof(var));
        if (var[0] != 0) {
            if      (strcmp(var, "led0=on") == 0) { P2 |= 0x01; }
            else if (strcmp(var, "led1=on") == 0) { P2 |= 0x02; }
            else if (strcmp(var, "led2=on") == 0) { P2 |= 0x04; }
            else if (strcmp(var, "ctrl=Browser") == 0) { LEDrun = false; }
            else if ((strncmp(var, "pw0=", 4) == 0) ||
                     (strncmp(var, "pw2=", 4) == 0)) {
                if (netHTTPs_LoginActive()) {
                    if (passw[0] == 1) {
                        strcpy(passw, var + 4);
                    } else if (strcmp(passw, var + 4) == 0) {
                        netHTTPs_SetPassword(passw);
                    }
                }
            }
            else if (strncmp(var, "lcd1=", 5) == 0) {
                strcpy(lcd_text[0], var + 5);
                osThreadFlagsSet(TID_Display, 0x01U);
            }
            else if (strncmp(var, "lcd2=", 5) == 0) {
                strcpy(lcd_text[1], var + 5);
                osThreadFlagsSet(TID_Display, 0x01U);
            }
        }
    } while (data);

    LED_SetOut_stm(P2);
}

uint32_t netCGI_Script(const char *env, char *buf, uint32_t buflen, uint32_t *pcgi)
{
    int32_t      socket;
    netTCP_State state;
    NET_ADDR     r_client;
    const char  *lang;
    uint32_t     len = 0U;
    uint8_t      id;
    static uint32_t adv;
    netIF_Option opt = netIF_OptionMAC_Address;
    int16_t      typ = 0;

    switch (env[0]) {

    case 'a':
        switch (env[3]) {
            case '4': typ = NET_ADDR_IP4; break;
            case '6': typ = NET_ADDR_IP6; break;
            default:  return 0U;
        }
        switch (env[2]) {
            case 'l':
                if (env[3] == '4') return 0U;
                opt = netIF_OptionIP6_LinkLocalAddress;
                break;
            case 'i':
                opt = (env[3] == '4') ? netIF_OptionIP4_Address
                                      : netIF_OptionIP6_StaticAddress;
                break;
            case 'm':
                if (env[3] != '4') return 0U;
                opt = netIF_OptionIP4_SubnetMask;
                break;
            case 'g':
                opt = (env[3] == '4') ? netIF_OptionIP4_DefaultGateway
                                      : netIF_OptionIP6_DefaultGateway;
                break;
            case 'p':
                opt = (env[3] == '4') ? netIF_OptionIP4_PrimaryDNS
                                      : netIF_OptionIP6_PrimaryDNS;
                break;
            case 's':
                opt = (env[3] == '4') ? netIF_OptionIP4_SecondaryDNS
                                      : netIF_OptionIP6_SecondaryDNS;
                break;
        }
        netIF_GetOption(NET_IF_CLASS_ETH, opt, ip_addr, sizeof(ip_addr));
        netIP_ntoa(typ, ip_addr, ip_string, sizeof(ip_string));
        len = (uint32_t)sprintf(buf, &env[5], ip_string);
        break;

    case 'b':
        if (env[2] == 'c') {
            len = (uint32_t)sprintf(buf, &env[4],
                                    LEDrun ?     ""     : "selected",
                                    LEDrun ? "selected" :    ""     );
            break;
        }
        id = env[2] - '0';
        if (id > 7) id = 0;
        id = (uint8_t)(1U << id);
        len = (uint32_t)sprintf(buf, &env[4], (P2 & id) ? "checked" : "");
        break;

    case 'c':
        while ((uint32_t)(len + 150) < buflen) {
            socket = ++MYBUF(pcgi)->idx;
            state  = netTCP_GetState(socket);
            if (state == netTCP_StateINVALID) return (uint32_t)len;

            len += (uint32_t)sprintf(buf + len, "<tr align=\"center\">");
            if (state <= netTCP_StateCLOSED) {
                len += (uint32_t)sprintf(buf + len,
                    "<td>%d</td><td>%d</td><td>-</td><td>-</td>"
                    "<td>-</td><td>-</td></tr>\r\n",
                    socket, netTCP_StateCLOSED);
            } else if (state == netTCP_StateLISTEN) {
                len += (uint32_t)sprintf(buf + len,
                    "<td>%d</td><td>%d</td><td>%d</td><td>-</td>"
                    "<td>-</td><td>-</td></tr>\r\n",
                    socket, netTCP_StateLISTEN, netTCP_GetLocalPort(socket));
            } else {
                netTCP_GetPeer(socket, &r_client, sizeof(r_client));
                netIP_ntoa(r_client.addr_type, r_client.addr,
                           ip_string, sizeof(ip_string));
                len += (uint32_t)sprintf(buf + len,
                    "<td>%d</td><td>%d</td><td>%d</td>"
                    "<td>%d</td><td>%s</td><td>%d</td></tr>\r\n",
                    socket, netTCP_StateLISTEN, netTCP_GetLocalPort(socket),
                    netTCP_GetTimer(socket), ip_string, r_client.port);
            }
        }
        len |= (1U << 31);
        break;

    case 'd':
        switch (env[2]) {
            case '1':
                len = (uint32_t)sprintf(buf, &env[4],
                      netHTTPs_LoginActive() ? "Enabled" : "Disabled");
                break;
            case '2':
                len = (uint32_t)sprintf(buf, &env[4], netHTTPs_GetPassword());
                break;
        }
        break;

    case 'e':
        lang = netHTTPs_GetLanguage();
        if      (strncmp(lang, "en", 2) == 0) lang = "English";
        else if (strncmp(lang, "de", 2) == 0) lang = "German";
        else if (strncmp(lang, "fr", 2) == 0) lang = "French";
        else if (strncmp(lang, "sl", 2) == 0) lang = "Slovene";
        else                                   lang = "Unknown";
        len = (uint32_t)sprintf(buf, &env[2], lang, netHTTPs_GetLanguage());
        break;

    case 'f':
        switch (env[2]) {
            case '1': len = (uint32_t)sprintf(buf, &env[4], lcd_text[0]); break;
            case '2': len = (uint32_t)sprintf(buf, &env[4], lcd_text[1]); break;
        }
        break;

    case 'g':
        switch (env[2]) {
            case '1':
                adv = AD_in(0);
                len = (uint32_t)sprintf(buf, &env[4], adv);
                break;
            case '2':
                len = (uint32_t)sprintf(buf, &env[4],
                      (double)((float)adv * 3.3f) / 4096.0f);
                break;
            case '3':
                adv = (adv * 100U) / 4096U;
                len = (uint32_t)sprintf(buf, &env[4], adv);
                break;
        }
        break;

    case 'h':
        {
            RTC_TimeTypeDef sTime = {0};
            RTC_DateTypeDef sDate = {0};
            char texto[20];

            /* Lectura local y segura para no interferir con otros hilos */
            HAL_RTC_GetTime(&RtcHandle, &sTime, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&RtcHandle, &sDate, RTC_FORMAT_BIN);

            switch (env[2]) {
                case '1':
                    sprintf(texto, "%02d:%02d:%02d",
                            sTime.Hours, sTime.Minutes, sTime.Seconds);
                    len = (uint32_t)sprintf(buf, &env[4], texto);
                    break;
                case '2':
                    sprintf(texto, "%02d/%02d/20%02d",
                            sDate.Date, sDate.Month, sDate.Year);
                    len = (uint32_t)sprintf(buf, &env[4], texto);
                    break;
            }
        }
        break;

    case 'j':
        {
            RTC_TimeTypeDef sTime = {0};
            RTC_DateTypeDef sDate = {0};
            char texto[20];

            /* Arreglado para comportarse de forma segura usando el offset &env[4] 
               como espera el motor CGI de Keil, evitando desbordamientos de buffer */
            HAL_RTC_GetTime(&RtcHandle, &sTime, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&RtcHandle, &sDate, RTC_FORMAT_BIN);

            if (env[2] == '1') {
                sprintf(texto, "%02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);
                len = (uint32_t)sprintf(buf, &env[4], texto);
            } else if (env[2] == '2') {
                sprintf(texto, "%02d/%02d/20%02d", sDate.Date, sDate.Month, sDate.Year);
                len = (uint32_t)sprintf(buf, &env[4], texto);
            }
        }
        break;

    case 'x':
        adv = AD_in(0);
        /* Mantengo &env[1], asegúrate de que tu etiqueta HTML encaja con este offset */
        len = (uint32_t)sprintf(buf, &env[1], adv);
        break;

    case 'y':
        break;

    default:
        break;
    }

    return len;
}

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic pop
#endif