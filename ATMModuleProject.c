// Omer Karslioglu-17050211061
#include "TM4C123GH6PM.h"
#include <stdio.h>
#include <stdlib.h>      
#include <string.h>
#include <math.h>

#define red 0x02
#define blue 0x04
#define green 0x08

void Delay(unsigned long counter);
void PortF_init(void);
void UART5_init(void);
void UART5_Transmitter(unsigned char data);
void printstring(char *str);

char password[4]={0};
char id[7]={0};
uint8_t userAccountState=0;
uint8_t controlIDCompletedFlag=0;
uint8_t controlPasswordCompletedFlag=0;
uint8_t sifreHakki=3;
char menuState;

uint8_t generalState=0;

uint8_t userAccountStateForTransfer=1;
char transferControlFlag;
uint8_t transferTimmingFlag=0;
uint8_t keypadPassFlag=0; // keypad'in sonsuz döngüden cikmasi icin

void readID(void);
void readPassword(void);
void controlID(void);
void controlPassword(void);
void menu(void);
void withdrawProcess(void); // para cekim
void summaryOfAccountForWithdraw(int accountMoney,int withdrawMoneyInt);
void depositProcess(void);
void transferProcess(void);
void transferControlTimingProcess(void);
void changePasswordProcess(void);

void SysTick_Init(void);
void SysTick_InterruptStart(void);
void SysTick_InterruptStop(void);
void SysTick_Wait(unsigned long delay);
void SysTick_Wait_N_ms(unsigned long N);

void test(void);

void keypad_Init(void);
char keypad_getkey(void);

void showYapiyorum1(void);
void flashLed(char color,int counterForFlashing,int time_ms);

char keypadReadValue;
char mesg[12];

unsigned int ncols = 0;
unsigned int nrows = 0;                                                                    
unsigned const char symbol[4][3] = {{'1', '2', '3'},
                                   { '4', '5', '6'},
                                   { '7', '8', '9'},
                                   { '*', '0', '#'}};
// Kayitli olan user id'leri
char *userIDs[3] = {"1234567","1234568","1234569"};


//varsayilan gecici user sifreleri
//char *userPasswords[3] = {"1234","1235","1236"};
char userPasswords[3][4]={{'1','2','3','4'},{'1','2','3','5'},{'1','2','3','6'}};

//kullanici hesap icerisindeki para base'i
uint32_t accountMoneys[3]={1000,0,1000};


int main(void)
{
	SysTick_Init();
	PortF_init();
	UART5_init();      
	keypad_Init();
	while(1)
	{		
		switch(generalState)
		{
			case 0:
				showYapiyorum1();
				flashLed('b',5,500); // flash blue five time time interval at 200 ms
				generalState++;
				break;
			case 1:
				readID();
				flashLed('w',1,300);
				generalState++;
			break;
			case 2:
				readPassword(); 
				flashLed('w',1,500);
				if(sifreHakki==0)
				{
					generalState=0;
					sifreHakki=3;
				}
				else
				{
					generalState++;
					printstring("\r\n\r\n ------------------------");
					printstring("\r\n -- Giris Basarili ... --");
					printstring("\r\n ------------------------");
					sifreHakki=3;
				}
				//sprintf(mesg,"\r\n\r\n %d",userAccountState);
				//printstring(mesg);				
				//while(1){}
				break;
			case 3:
				menu();
				generalState=(int)menuState-'0'+3; // convert char to int with typecasting and add 3 for general state
				flashLed('g',3,200);
				break;
			case 4:
				withdrawProcess();
				flashLed('b',1,1000);
				generalState=3;
				break;
			case 5:
				depositProcess();
				flashLed('b',1,1000);
				generalState=3;
				break;
			case 6:
				changePasswordProcess();
				flashLed('b',1,1000);
				generalState=3;
				break;
			case 7:
				transferProcess();
				flashLed('b',1,1000);
				generalState=3;
				break;
			case 8: // exit
				generalState=0;
				flashLed('g',1,2000);
				break;
			default:
				generalState=0;
				break;	
		}
	}
}
void PortF_init(void)
{
	SYSCTL->RCGCGPIO |= 0x20;        // initialize clock of Port F
	GPIOF->DIR |=0x0E;         // set PF4 as a digial output pin 
	GPIOF->DEN |=0x1F;
	GPIOF->DATA = 0;
}
void UART5_init(void)
{
	  SYSCTL->RCGCUART |= 0x20; 
    SYSCTL->RCGCGPIO |= 0x10; 
    Delay(1);
    
    UART5->CTL = 0;         
    UART5->IBRD = 104;     
    UART5->FBRD = 11;       
    UART5->CC = 0;          
    UART5->LCRH = 0x60;    
    UART5->CTL = 0x301;     

    
    GPIOE->DEN = 0x30;      
    GPIOE->AFSEL = 0x30;   
    GPIOE->AMSEL = 0;    
    GPIOE->PCTL = 0x00110000;     
}
void UART5_Transmitter(unsigned char data)  
{
    while((UART5->FR & (1<<5)) != 0); 
    UART5->DR = data;    
}

