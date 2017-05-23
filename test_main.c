#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/route.h>
#include <linux/pkt_sched.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_tcpudp.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/if_arp.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/err.h>
#include <net/protocol.h>
#include <linux/gpio.h>

#define DEBUG 1
#include "debug.h"
//#include "skb_od_list.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergey Sergeev <sergey.sergeev@yapic.net>");
MODULE_DESCRIPTION("kernel test");

#define DEV_ADDR 0x50
#define I2CDELAY 5

static int ddd = 0;
module_param(ddd, int, 0);

#define MY_SDA 16
#define MY_SCL 14

void setGPIO(int pin, int val){
	gpio_direction_output(pin, val);
}
int getGPIO(int pin){
	gpio_direction_input(pin);
	return !!gpio_get_value(pin);
}
void setScl(char val){
	setGPIO(MY_SCL, val);
  udelay(I2CDELAY);
}
void setSda(char val){
	setGPIO(MY_SDA, val);
  udelay(I2CDELAY);
}
char getScl(void){
	udelay(I2CDELAY);
	return getGPIO(MY_SCL);
}
char getSda(void){
	udelay(I2CDELAY);
	return getGPIO(MY_SDA);
}
static char i2cWrite(unsigned char data){
  char ack;
	unsigned char x;
  for(x = 0x80; x != 0; x >>= 1){
    if(data & x) setSda(1); else setSda(0);
    setScl(1);
    setScl(0);
  }
  //Get ack
  setSda(1);
	/* для некоторых микросхем важно сначала считать sda а затем уже устанавливать SCL !
		 иначе sda тут же поменяется! быстро реагинуют? */
  ack = !getSda();
  setScl(1);
  setScl(0);
  return ack;
}
static unsigned char i2cRead(char ack){
  unsigned char data = 0;
	unsigned char x;
  setSda(1);
  for(x = 0x80; x != 0; x >>= 1){
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
    printk(KERN_WARNING "No ACK1!\n"); //dummy write
  //common routines ! пошлем запрос на чтение адреса 0x0 из eeprom-а
  if(!i2cWrite(adr & 0xFF))
    printk(KERN_WARNING "No ACK2!\n");
  if(!i2cStart(devadr_and_eeprom_off + 1))
    printk(KERN_WARNING "No ACK3!\n"); //read
  r = i2cRead(0);
  i2cStop();
  return r;
}

void eepromWrite(int adr, unsigned char data){
  int devadr_and_eeprom_off = DEV_ADDR | ((adr >> 8) & 0x07);
  devadr_and_eeprom_off <<= 1;
  //у нас флешка на 1024 байта! ST24C08 !
  if(!i2cStart(devadr_and_eeprom_off))
    printk(KERN_WARNING "No ACK1!\n"); //dummy write
  if(!i2cWrite(adr & 0xFF))
    printk(KERN_WARNING "No ACK2!\n");
  if(!i2cWrite(data))
    printk(KERN_WARNING "No ACK3!\n");
  i2cStop();
  //Не знаю точно, но в Datasheet описана задержка записи в 5мс
  //если между повторный вызов этой функции выпадает с ошибкой no ACKx то увеличь эту задержку! флешка просто не успевает!
  mdelay(5);
}

//*********************************************************
//выполняется при загрузке модуля
static int __init test_m_module_init(void){
	char buf[255];
	int a;
	int pre;
	//setSda(1);
	/* setGPIO(MY_SDA, 1);
	for(a = 0; a < 10; a++){
		//printk(KERN_DEBUG "Sda = %d\n", getSda());
		printk(KERN_DEBUG "Sda = %d\n", getGPIO(MY_SDA));
  	set_current_state(TASK_INTERRUPTIBLE);
  	schedule_timeout(HZ / 10);
	} */


	printk(KERN_DEBUG "Eeprom first byte = 0x%02X\n", eepromRead(0) & 0xFF);
//	return -EINVAL;
	//getSda(); //IN
	//setScl(0); //OUT
	for(a = 0; a < 64; a++){
  	buf[a] = eepromRead(a);
	}
//  setGPIO(14, ddd);
//  setGPIO(16, !ddd);
//	return -ENODEV;
//	setSda(!ddd);
//	setScl(ddd);

//	setGPIO(16, ddd);
//	setGPIO(14, ddd);
//	setGPIO(15, ddd);
//	setGPIO(11, ddd);
//	printk(KERN_DEBUG "16 = %d\n", getGPIO(16));
//	printk(KERN_DEBUG "14 = %d\n", getGPIO(14));
//	return -ENODEV;
//	printk(KERN_DEBUG "15 = %d\n", getGPIO(15));
//	printk(KERN_DEBUG "16 = %d\n", getGPIO(16));
//	return -EINVAL;
 	printk(KERN_DEBUG "EEPROM first 20 bytes:\n");
	for(a = 0; a < 64; a++){
		printk(KERN_DEBUG "%02X\n", buf[a] & 0xFF);
	}
//	printk(KERN_DEBUG "%s\n", buf);
/*	a = 0;
	pre = 0;
	while(a < 10){
		int v = getSda();
		if(v != pre){
			setScl(v);
			pre = v;
			a++;
		}
  	set_current_state(TASK_INTERRUPTIBLE);
  	schedule_timeout(5);
	} */
	//printk(KERN_DEBUG "Sda = %d\n", getSda());
  //printk(KERN_DEBUG "Sck = %d\n", getScl());

	/* запись */
/* {
    int l = snprintf(buf, sizeof(buf), "Hello from OwL! You do this! Ok Ok Ic? Ok Ok!");
    for(a = 0; a < l + 1; a++)
    eepromWrite(a, buf[a]);
  } */

  return -ENOMEM;
}//--------------------------------------------------------

//*********************************************************
//выполняется при выгрузке модуля
static void __exit test_m_module_exit (void){
}//--------------------------------------------------------

module_init(test_m_module_init);
module_exit(test_m_module_exit);
