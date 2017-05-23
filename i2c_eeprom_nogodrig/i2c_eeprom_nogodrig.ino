//чтение i2c eeprom-а ногодрыгом
//основано на коде http://spritesmods.com/?art=rtl8366sb но исправлены ошибки и сделана адаптация к arduino!
//точнее !!! это  просто код из проека rtl8370_try3.ino ! но чисто его часть отвечающая за чтение/запись eeprom-а!
//как выяснилось некоторые микросхемы критичны к start и ack ! и мне пришлось слегка изменить код
#include <util/delay.h>

char buf[64];

#define DEV_ADDR 0x50

#define I2CDELAY 5
#define MY_SDA A0
#define MY_SCL A1

void setScl(char val){
  pinMode(MY_SCL, OUTPUT);
  if(val)
    digitalWrite(MY_SCL, HIGH);
  else
    digitalWrite(MY_SCL, LOW);
  _delay_us(I2CDELAY);
}
void setSda(char val){
  pinMode(MY_SDA, OUTPUT);  
  if(val)
    digitalWrite(MY_SDA, HIGH);
  else
    digitalWrite(MY_SDA, LOW);
  _delay_us(I2CDELAY);
}
char getScl(void){
  _delay_us(I2CDELAY);
  pinMode(MY_SCL, INPUT);  
  return digitalRead(MY_SCL);
}
char getSda(void){
  _delay_us(I2CDELAY);
  pinMode(MY_SDA, INPUT);  
  return digitalRead(MY_SDA);
}
static char i2cWrite(unsigned char data){
  char ack;
  for(unsigned char x = 0x80; x != 0; x >>= 1){
    if (data & x) setSda(1); else setSda(0);
    setScl(1);
    setScl(0);
  }
  
  //Get ack
  setSda(1);
  ack = !getSda(); //для некоторых микросхем важно сначала считать sda а затем уже устанавливать SCL ! иначе sda тут же поменяется! быстро реагинуют?
  setScl(1);
  setScl(0);
  return ack;
}
static unsigned char i2cRead(char ack){
  unsigned char data = 0;
  setSda(1);
  for(unsigned char x = 0x80; x != 0; x >>= 1){
    setScl(1);
    if (getSda()) data |= x;
    setScl(0);
  }
  //Send ack
  setSda(!ack);
  setScl(1);
  setScl(0);
  return data;
}
static char i2cStart(char address){
  setScl(1);
  setSda(1); //это важно для некоторых микросхем!
  setSda(0);
  setScl(0);
  return i2cWrite(address);
}
static void i2cStop(void){
  setSda(0);
  setScl(1);
  setSda(1);
}

//Eeprom routines
unsigned char eepromRead(int adr){
  unsigned char r;
  int devadr_and_eeprom_off = DEV_ADDR | ((adr >> 8) & 0x07);
  devadr_and_eeprom_off <<= 1;
  //у нас флешка на 1024 байта! ST24C08 !
  if(!i2cStart(devadr_and_eeprom_off))
    Serial.println("No ACK1!"); //dummy write      
  //common routines ! пошлем запрос на чтение адреса 0x0 из eeprom-а
  if(!i2cWrite(adr & 0xFF))
    Serial.println("No ACK2!");
  if(!i2cStart(devadr_and_eeprom_off + 1))
    Serial.println("No ACK3!"); //read
  r = i2cRead(0);
  i2cStop();
  return r;
}

void eepromWrite(int adr, unsigned char data){
  int devadr_and_eeprom_off = DEV_ADDR | ((adr >> 8) & 0x07);
  devadr_and_eeprom_off <<= 1;
  //у нас флешка на 1024 байта! ST24C08 !
  if(!i2cStart(devadr_and_eeprom_off))
    Serial.println("No ACK1!"); //dummy write
  if(!i2cWrite(adr & 0xFF))
    Serial.println("No ACK2!");
  if(!i2cWrite(data))
    Serial.println("No ACK3!");
  i2cStop();
  //Не знаю точно, но в Datasheet описана задержка записи в 5мс
  //если между повторный вызов этой функции выпадает с ошибкой no ACKx то увеличь эту задержку! флешка просто не успевает!
  delay(5);
}

void setup(){
  unsigned int rrr;
  uint8_t rx_buf[64];
  Serial.begin(115200); 
  return;
  rx_buf[0] = eepromRead(0);
  rx_buf[1] = eepromRead(1);
  Serial.print("EEPROM first two bytes: ");
  snprintf(buf, sizeof(buf), "%02X %02X", rx_buf[0] & 0xFF, rx_buf[1] & 0xFF);  
  Serial.println(buf);
  //return;
  
   { //dump first 20 bytes of eeprom
    int a;
    int off = 0x0;
    Serial.println();
    memset(buf, 0x0, sizeof(buf));
    for(a = 0; a < 20; a++){
      rx_buf[a] = eepromRead(off + a);
      snprintf(buf, sizeof(buf), "%02X ", eepromRead(off + a));
      Serial.print(buf);
    }
    Serial.println();
    //запись
    /* eepromWrite(0x01DE, 0xFF); delay(50);
    eepromWrite(0x01DF, 0xFF); delay(50);
    eepromWrite(0x01E0, 0xFF); delay(50);
    eepromWrite(0x01E1, 0xFF); delay(50);
    eepromWrite(0, 0xDE); delay(50); */
    
/*     eepromWrite(0x0, 'P');
    eepromWrite(0x1, '\0'); */
//    eepromWrite(0x1, 'w');
//    eepromWrite(0x2, 'L');
//    eepromWrite(0x3, 0);
    /* {
      int l = snprintf(buf, sizeof(buf), "Hello from OwL! You do this! Ok Ok Ic? Ok Ok!");
      for(a = 0; a < l + 1; a++)
        eepromWrite(a, buf[a]);
    } */

//    Serial.println((char *)rx_buf);
  }
}

//int x = 0;
void loop(){
/*  if(getSda())
    Serial.println("1");
  else
    Serial.println("0");
  if(getScl())
    Serial.println("1");
  else
    Serial.println("0");
*/
/*  setSda(x);
  setScl(!x);
  Serial.println(x);
  x = !x;  
  delay(1000); */

}