void printstring(char *str)
{
  while(*str)
	{
		UART5_Transmitter(*(str++));
	}
}
void Delay(unsigned long counter)
{
	unsigned long i = 0;
	for(i=0; i< counter*1000; i++);
}

void keypad_Init(void)
{
  SYSCTL->RCGCGPIO |= 0x0C;            //enable clk for port C & D  
  while ((SYSCTL->RCGCGPIO&0x0C)==0);  //wait for clock to be enabled
	GPIOD->LOCK = 0x4C4F434B;   		     // unlock GPIO Port D
  GPIOC->CR  |= 0xE0;             		 //allow changes to all the bits in port C
  GPIOD->CR  |= 0x0F;                  //allow changes to all the bits in port D
  GPIOC->DIR |= 0xE0;                  //set directions cols are o/ps
  GPIOD->DIR |= 0x00;                  //set directions raws are i/ps
  GPIOD->PDR |= 0x0F;                  //pull down resistor on Raws
  GPIOC->DEN |= 0xE0;                  //digital enable pins in port C
  GPIOD->DEN |= 0x0F;                  //digital enable pins in port D
}

char keypad_getkey(void)
{
/*****************************************************
* [PD0 - PD3] -> [R1 - R4]  Raws
* [PC5 - PC7] -> [C1 - C3]  Cols
****************************************************/
	while(1)
	{
		int i=0;
		int j=0;
		int countToCancelTransferFlag=0;
 
		for(i=0;i<3;i++)                        //columns traverse
		{
			GPIOC->DATA = (1U << (i+5));
			Delay(2);
			for(j=0;j<4;j++)                     //raws traverse
			{
				if((GPIOD->DATA & 0x0F) & (1U << j))
				{
					return symbol[j][i];
				}
				else if(transferTimmingFlag==1) // // eger transfer timmingdeyse ayri bir okuma islemi yap
				{
					GPIOC->DATA |= 0xE0;
					while(countToCancelTransferFlag<=1000)
					{
						if((GPIOD->DATA & 0x0F) & (1U << 3))
						{
							return '#';
						}
						countToCancelTransferFlag++;
					}
					//test();
					keypadPassFlag=0;
					countToCancelTransferFlag=0;
					return 'a';
				}
			}
		}
	}
}

void readPassword(void)
{
	int k=0;char gizliSifState;
	printstring("\r\n Sifrenizin gozukmesini ister misiniz ? (1-EVET / 2-HAYIR)");
	gizliSifState=keypad_getkey();SysTick_Wait_N_ms(400);
	
	printstring("\r\n\r\n Sifrenizi giriniz : ");
	while(k<4)
	{
		password[k]=keypad_getkey();SysTick_Wait_N_ms(400);
		switch(gizliSifState)
		{
			case '1': sprintf(mesg,"%c",password[k]);printstring(mesg);break;
			case '2': printstring("*");break;
			default : printstring("*");break;
		}
		k++;
	}
	
	controlPassword();
	controlPasswordCompletedFlag=0;
}

