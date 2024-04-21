
/*                                                                    */
/* project2.c                                                         */
/*                                                                    */
/* Created: 2023-11-21 오후 3:15:36                                    */
/* Author: 최아영                                                      */

#include <mega128.h>
#include <delay.h>
                                              
//typedef 전처리문 사용. bit수마다 다르게 적용
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
//const 이용 -> Segment를 나타내는 배열 생성
const U8 seg_pat[10]= {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
//Global 변수 설정. 초기화 하기
U8 EINT4_FLAG_BIT = 0;
U8 EINT5_FLAG_BIT = 0;
U8 EINT6_FLAG_BIT = 0;
U8 EINT7_FLAG_BIT = 0;

U16 TIMER1_CTC_FLAG_BIT = 0;
U8 TIMER2_OVF_FLAG_BIT = 0;
U16 TIMER2_OVF_COUNT = 0;

U8 ADC_RESULT_FLAG = 0; //온도값 받는 횟수
U32 TEMP_ADC_VALUE = 0; //온도값

U8 NA100, NA10, NA1;
U8 NV100=0, NV10=0, NV1=0;
U32 ad_value = 0;

//함수설정(알아보기 쉬운 이름으로)
void Seg4_out(U8 N1000, U8 N100, U8 N10, U8 N1);  //Segment 네자리 정수 출력 함수 정의(생년)
void Seg4_out2(U8 N1000, U8 N100, U8 N10, U8 N1); //Segment 네자리 정수 출력 함수 정의(월일)
void EXIT4_TIMER2(void);
void EXIT5_TIMER1(void);
void EXIT6_PWM_M6(void);
void EXIT7_PWM_M2(void);

void UART_volt(void);
void AD_display(int value);
void SEG7_TEMP_DIS(int val);


void main(void) {
    //DDRC = 0xFF;    PORTC = 0xFF;              //작동 확인용으로 LED 출력 설정
    DDRB = 0xF0;                                 //학번 디스플레이를 위한 포트B 4,5,6,7 출력 설정(LED)
    DDRD = 0xF0;                                 //학번 디스플레이를 위한 포트D 4,5,6,7 출력 설정(LED)
    DDRG = 0x0F;                                 //학번 디스플레이를 위한 포트G 0,1,2,3 출력 설정(COM)      

    Seg4_out(1, 9, 9, 9);                           //생년 표시
    Seg4_out2(1, 2, 2, 9);                          //월일 표시

    EIMSK = 0b11110000;                          //인터럽트 사용 여부 결정 레지스터로 INT4,5,6,7 enable
    EICRB = 0b10111011;                          //INT4,5,6,7은 B에 의해 Interrupt trigger 방식 설정(7fa,6ri,5fa,4ri)              
    SREG |= 0x80;                                //status resister bit7(MSB)만 1으로 설정하여 모든 인터럽트를 허용(global interrupt enable),OR을 사용하여 나머지비트는 그대로

    //while문 이전에는 CPU에 큰 영향을 안 줌. While문에서는 영향을 받기에 delay를 최적화 해줘야 함.    
    while (1) {
        if (ADC_RESULT_FLAG == 0) Seg4_out2(1, 2, 2, 9);     //온도 결과값이 없을 때는 학번을 계속 띄움
        else SEG7_TEMP_DIS(TEMP_ADC_VALUE);            //온도 값이 나올때는 온도를 디스플레이함

        if (EINT4_FLAG_BIT == 1) {                        //인터럽트가 발생하면 
            EINT4_FLAG_BIT = 0;                          //0으로 다시 바꿔주고
            EXIT4_TIMER2();                            //EXIT4_TIMER2함수를 실행한다.
        } //end of if
        if (EINT5_FLAG_BIT == 1) {
            EINT5_FLAG_BIT = 0;
            EXIT5_TIMER1();
        } //end of if
        if (EINT6_FLAG_BIT == 1) {
            EINT6_FLAG_BIT = 0;
            EXIT6_PWM_M6();
        } //end of if
        if (EINT7_FLAG_BIT == 1) {
            EINT7_FLAG_BIT = 0;
            EXIT7_PWM_M2();
        } //end of if  
        if (TIMER2_OVF_FLAG_BIT == 1) {                     //오버플로우 모드가 발생하면
            TIMER2_OVF_FLAG_BIT = 0;                      //0으로 바꿔주고
            TIMER2_OVF_COUNT++;                         //1씩 더해간다  
            if (TIMER2_OVF_COUNT == 1011) {                  //1229sec(주기가 1.216msec이므로 1011번)   
                TIMER2_OVF_COUNT = 0;
                ADC_RESULT_FLAG++;
                ADMUX = 7;                              // A/D 변환기 멀티플렉서 선택 레지스터 
                ADCSRA = 0xE2;                          // ADEN=1, ADSC=1, ADFR=1, 16MHz, 8분주  
                delay_us(100);                          // 변환시간이 길어져 발생하는 ADC오류의 방지를 위한 딜레이가 필요함 
                //while문을 써도된다고? -> 뭔 차이? ADC 강의자료 79 Page 참조.while문이건 delay건 컨버전하는 시간을 Cpu가 기다려준건데. while문은 ADC에서 A가 죽어버리면 영원히 거기서 못 빠져나오니까 While문 안쓴거.    
                TEMP_ADC_VALUE = (int)ADCL + ((int)ADCH << 8);  //A/D 변환값 읽기
                if (ADC_RESULT_FLAG == 10) {
                    ADC_RESULT_FLAG %= 10;                  // 10회동안만 온도 표시    
                    TIMSK &= 0b10111111;                   // TOIE2 = 0으로 설정하여 인터럽트가 안걸리게 하는 방법. 인터럽트를 써도 되지만 타이머 멈투는게 더 좋음.
                } //end of if  
            } //end of if
        } //end of if
        if (TIMER1_CTC_FLAG_BIT >= 1000) {
            TIMER1_CTC_FLAG_BIT = 0;
            ADMUX = 6;                                  //ADC6 단극성 입력 선택
            ADCSRA = 0x82;
            ADCSRA = 0xC2;                              // ADEN=1, 8분주 변환 시작

            while ((ADCSRA & 0x10) == 0);                // ADIF=1이 될 때까지(Single)
            ad_value = (int)ADCL + ((int)ADCH << 8);    // A/D 변환값 읽기
            AD_display(ad_value);                       // A/D 변환값 표시
            UART_volt();                                // 유아트 디스플레이를 위한 함수
        } //end of if

    } //end of while
} //end of main


//External INT 서브루틴
interrupt[EXT_INT4] void external_int4(void) {   //main CPU가 프로그램을 수행하고 있다가 interrupt가 발생하면 밑 서비스루틴을 실행,헤더파일에서 정의된 인터럽트 소스명 사용,함수명은 임의로 설정
    SREG &= 0x7F;                                //Nesting 방지를 위해 status resister bit7만 0으로 설정하여 모든 인터럽트를 차단(global interrupt disable)
    EINT4_FLAG_BIT = 1;                            //interrupt 짧게 프로그래밍하기 위해 이렇게 설정함. 인터럽트4가 들어와 1이되면 while문 안에 설정해둔 동작을 실행
    SREG |= 0x80;                                //status resister bit7만 1으로 설정하여 모든 인터럽트를 허용(global interrupt enable)
}

interrupt[EXT_INT5] void external_int5(void) {
    SREG &= 0x7F;
    EINT5_FLAG_BIT = 1;
    SREG |= 0x80;
}

interrupt[EXT_INT6] void external_int6(void) {
    SREG &= 0x7F;
    EINT6_FLAG_BIT = 1;
    SREG |= 0x80;
}

interrupt[EXT_INT7] void external_int7(void) {
    SREG &= 0x7F;
    EINT7_FLAG_BIT = 1;
    SREG |= 0x80;
}
//Timer INT 서브루틴
interrupt[TIM2_OVF] void TIMER2_OVF_int(void) {  //Timer 2 Overflow mode(생일로 1229msec)
    SREG &= 0x7F;
    TIMER2_OVF_FLAG_BIT=1;
    TCNT2 = 237;                                 //초기값을 인터럽트 안에도 써줘야함(최대한1.216msec로 맞춤)
    SREG |= 0x80;
}

interrupt[TIM1_COMPA] void TIMER1_CTC_int(void) { //Timer 1 CTC mode(생일로 12290msec)
    SREG &= 0x7F;
    TIMER1_CTC_FLAG_BIT++;
    SREG |= 0x80;
}


//함수 생성

//세그먼트 디스플레이
void Seg4_out(U8 N1000, U8 N100, U8 N10, U8 N1)    //생일 - 생년 디스플레이
{
    U8 i;
    for (i = 0; i < 50; i++) {                                          //50번이 될떄까지 디스플레이(너무 빨리지나가지 않기 위해)
        PORTG = 0b00001000;                                         //맨 우측 7Segment DIG4(PG3=1)ON, 1자리 표시
        PORTD = ((seg_pat[N1] & 0x0F) << 4) | (PORTD & 0x0F);       //[0]표시를 위해 ABCD표시, PORTD 하위 4bits 변경되지 않게 | 
        PORTB = (seg_pat[N1] & 0x70) | (PORTB & 0x0F);             //[0]표시를 위해 EFG표시, PORTB 하위 4bits 변경되지 않게 | 
        delay_ms(1);                                                //구별을 위한 딜레이
        PORTG = 0b00000100;                                         //우측에서 두번째 7Segment DIG3(PG2=1)ON, 10자리 표시
        PORTD = ((seg_pat[N10] & 0x0F) << 4) | (PORTD & 0x0F);
        PORTB = (seg_pat[N10] & 0x70) | (PORTB & 0x0F);
        delay_ms(1);
        PORTG = 0b00000010;                                         //우측에서 세번째 7Segment DIG2(PG1=1)ON, 100자리 표시
        PORTD = ((seg_pat[N100] & 0x0F) << 4) | (PORTD & 0x0F);
        PORTB = (seg_pat[N100] & 0x70) | (PORTB & 0x0F);
        delay_ms(1);
        PORTG = 0b00000001;                                         //우측에서 네번째 7Segment DIG1(PG0=1)ON, 1000자리 표시
        PORTD = ((seg_pat[N1000] & 0x0F) << 4) | (PORTD & 0x0F);
        PORTB = (seg_pat[N1000] & 0x70) | (PORTB & 0x0F);
        delay_ms(1);
    } //end of for    
}//end of Seg4_out

void Seg4_out2(U8 N1000, U8 N100, U8 N10, U8 N1)   //생일 - 월일 디스플레이
{
    PORTG = 0b00001000;                                          //while문 안에 있으니 for문 필요없음
    PORTD = ((seg_pat[N1] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[N1] & 0x70) | (PORTB & 0x0F);
    delay_ms(1);                                                //구별을 위한 최소한의 미세한 딜레이 
    PORTG = 0b00000100;
    PORTD = ((seg_pat[N10] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[N10] & 0x70) | (PORTB & 0x0F);
    delay_ms(1);
    PORTG = 0b00000010;
    PORTD = ((seg_pat[N100] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[N100] & 0x70) | (PORTB & 0x0F);
    delay_ms(1);
    PORTG = 0b00000001;
    PORTD = ((seg_pat[N1000] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[N1000] & 0x70) | (PORTB & 0x0F);
    delay_ms(1);
} //end of Seg4_out2

//External INT발생 시 타이머 사용
void EXIT4_TIMER2(void) { 
    TIMSK |= 0b01000000;                        // overflow interrupt enable
    TCCR2 = 0b00000101;                         // normal모드, 프리스케일= 1024분주
    TCNT2 = 237;                                // 174=5.184ms (학번뒷자리 51이라 8비트로 51ms불가)

} //end of EXIT4_TIMER2              

void EXIT5_TIMER1(void) {
    TCCR1A = 0b00000000;                        // mode4
    TCCR1B = 0b00001101;                        // mode4(CTC), 프리스케일 = 1024분주  
    TCCR1C = 0x00;

    TCNT1H = 0x0;                               //레지스터 초기화
    TCNT1L = 0x0;                               //레지스터 초기화   
    OCR1AH = 0x00;                              //191,hex00BF로 나온 것.(1024분주, 12.288msec)
    OCR1AL = 0xBF;                              
    TIMSK |= 0x10;                              //compare match interrupt enable
} //end of EXIT5_TIMER1

void EXIT6_PWM_M6(void) {
    TCCR3A = 0b10000010;                        //OC3A(PE3)에 파형출력, mode6(A=10, B=01) 
    TCCR3B = 0b00001101;                        //mode6(Fast PWM모드),1024분주(101) 
    TCCR3C = 0x00;

    TCNT3H = 0x0;                               //레지스터 초기화
    TCNT3L = 0x0;                               //레지스터 초기화    
    OCR3AH = 0x00;                               //듀티비(29%) => x/512 = 29%니까 x = 148
    OCR3AL = 148;

    DDRE |= 0x08;                               //TIMER3 PWM PIN PE3은 DDRE 3을 출력설정해야 PWM 파형이 출력됨
} //end of EXIT6_PWM_M6

void EXIT7_PWM_M2(void) {
    TCCR3A = 0b10000010;                        //OC3A(PE3)에 파형출력, mode2(A=10,B=00) 
    TCCR3B = 0b00000100;                        //mode2(PWM Phase correct 모드),256분주 
    TCCR3C = 0x00;

    TCNT3H = 0x0;                               //레지스터 초기화
    TCNT3L = 0x0;                               //레지스터 초기화   
    OCR3AH = 0x00;                               //듀티비(29%) => x/512 = 29%니까 x = 148
    OCR3AL = 148;

    DDRE |= 0x08;                               //PE3 출력 
} //end of EXIT7_PWM_M2

//온도값을 Segment에 디스플레이
void SEG7_TEMP_DIS(int val) {
    float fval;
    int ival, buf;

    fval = (float)val * 5.0 / 1023.0;             // 온도값으로 변환
    ival = (int)(fval * 1000.0);                  // 반올림 후 정수화(소수 첫째자리까지)    

    NA100 = ival / 100;                           // 정수부 10의 자리 추출
    buf = ival % 100;
    NA10 = buf / 10;                              // 한자리수 추출
    NA1 = buf % 10;                               // 소수 첫째 자리 추출 

    PORTG = 0b00001000;                           // PG3=1, 소수 첫째 자리
    PORTD = ((seg_pat[NA1] & 0x0F) << 4) | (PORTD & 0x0F);  //PORTD 하위 4bits 변경되지 않게 |
    PORTB = (seg_pat[NA1] & 0x70) | (PORTB & 0x0F);        //PORTB 하위 4bits 변경되지 않게 |
    delay_ms(1);
    PORTG = 0b00000100;                           // PG2=1
    PORTD = 0x00;
    PORTB = 0x80;                                 // DP on(소수점) 
    delay_ms(1);
    PORTG = 0b00000010;                           // PG1=1, 정수 일의 자리      
    PORTD = ((seg_pat[NA10] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[NA10] & 0x70) | (PORTB & 0x0F);
    delay_ms(1);
    PORTG = 0b00000001;                           // PG0=1,정수 10의 자리
    PORTD = ((seg_pat[NA100] & 0x0F) << 4) | (PORTD & 0x0F);
    PORTB = (seg_pat[NA100] & 0x70) | (PORTB & 0x0F);
    delay_ms(1);
} //end of SEG7_TEMP_DIS

//ADC를 통해 전압값 측정
void AD_display(int value) {                     //전압 값을 23.5도로 내보내야 함?
    float fvalue;
    int ivalue, buff;

    fvalue = (float)value * 5.0 / 1024.0;        // 전압 값으로 변환
    ivalue = (int)(fvalue * 100.0 + 0.5);        // 반올림 후 정수화,(소수 둘째자리까지) 

    NV100 = ivalue / 100;                        // 정수부 추출
    buff = ivalue % 100;

    NV10 = buff / 10;                            // 소수 첫째 자리 추출
    NV1 = buff % 10;                             // 소수 둘째 자리 추출
} //end of AD_display

//값을 UART 동작 -> PC화면에 디스플레이
void UART_volt(void) {
    UCSR0A = 0x0;               // USART 초기화 
    UCSR0B = 0b00001000;        // 송신 인에이블 TXEN = 1 
    UCSR0C = 0b00000110;        // 비동기[7], 데이터 8비트 모드 
    UBRR0H = 0;                 // X-TAL = 16MHz 일때, BAUD = 9600(Terminal Baud rate과 맞춰야 함)
    UBRR0L = 103;

    while ((UCSR0A & 0x20) == 0x0);     //송신 버퍼가 비어지면 UDRE0[5]= 1이 될 때까지 대기       
    UDR0 = NV100 + 0x30;                 //위 추출 숫자 아스키코드로 데이터 전송
    while ((UCSR0A & 0x20) == 0x0);
    UDR0 = '.';                        //문자'.'전송
    while ((UCSR0A & 0x20) == 0x0);
    UDR0 = NV10 + 0x30;                  //위 추출 숫자 아스키코드로 데이터 전송
    while ((UCSR0A & 0x20) == 0x0);
    UDR0 = NV1 + 0x30;                   //위 추출 숫자 아스키코드로 데이터 전송
    while ((UCSR0A & 0x20) == 0x0);
    UDR0 = 'V';                        //문자'V'전송
    while ((UCSR0A & 0x20) == 0x0);
    UDR0 = ' ';                        //알아보기 쉽게 띄어쓰기
} //end of UART_volt
