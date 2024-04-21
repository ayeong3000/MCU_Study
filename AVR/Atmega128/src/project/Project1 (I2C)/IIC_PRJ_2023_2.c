/* Assignment for Atmega128 - M24C08 */
 

#include <mega128a.h>
#include <delay.h>

#define CLK_HIGH    PORTD |= 0x01   // PD.0=1 클락 1
#define CLK_LOW     PORTD &= 0xFE   // PD.0=0 클락 0
#define DAT_HIGH    PORTD |= 0x02   // PD.1=1 데이터 1
#define DAT_LOW     PORTD &= 0xFD   // PD.1=0 데이터 0

#define SCL_OUT     DDRD |= 0x01    // PD.0=1 클락 출력모드
#define SDA_IN      DDRD &= 0xFD    // PD.1=0 데이터 입력모드
#define SDA_OUT     DDRD |= 0x02    // PD.1=1 데이터 출력모드

typedef unsigned char U8;           // 데이터 타입 상수화

U8 DEV_ADD_W = 0xA0;                // 디바이스 어드레스 쓰기모드
U8 DEV_ADD_R = 0xA1;                // 디바이스 어드레스 읽기모드
U8 IIC_ADD = 0xAA;                  // 어드레스 지정
U8 IIC_DAT = 0x55;                  // 쓸 데이터 값
U8 READ_DATA = 0;                   // 읽어온 데이터 값
U8 EINT4_FLAG = 0;                  // inetrrupt 4번 flag
U8 EINT5_FLAG = 0;                  // inetrrupt 5번 flag
                                    

void IIC_START(void);               // START 함수
void IIC_STOP(void);                // STOP 함수
void ACK_write(void);               // ACK 함수  master -> slave
void no_ACK(void);                  // NACK 함수 master -> slave
void ACK_read(void);                // ACK 함수  slave -> master
void Process_8data(U8 value);       // Bytewrite 8비트 데이터 쓰기
void Process_8data_read(void);      // Randomread 8비트 데이터 받기 
void Byte_write(void);              // Bytewrite 함수 
void Random_read(void);             // Randomread 함수

void main(void){
    DDRC=0xFF;                      // 프로그램 확인 위한 LED
    PORTC=0xFF;                     // 초기 LED off clear
    EIMSK |= 0b00110000;            // 인터럽트 4,5 enable 
    EICRB |= 0b00001111;            // 인터럽트 4,5 rising edge 
    SREG |= 0x80;                   // 모든 인터럽트 허용 
    SCL_OUT;                        // 클락 출력
    SDA_OUT;                        // 데이터 출력          
                                      
    while(1){
    
        if(EINT4_FLAG==1){          // 인터럽트 4 발생시 Bytewrite 실행
            Byte_write(); 
            delay_ms(100);          // stop후 srat와의 간격  
        }//end of if  
        if(EINT5_FLAG==1){          // 인터럽트 5 발생시 Randomread 실행
            Random_read();                              
            delay_ms(100);
        }//end of if 
    }//end of while
}//end of main

void IIC_START(void){               //START 함수
    DAT_HIGH;                       //8MHZ 8000KHz-> 100Khz 1clock = 0.00001s -> 10us 
    delay_us(6);                    // delay_us(3)*4 =12us 만들려고 했으나 오실로스코프에 7-8us로 찍혀서 딜레이 추가.
    CLK_HIGH;   
    delay_us(6);
    DAT_LOW;    
    delay_us(6);
    CLK_LOW;    
    delay_us(6);
}//end of IIC_START

void IIC_STOP(void){               //STOP 함수
    SDA_OUT;                       //데이터 출력
    DAT_LOW;  
    delay_us(6);
    CLK_HIGH;   
    delay_us(6);
    DAT_HIGH;   
    delay_us(6);
    CLK_LOW;    
    delay_us(6);
}//end of IIC_STOP

void Process_8data(U8 value){    // Bytewrite 8비트 데이터 쓰기
    U8 i_8bit;                   // 8비트 데이터 써주기위한 반복문 변수
    U8 value_buffer;             // 쓸 데이터 버퍼값     
    value_buffer=value;
    SDA_OUT;                     //데이터 출력
    for(i_8bit=0;i_8bit<7;i_8bit++){ // 7비트 데이터 써주기위한 반복문

        if((value_buffer&0x80)==0x80)DAT_HIGH; // 써준 값의 최상위 비트가 1이면 데이터 HIGH
        else DAT_LOW;                          // 아니면 LOW
        value_buffer<<=1;                      // MSB-> LSB까지 하나씩  shift
        delay_us(6);
        CLK_HIGH;  
        delay_us(12);
        CLK_LOW; 
        delay_us(6);       
    }//end of for      
    if((value_buffer&0x80)==0x80)DAT_HIGH;     // LSB 따로 써줌 위의 반복문에서  value_buffer의 MSB에 데이터 LSB값이 있음           
    else DAT_LOW;            
    delay_us(6);
    CLK_HIGH;  
    delay_us(12);
    CLK_LOW;  
    if(value_buffer==0x80) DAT_LOW;           // data high 이면 ACK값이 clk이 high일때 입력받으므로  
    delay_us(6);                              // clk low 일때 데이터가 변경되는 파형모양을 위해 clk high 되기전 data 인위적으로로 low 해줌     
}//end of Process_8data