void controlPassword(void)
{
	uint8_t j;
	
	for(j=0;j<4;j++)
	{
		if(!(password[j]==(userPasswords[userAccountState][j]))) break;
		else if(j==3) controlPasswordCompletedFlag=1;
	}
	if(controlPasswordCompletedFlag==0)
	{
		sifreHakki--;
		printstring("\r\n*****************************************************************************");
		sprintf(mesg, "\r\n Girmis oldugunuz sifre hesabiniz ile eslesmemektedir. Kalan hakkiniz : %d", sifreHakki);
		printstring(mesg);
		printstring("\r\n*****************************************************************************\r\n");
		if(sifreHakki>0)
			readPassword();
		else
		{
			printstring("\r\n*****************************************************************************");
			printstring("\r\n******************** Sifre girme hakkiniz kalmamistir . *********************");
			printstring("\r\n*****************************************************************************\r\n");
		}
	}
}

void readID(void)
{
  uint8_t i=0;
	uint8_t againEnterFlag=0;
	printstring("\r\n Hesap numaranizi tekrar girmek icin '*' ' a basiniz . !");
	sprintf(mesg, "\r\n Lutfen Hesap Numaranizi Girin : ");
	printstring(mesg);
  while(i<7)
  {
		id[i]=keypad_getkey();
		if(id[i]=='*') // tekrar girme kosulu
		{
			againEnterFlag=1;
			break;
		}
	  sprintf(mesg, "%c",id[i]);
		printstring(mesg);
	  i++;
		SysTick_Wait_N_ms(400);
	}
	if(againEnterFlag==1)
	{
		SysTick_Wait_N_ms(400);
		againEnterFlag=0;
		readID();
	}
	
	controlID();
	controlIDCompletedFlag=0;//buraya geldiyse id bulunmustur
}

void controlID(void)
{
	uint8_t i,j;
	
	for(i=0;i<3;i++)
	{
		for(j=0;j<7;j++)
		{
			if(!(id[j]==*(userIDs[i]+j))) break;
			if(j==6)
			{
				if(generalState==6) // eger transfer islemi icin id controlu yapildiysa
					userAccountStateForTransfer=i;
				else
					userAccountState=i; // sifre eslesmesi icin kritik
				controlIDCompletedFlag=1;
			}	
		}
	}
	
	if(controlIDCompletedFlag==0)
	{
		printstring("\r\n*****************************************************************************");
		printstring("\r\n**[UYARI] Girdiginiz Hesap Numarasi Sistemimizde Kayitli Bulunmamaktadir ! **");
		printstring("\r\n*****************************************************************************\r\n");
		if(generalState==6) // eger transfer islemi icin id controlu yapildiysa
			;
		else
			readID();
		
	}
}

void menu(void)
{
	printstring("\r\n");
	printstring("\r\n **************************************************");
	printstring("\r\n ** 1-Para Cekim Islemi (Withdraw)               **");
	printstring("\r\n **************************************************");
	printstring("\r\n ** 2-Para Yatirma Islemi (Deposit)              **");
	printstring("\r\n **************************************************");
	printstring("\r\n ** 3-Sifre Degisim Islemi (Change Pin Password) **");
	printstring("\r\n **************************************************");
	printstring("\r\n ** 4-Para Transferi (Money Transfer Process)    **");
	printstring("\r\n **************************************************");
	printstring("\r\n ** 5-Cikis (Exit)                               **");
	printstring("\r\n **************************************************");
	sprintf(mesg, "\r\n\r\n Islem numrarasi giriniz : ");
	printstring(mesg);
	menuState=keypad_getkey();
	SysTick_Wait_N_ms(400);
	sprintf(mesg," %c",menuState);
	printstring(mesg);
	while(!(menuState=='1' || menuState=='2' || menuState=='3' || menuState=='4'|| menuState=='5'))
	{
		printstring("\r\n ******************************************************************");
		printstring("\r\n ** [UYARI] Sadece (1) , (2) , (3) , (4) ve (5) girebilirsiniz ! **");
		printstring("\r\n ******************************************************************");
		menuState=keypad_getkey();
		SysTick_Wait_N_ms(400);
	}
}

