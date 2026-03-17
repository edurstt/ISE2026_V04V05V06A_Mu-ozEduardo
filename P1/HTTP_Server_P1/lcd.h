
void retardo(int micro_segundos);
void init(void);
void LCD_reset(void);
void wr_cmd (unsigned char cmd);
void wr_data (unsigned char cmd);
void LCD_update(void);
//void symbolToLocalBuffer_L1(uint8_t symbol);
//void symbolToLocalBuffer_L2(uint8_t symbol);
//extern void escribeLinea1(char cadena[]);
//extern void escribeLinea2(char cadena[]);
void EscribeFraseL1(const char *frase1);
void EscribeFraseL2(const char *frase2);
void limpiardisplay(void);
//void LCD_clear_L1(void);
//void LCD_clear_L2(void);
