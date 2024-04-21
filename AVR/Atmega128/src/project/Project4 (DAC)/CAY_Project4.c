/*                                             */
/* CAY_Project4.c                              */
/*                                             */
/* Created: 2023-11-22 ���� 2:34:24             */
/* Author: �־ƿ�                                */
/* Project subject : DAC                        */

#include <mega128a.h>
#include <delay.h>

#define CLK_HIGH    PORTD |= 0x01   // PD.0=1 Ŭ�� 1
#define CLK_LOW     PORTD &= 0xFE   // PD.0=0 Ŭ�� 0
#define DAT_HIGH    PORTD |= 0x02   // PD.1=1 ������ 1
#define DAT_LOW     PORTD &= 0xFD   // PD.1=0 ������ 0
#define LD_HIGH     PORTD |= 0x04   // PD.2=1 ������ 1
#define LD_LOW      PORTD &= 0xFB   // PD.2=0 ������ 0

#define SCL_OUT     DDRD |= 0x01    // PD.0=1 Ŭ�� ��¸��
#define SDA_OUT     DDRD |= 0x02    // PD.1=1 ������ ��¸��
#define LD_OUT      DDRD |= 0x04    // PD.2=1 ������ ��¸��

typedef unsigned char U8;
typedef unsigned short U16;

const U16 DAC_Digital[21]= {0x00,0x19, 0x33,0x4C,0x66,0x80,0x99,0xB3,0xCC,0xE6,0xFF,0xE6,0xCC,0xB3,0x99,0x80,0x66,0x4C,0x33,0x19,0x00};
U16 DAC_DATA = 0;                 
U16 TEMP_DAC_DATA = 0;    
U8 EINT4_FLAG_BIT = 0;
U16 i,j; 

void DAC_write(void);

void main(void)
{
 DDRC = 0xFF;
 PORTC = 0xFF;  
 
 EIMSK |= 0b00010000;    //���ͷ�Ʈ ��� ���� ���� �������ͷ� INT4 enable
 EICRB |= 0b00000011;    //4�� Rising edge

 SCL_OUT;
 SDA_OUT;
 LD_OUT;
 
 SREG |= 0x80; 

while (1)
    {
        CLK_HIGH;
		LD_HIGH;   
		
        if(EINT4_FLAG_BIT==1){          // ���ͷ�Ʈ 4 �߻��� Bytewrite ����
		    EINT4_FLAG_BIT=0;
			PORTC=0xFE;				// Start
            DAC_write(); 	
            delay_ms(10);          // stop�� srat���� ���� 100us 

        }//end of if  
  }//end of while
}//end of main       

interrupt[EXT_INT4] void external_int4(void) {   //main CPU�� ���α׷��� �����ϰ� �ִٰ� interrupt�� �߻��ϸ� �� ���񽺷�ƾ�� ����,������Ͽ��� ���ǵ� ���ͷ�Ʈ �ҽ��� ���,�Լ����� ���Ƿ� ����
    SREG &= 0x7F;                                //Nesting ������ ���� status resister bit7�� 0���� �����Ͽ� ��� ���ͷ�Ʈ�� ����(global interrupt disable)
    EINT4_FLAG_BIT = 1;                            //interrupt ª�� ���α׷����ϱ� ���� �̷��� ������. ���ͷ�Ʈ4�� ���� 1�̵Ǹ� while�� �ȿ� �����ص� ������ ����
    SREG |= 0x80;                                //status resister bit7�� 1���� �����Ͽ� ��� ���ͷ�Ʈ�� ���(global interrupt enable)
}

void DAC_write(void){    
       
	for(i=0;i<21;i++) {
		DAC_DATA = DAC_Digital[i];
		
		LD_LOW;
		CLK_LOW;   
		TEMP_DAC_DATA=DAC_DATA;
		
		for(j=0;j<=9;j++)
		{
			if((TEMP_DAC_DATA&0x200)==0x200)DAT_HIGH; 
			else DAT_LOW; 
			
			TEMP_DAC_DATA<<=1;                     
			delay_us(6);
			CLK_HIGH;  
			delay_us(12);
			CLK_LOW; 
			delay_us(6);       
		}//end of for   
		LD_HIGH;   
	}		
}