void withdrawProcess(void)
{
	char withdrawMoneyChar[]={0};
	char withdrawCharBuff;
	int  withdrawMoneyInt=0;
	uint8_t k=0;
	printstring("\r\n\r\n Tutarinizi girdikten sonra (#)'ye basiniz . Menuye donmek icin (*)'a basin . ");
	printstring("\r\n\r\n Hesabinizdan cekmek istediginiz tutari giriniz : ");
	while(1)
	{
		withdrawCharBuff=keypad_getkey();
		SysTick_Wait_N_ms(400);
		if(!(withdrawCharBuff=='*' || withdrawCharBuff=='#'))
		{
			withdrawMoneyChar[k]=withdrawCharBuff; 
		}
		else if(withdrawCharBuff=='*') 
		{
			generalState=3;
			printstring("\r\n\r\n Menu'ye donuyorsunuz . ");
			Delay(10000);
			generalState=3; //go to menu
			break;
		}
		else if(withdrawCharBuff=='#')
		{
			break;
		}
		sprintf(mesg, "%c",withdrawMoneyChar[k]);
		printstring(mesg);
		k++;
	}
	if(withdrawCharBuff=='#') // para cekim isleminin yapilacagi matematiksel alan
	{
		withdrawMoneyInt=atoi(withdrawMoneyChar);
		if(accountMoneys[userAccountState]>=withdrawMoneyInt)
		{
			summaryOfAccountForWithdraw(accountMoneys[userAccountState],withdrawMoneyInt); // SONUC OZETI
		}
		else
		{
			SysTick_Wait_N_ms(400);
		  printstring("\r\n\r\n [UYARI] Girilen miktar hesabinizda bulunmamaktadir . ");
			printstring("\r\n Hesabinizdaki maksimum parayi cekmek ister misiniz ? (1-EVET / 2-HAYIR) ");

			if(keypad_getkey()=='1')
			{
				summaryOfAccountForWithdraw(accountMoneys[userAccountState],accountMoneys[userAccountState]); // SONUC OZETI
			}
			else if(keypad_getkey()=='2')
			{
				generalState=3; // GO TO MENU
				printstring("\r\n\r\n Menu'ye donuyorsunuz . ");
				Delay(10000);
			}
		}
	}
	SysTick_Wait_N_ms(400);
}

void summaryOfAccountForWithdraw(int accountMoney,int withdrawMoney)
{
	uint16_t countBanknots[5];
	char summaryBuff='1';
	int j=0;
	int kusuratiCikarilmis;
	kusuratiCikarilmis=withdrawMoney-(withdrawMoney%5);
	if(withdrawMoney%5!=0) 
	{
		sprintf(mesg,"\r\n %d tl almaniz mumkun degil , %d tl almak ister misiniz ? (1-Evet(Yes) , 2-Hayir(No))",withdrawMoney,kusuratiCikarilmis);
		printstring(mesg);
		summaryBuff=keypad_getkey();
		SysTick_Wait_N_ms(400);
	}
	if(summaryBuff=='1') // bankotlari ayirmak icin kullanacagim bolum
	{
		accountMoneys[userAccountState]=accountMoneys[userAccountState]-kusuratiCikarilmis; // ANA PARA - CEKILEN PARA
		countBanknots[0]= withdrawMoney/100;     // count100
		countBanknots[1]=(withdrawMoney-(countBanknots[0]*100))/50;  // count50
		countBanknots[2]=(withdrawMoney-(countBanknots[0]*100)-(countBanknots[1]*50))/20;  // count20
		countBanknots[3]=(withdrawMoney-(countBanknots[0]*100)-(countBanknots[1]*50)-(countBanknots[2]*20))/10;  // count10
		countBanknots[4]=(withdrawMoney-(countBanknots[0]*100)-(countBanknots[1]*50)-(countBanknots[2]*20)-(countBanknots[3]*10))/5;   // count5
		printstring("\r\n\r\n                       *** Summary Of Account *** \r\n");
		for(j=0;j<5;j++)
		{
			if(countBanknots[j]!=0)
			{
				switch(j)
				{
					case 0 :	sprintf(mesg," %d Adet (Piece) : 100 tl , ",countBanknots[j]); break;
					case 1 :	sprintf(mesg," %d Adet (Piece) : 50 tl , " ,countBanknots[j]); break;
					case 2 :	sprintf(mesg," %d Adet (Piece) : 20 tl , " ,countBanknots[j]); break;
					case 3 :	sprintf(mesg," %d Adet (Piece) : 10 tl , " ,countBanknots[j]); break;
					case 4 :	sprintf(mesg," %d Adet (Piece) : 5 tl  , " ,countBanknots[j]); break;
				}
				printstring(mesg);
			}		
		}
		printstring(" aldiniz . Isleminiz basariyla gerceklesti . ");
	  sprintf(mesg,"\r\n Hesabininzda Kalan Para : %d tl'dir",accountMoneys[userAccountState]);
		printstring(mesg);
	}
	else if(summaryBuff=='2')
	{
		generalState=3; // GO TO MENU
		printstring("\r\n\r\n Menu'ye donuyorsunuz . ");
		Delay(10000);
	}
	else if(summaryBuff!='1' && summaryBuff!='2')
	{
		printstring("\r\n\r\n [UYARI] Sadece (1) EVET'e veya (2) HAYIR'a basabilirsiniz . ");
		summaryOfAccountForWithdraw(accountMoney,withdrawMoney);
		SysTick_Wait_N_ms(400);
	}
}


