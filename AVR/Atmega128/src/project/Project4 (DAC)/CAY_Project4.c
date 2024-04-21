/*                                             */
/* CAY_Project4.c                              */
/*                                             */
/* Created: 2023-11-22 오후 2:34:24             */
/* Author: 최아영                                */
/* Project subject : DAC                        */

#include <mega128a.h>
#include <delay.h>

#define CLK_HIGH    PORTD |= 0x01   // PD.0=1 클락 1
#define CLK_LOW     PORTD &= 0xFE   // PD.0=0 클락 0
#define DAT_HIGH    PORTD |= 0x02   // PD.1=1 데이터 1
#define DAT_LOW     PORTD &= 0xFD   // PD.1=0 데이터 0
#define LD_HIGH     PORTD |= 0x04   // PD.2=1 데이터 1
#define LD_LOW      PORTD &= 0xFB   // PD.2=0 데이터 0

#define SCL_OUT     DDRD |= 0x01    // PD.0=1 클락 출력모드
#define SDA_OUT     DDRD |= 0x02    // PD.1=1 데이터 출력모드
#define LD_OUT      DDRD |= 0x04    // PD.2=1 데이터 출력모드

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
 
 EIMSK |= 0b00010000;    //인터럽트 사용 여부 결정 레지스터로 INT4 enable
 EICRB |= 0b00000011;    //4번 Rising edge

 SCL_OUT;
 SDA_OUT;
 LD_OUT;
 
 SREG |= 0x80; 

while (1)
    {
        CLK_HIGH;
		LD_HIGH;   
		
        if(EINT4_FLAG_BIT==1){          // 인터럽트 4 발생시 Bytewrite 실행
		    EINT4_FLAG_BIT=0;
			PORTC=0xFE;				// Start
            DAC_write(); 	
            delay_ms(10);          // stop후 srat와의 간격 100us 

        }//end of if  
  }//end of while
}//end of main       

interrupt[EXT_INT4] void external_int4(void) {   //main CPU가 프로그램을 수행하고 있다가 interrupt가 발생하면 밑 서비스루틴을 실행,헤더파일에서 정의된 인터럽트 소스명 사용,함수명은 임의로 설정
    SREG &= 0x7F;                                //Nesting 방지를 위해 status resister bit7만 0으로 설정하여 모든 인터럽트를 차단(global interrupt disable)
    EINT4_FLAG_BIT = 1;                            //interrupt 짧게 프로그래밍하기 위해 이렇게 설정함. 인터럽트4가 들어와 1이되면 while문 안에 설정해둔 동작을 실행
    SREG |= 0x80;                                //status resister bit7만 1으로 설정하여 모든 인터럽트를 허용(global interrupt enable)
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