void Process_8data_read(void){                // Randomread 8비트 데이터 받기 
    
    U8 i, TEMP_READ_DATA = 0;
    SDA_IN;

    for (i = 0; i < 8; i++){                  // 8비트데이터 읽기
        CLK_LOW;
        delay_us(6);
        CLK_HIGH;
        delay_us(12);
        CLK_LOW;  
        delay_us(6);
        TEMP_READ_DATA = (PIND & 0x02 == 0x00) ? 0x00 : 0x80;// 데이터 읽은 값이 0이면 0x00 아니면 0x80
        TEMP_READ_DATA >>= i;                                // MSB부터 자리 정렬 shift
        READ_DATA |= TEMP_READ_DATA;                         // 읽은 데이터값 저장
    }//end of for  
    if(IIC_DAT==READ_DATA)PORTC=0xf0;        // write 값과 read 값이 일치하면 led 동작    
}//end of Process_8data_read

void ACK_read(void){                         // ACK 함수  slave -> master
    U8 i = 0;                                // ACK 확인받기위한 반복문 변수

    SDA_IN;                                  // 데이터 입력
    CLK_HIGH;                                // ACK 신호 받기준비
    for(i=10;i>0;i--){                       // 10번 반복중  ACK 받으면 반복문 빠져 나감
        if((PIND & 0x02) == 0x00){i=1;
        PORTC=0xf1;
        }
        else PORTC=0x00;                     // NACK 받았다면 LED 다 켜짐                           
    }//end of for
    delay_us(12);
    CLK_LOW;        
    SDA_OUT;                                 //데이터 출력
    delay_us(6);      
}//end of ACK_read

void ACK_write(void){     // ACK 함수  master -> slave
    SDA_OUT;              // 데이터 출력
    DAT_LOW;    
    CLK_HIGH;  
    delay_us(12);
    CLK_LOW; 
    delay_us(6);   
}//end of ACK_write

void no_ACK(void){       // NACK 함수  master -> slave
    SDA_OUT;  
    DAT_HIGH;    
    CLK_HIGH;  
    delay_us(12);
    CLK_LOW; 
    delay_us(6);  
}//end of no_ACK

void Byte_write(void){        // Bytewrite 함수 
    IIC_START();              // start
    Process_8data(DEV_ADD_W); // device address 7bit와 write 
    ACK_read();               // ack
    Process_8data(IIC_ADD);   // address 8bits
    ACK_read();               // ack
    Process_8data(IIC_DAT);   // data 8bits
    ACK_read();               // ack
    IIC_STOP();               // stop  
}//end of Byte_write   

void Random_read(void){      // Randomread 함수
    IIC_START();             // start
    Process_8data(DEV_ADD_W);// device address 7bit와 write 0
    ACK_read();              // ack
    Process_8data(IIC_ADD);  // address 8bits
    ACK_read();              // ack
    IIC_START();             // start
    Process_8data(DEV_ADD_R);// device address 7bit와 read 1
    ACK_read();              // ack
    Process_8data_read();    // data 8bits read
    no_ACK();                // nack 데이터 그만 주기 master->slave
    IIC_STOP();              // stop 
}//end of Random_read

interrupt [EXT_INT4] void external_int4(void){  //외부 인터럽트4 Bytewrite 동작 인터럽트
    SREG &= 0x7F;                               // All Interrupt disable   
    EINT5_FLAG = 0;                             // 인터럽트5 clear 
    EINT4_FLAG = 1;                             // 인터럽트4 발생          
    SREG |= 0x80;                               // All Interrupt enable  
}//end of external_int4

interrupt [EXT_INT5] void external_int5(void){  //외부 인터럽트5 Randomread 동작 인터럽트 
    SREG &= 0x7F;                               // All Interrupt disable   
    EINT4_FLAG = 0;                             // 인터럽트4 clear
    EINT5_FLAG = 1;                             // 인터럽트5 발생
    SREG |= 0x80;                               // All Interrupt enable  
}//end of external_int5