void depositProcess(void)
{
	char countBanknots[]={0};
	char takingMoneyBuff;
	int totalMoneyEntered=0; int j=0 , k=0;int buBufflazim=0;int bankNotValue=0;
	printstring("\r\n\r\n\r\n\r\n *************************** \r\n");
	printstring("\r\n *** Para Yatirma Islemi *** \r\n");
	printstring("\r\n *************************** \r\n");
	printstring("\r\n\r\n Bankot sayisini her girme isleminizi her tamamlamanizda lufen #'ye basin . ");
	printstring("\r\n Menuye donmek icin lutfen (*)'a basiniz . ");
	
	for(;j<5;j++)
	{
		switch(j)
		{
			case 0 : printstring("\r\n Lutfen 100 tl'lik bankot sayisi girin : ");bankNotValue=100; break;
			case 1 : printstring("\r\n Lutfen 50 tl'lik bankot sayisi girin : "); bankNotValue=50; break;
			case 2 : printstring("\r\n Lutfen 20 tl'lik bankot sayisi girin : "); bankNotValue=20; break;
			case 3 : printstring("\r\n Lutfen 10 tl'lik bankot sayisi girin : "); bankNotValue=10; break;
			case 4 : printstring("\r\n Lutfen 5 tl'lik bankot sayisi girin : ");  bankNotValue=5; break;
		}
		while(1) // bankot sayisinin yazilmasi icin olusturdugum bolum
		{
			takingMoneyBuff=keypad_getkey();
			SysTick_Wait_N_ms(400); // to block arc
			if(takingMoneyBuff=='#')
			{
				buBufflazim=atoi(countBanknots);
				totalMoneyEntered+=(buBufflazim*bankNotValue);
				bankNotValue=0;
				k=0;
				countBanknots[0]=0; // clear countBanknots
				break;
			}
			else if(takingMoneyBuff!='*')
			{
				countBanknots[k]=takingMoneyBuff;
				sprintf(mesg,"%c",countBanknots[k]);
				printstring(mesg);
				k++;
			}
		}
		if(takingMoneyBuff=='*')
			break;
		else
			continue;
	}
	if(takingMoneyBuff=='*') // go to menu
	{
		totalMoneyEntered=0;
		generalState=3; // menu state
		printstring("\r\n\r\n Menu'ye donuyorsunuz . ");
		Delay(10000);
	}
	else
	{
		accountMoneys[userAccountState]+=totalMoneyEntered; // hesaptaki paraya ekle
		sprintf(mesg,"\r\n\r\n Hesabinizdaki Toplam Miktar : %d",accountMoneys[userAccountState]);
		printstring(mesg);
	}
}

void transferProcess(void)
{
	char transferMoneyChar[]={0};
	char transferCharBuff;
	int  transferMoneyInt=0;
	uint8_t k=0;
	int mainAccountMoneyBuff=0;
	int transferAccountMoneyBuff=0;
	
	generalState=6; //zaten state 6 lakin kesinlestimek icin lazim
	
	printstring("\r\n\r\n\r\n\r\n **************************** \r\n");
	printstring("\r\n *** Para Transfer Islemi *** \r\n");
	printstring("\r\n **************************** \r\n");
	
	readID();
	
	printstring("\r\n\r\n Tutarinizi girdikten sonra (#)'ye basiniz . Menuye donmek icin (*)'a basin . ");
	printstring("\r\n\r\n Gondermek istediginiz tutari giriniz : ");
	
	while(1)
	{
		transferCharBuff=keypad_getkey();
		SysTick_Wait_N_ms(400);
		if(!(transferCharBuff=='*' || transferCharBuff=='#'))
		{
			transferMoneyChar[k]=transferCharBuff; 
		}
		else if(transferCharBuff=='*') 
		{
			generalState=3;
			printstring("\r\n\r\n Menu'ye donuyorsunuz . ");
			Delay(10000);
			break;
		}
		else if(transferCharBuff=='#')
		{
			break;
		}
		sprintf(mesg, "%c",transferMoneyChar[k]);
		printstring(mesg);
		k++;
	}
	if(transferCharBuff=='#') // para cekim isleminin yapilacagi matematiksel alan
	{
		transferMoneyInt=atoi(transferMoneyChar);
		if(accountMoneys[userAccountState]>=transferMoneyInt)
		{
			transferControlTimingProcess();
			if(transferControlFlag==1)
			{
				mainAccountMoneyBuff=accountMoneys[userAccountState];
				accountMoneys[userAccountState]-=transferMoneyInt;
				transferAccountMoneyBuff=accountMoneys[userAccountStateForTransfer];
				accountMoneys[userAccountStateForTransfer]+=transferMoneyInt;
				
				printstring("\r\n\r\n\r\n\r\n            ISLEM OZETI ");
				printstring("\r\n            *********** ");
				printstring("\r\n");
				sprintf(mesg,"\r\n\r\n Transfer edilen para : %d tl'dir . ",transferMoneyInt);
				printstring(mesg);
				sprintf(mesg,"\r\n Hesabinizda kalan bakiye : %d tl'dir . ",accountMoneys[userAccountState]);
				printstring(mesg);
				sprintf(mesg,"\r\n TEST TEST TEST transfer edilen hesaptaki para : %d tl'dir . ",accountMoneys[userAccountStateForTransfer]);
				printstring(mesg);
				transferControlFlag=0;
			}
		}
		else
		{
			SysTick_Wait_N_ms(400);
		  printstring("\r\n\r\n [UYARI] Girilen miktar hesabinizda bulunmamaktadir . ");
			printstring("\r\n Hesabinizdaki maksimum parayi transfer etmek ister misiniz ? (1-EVET / 2-HAYIR) ");

			if(keypad_getkey()=='1')
			{
				transferControlTimingProcess();
				if(transferControlFlag==1)
				{
					mainAccountMoneyBuff=accountMoneys[userAccountState];
					accountMoneys[userAccountState]=0;
					transferAccountMoneyBuff=accountMoneys[userAccountStateForTransfer];
					accountMoneys[userAccountStateForTransfer]+=mainAccountMoneyBuff;
					
					printstring("\r\n\r\n\r\n\r\n            ISLEM OZETI ");
					printstring("\r\n            *********** ");
					printstring("\r\n");
					sprintf(mesg,"\r\n\r\n Transfer edilen para : %d tl'dir . ",mainAccountMoneyBuff);
					printstring(mesg);
					sprintf(mesg,"\r\n Hesabinizda kalan bakiye : %d tl'dir . ",accountMoneys[userAccountState]);
					printstring(mesg);
					sprintf(mesg,"\r\n TEST TEST TEST transfer edilen hesaptaki para : %d tl'dir . ",accountMoneys[userAccountStateForTransfer]);
					printstring(mesg);
					transferControlFlag=0;
				}
			}
			else if(keypad_getkey()=='2')
			{
				generalState=3; // GO TO MENU
				printstring("\r\n\r\n Menu'ye donuyorsunuz . ");
				flashLed('g',5,500);
			}
		}
	}
}

void transferControlTimingProcess()
{
	int counterForTime=30;
	transferTimmingFlag=1;
	printstring("\r\n\r\n Isleminizi iptal etmek icin (#)'a bir saniye basili tutun ... \r\n");
  //SysTick_InterruptStart();
	while(counterForTime>0)
	{
		SysTick_Wait_N_ms(1000);
		sprintf(mesg,"\r\n Isleminizin tamamlanmasi icin %d saniye kalmistir ...",counterForTime);
		printstring(mesg);
		GPIOF->DATA  ^= green|red|green;  //toggle PF3 pin
//		if(keypad_getkey()=='#')
//		{
//			transferControlFlag=2;
//		}
		if(keypad_getkey()=='#') // islem iptal edildiyse
		{
			//test();
			transferControlFlag=0; // don't make transfer process
			transferTimmingFlag=0; // özel keypad okuma islemi bitti
  		//SysTick_InterruptStop(); // stop the interrupt
			printstring("\r\n\r\n Isleminiz basariyla iptal edilmistir ! \r\n");
			sprintf(mesg,"\r\n Hesabinizdaki Bakiye : %d tl'dir . ",accountMoneys[userAccountState]);
			printstring(mesg);
			break;
		}
		
		counterForTime--;
		
		if(counterForTime==0) // islem iptal edilmediyse - transfer isleminin gerceklestirilmesi icin flag'i kaldir 
		{
			//test();
			transferControlFlag=1; // make transfer process
			transferTimmingFlag=0; // özel keypad okuma islemi bitti
			//SysTick_InterruptStop();
		}
	}
}

void changePasswordProcess(void)
{
	int k=0,j=0;char gizliSifState;uint16_t changePasswordProcessAgainFlag=1;
	char changePasswordBuff[4]={0};

	while(changePasswordProcessAgainFlag==1)
	{
		changePasswordProcessAgainFlag=0;k=0;j=0; // initializing counter and flag
		
		printstring("\r\n Gireceginiz yeni sifrenizin gozukmesini ister misiniz ? (1-EVET / 2-HAYIR)");
		gizliSifState=keypad_getkey();SysTick_Wait_N_ms(400);
		
		printstring("\r\n\r\n Yeni sifre giriniz : ");
		while(k<4)
		{
			changePasswordBuff[k]=keypad_getkey();SysTick_Wait_N_ms(400);
			switch(gizliSifState)
			{
				case '1': sprintf(mesg,"%c",changePasswordBuff[k]);printstring(mesg);break;
				case '2': printstring("*");break;
				default : printstring("*");break;
			}
			k++;
		}
		for(k=0;k<4;k++) // yeni sifrenin uygun olup olmadigi kontrol ediliyor
		{
			for(;j<4;j++)
			{
				if(k==j) break;
				else if(changePasswordBuff[k]==changePasswordBuff[j])
				{
					changePasswordProcessAgainFlag=1;
					sprintf(mesg,"\r\n\r\n %d . karakter ile %d . karakter aynidir .",j+1,k+1);printstring(mesg);
					printstring("\r\n [UYARI] Sifreniz farkli karakterlerden olusmalidir !");
					break;
				}
			}
			if(changePasswordProcessAgainFlag==1) break; // donguden cik 
		}
		if(changePasswordProcessAgainFlag==0) 
		{
//			userPasswords[userAccountState]=malloc(sizeof(char));
//			userPasswords[userAccountState]=NULL;
//		strcpy(userPasswords[userAccountState],changePasswordBuff);
//			userPasswords[userAccountState]=changePasswordBuff;
			j=0;
			while(j<4)
			{
				(userPasswords[userAccountState][j])=changePasswordBuff[j];
				j++;
			}
			printstring("\r\n TEST TEST TEST Yeni sifreniz : ");
			printstring(changePasswordBuff);
			printstring("\r\n Isleminiz basariyla gerceklesti ...");
			break;
		}
	}
}

void SysTick_Init(void)
{
	SysTick->CTRL=0; // disable the st_ctrl register
	SysTick->LOAD =0x00FFFFFF; // maximum reload value
	SysTick->VAL=0; // clear the current register
	SysTick->CTRL=0x00000005; // enable the interrupt and st_ctrl register
}
void SysTick_InterruptStart(void)
{
	SysTick->CTRL=0; // disable the st_ctrl register
	SysTick->LOAD =0x00FFFFFF; // maximum reload value
	SysTick->VAL=0; // clear the current register
	SysTick->CTRL|=0x00000007; // enable the interrupt and st_ctrl register
}
void SysTick_InterruptStop(void)
{
	SysTick->CTRL=0; // disable the st_ctrl register
	SysTick->VAL=(0<<2); // clear the interrupt bit
}
void SysTick_Handler(void) // interrupt handler FOR transfer
{
  GPIOF->DATA  ^= green;  //toggle PF3 pin
	keypadPassFlag=1;
}

//(supposing)Clock source = 16 mhz (62.5 ns)
//SysTick Interval = 10 ms
//(reload+1)*CLK_PERIOD=10ms
//reload = (10 ms x 16 MHz)-1
//reload = 1.6x10^6-1 

void SysTick_Wait(unsigned long delay)
{
	SysTick->LOAD=delay;
	SysTick->VAL=0; // clear the current register
	while((SysTick->CTRL & 0x10000)==0){} // waiting until that flag is set
}

//wait for N*1ms
void SysTick_Wait_N_ms(unsigned long N)
{
	unsigned long i;
	for(i=0;i<N;i++){
		SysTick_Wait(16000-1); // 1 ms / 62.5ns = 16000 ==> i obtained 1 ms	
	}
}


 void test(void)
 {
	printstring("\r\n\r\n TEST TEST TEST TEST");
 }

 
// eglence amacli
void showYapiyorum1(void)
{
	printstring("\r\n    ____                        _  __              _ _             _         ____              _     ");
	printstring("\r\n   / __ \                      | |/ /             | (_)           | |       |  _ \            | |    ");
	printstring("\r\n  | |  | |_ __ ___   ___ _ __  | ' / __ _ _ __ ___| |_  ___   __ _| |_   _  | |_) | __ _ _ __ | | __ ");
	printstring("\r\n  | |  | | '_ ` _ \ / _ \ '__| |  < / _` | '__/ __| | |/ _ \ / _` | | | | | |  _ < / _` | '_ \| |/ / ");
	printstring("\r\n  | |__| | | | | | |  __/ |    | . \ (_| | |  \__ \ | | (_) | (_| | | |_| | | |_) | (_| | | | |   <  ");
	printstring("\r\n   \____/|_| |_| |_|\___|_|    |_|\_\__,_|_|  |___/_|_|\___/ \__, |_|\__,_| |____/ \__,_|_| |_|_|\_\ \r\n");
}

// red - green - blue and white flashing led
void flashLed(char color,int counterForFlashing,int time_ms)
{
	switch(color)
	{
		case 'g':
			while(0<counterForFlashing)
			{
				GPIOF->DATA=green;
				SysTick_Wait_N_ms(time_ms);
				GPIOF->DATA=0x00;
				counterForFlashing--;
				SysTick_Wait_N_ms(time_ms);
			}
			break;
			
		case 'r':
			while(0<counterForFlashing)
			{
				GPIOF->DATA=red;
				SysTick_Wait_N_ms(time_ms);
				GPIOF->DATA=0x00;
				counterForFlashing--;
				SysTick_Wait_N_ms(time_ms);
			}
			break;

		case 'b':
			while(0<counterForFlashing)
			{
				GPIOF->DATA=blue;
				SysTick_Wait_N_ms(time_ms);
				GPIOF->DATA=0x00;
				counterForFlashing--;
				SysTick_Wait_N_ms(time_ms);
			}
			break;

		case 'w':
			while(0<counterForFlashing)
			{
				GPIOF->DATA=green|blue|red;
				SysTick_Wait_N_ms(time_ms);
				GPIOF->DATA=0x00;
				counterForFlashing--;
				SysTick_Wait_N_ms(time_ms);
			}
			break;		
	}
}
