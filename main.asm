;**************************************************************
;* This stationery serves as the framework for a              *
;* user application. For a more comprehensive program that    *
;* demonstrates the more advanced functionality of this       *
;* processor, please see the demonstration applications       *
;* located in the examples subdirectory of the                *
;* Freescale CodeWarrior for the HC12 Program directory       *
;**************************************************************

; export symbols
             XDEF asm_mydelay
             XDEF COMWRT4_asm
             XDEF DATWRT4_asm
            ; we use export 'Entry' as symbol. This allows us to
            ; reference 'Entry' either in the linker .prm file
            ; or from C/C++ later on

; Include derivative-specific definitions 
		INCLUDE 'derivative.inc'  		

		    
; variable/data section
MY_EXTENDED_RAM: SECTION
LCD_DATA	EQU PORTK		
LCD_CTRL	EQU PORTK		
RS	EQU mPORTK_BIT0	
EN	EQU mPORTK_BIT1	

;----------------------USE $1000-$2FFF for Scratch Pad 
R1      EQU     $1001
R2      EQU     $1002
R3      EQU     $1003
TEMP    EQU     $1100
ASMData: DS.B 1
; Insert here your data definition. For demonstration, temp_byte is used.

;Strings

; code section
MyCode:     SECTION
; this assembly routine is called by the C/C++ application
asm_mydelay:        
            tba ; A = B. B receives the parameter.          
            cmpa #$00
            beq next
            
oloop:      ldx #2400  ; #2400 (24MHz bus speed). It seems that the Bus speed by default is 24 MHz
iloop:      psha      ; 2 cycles
            pula      ; 3 cycles
            nop       ; 1 cycle
            nop       ; 1 cycle
            dbne X,iloop; ;3 cycles
            dbne A, oloop; 3 cycles
            
            movb #$00, ASMData; 4 cycles. Result in ASMData
            bra fini
            
next:       movb #$FF, ASMData; 4 cycles. Result in ASMData
fini:       RTC  
                  
;----------------------------
COMWRT4_asm:               		
		  STAB	TEMP		
		  ANDB  #$F0
		  LSRB
		  LSRB
		  STAB  LCD_DATA
		  BCLR  LCD_CTRL,RS 	
		  BSET  LCD_CTRL,EN 	
		  NOP
		  NOP
		  NOP				
		  BCLR  LCD_CTRL,EN 	
		  LDAB  TEMP
		  ANDB  #$0F
    	LSLB
    	LSLB
  		STAB  LCD_DATA
		  BCLR  LCD_CTRL,RS 	
		  BSET  LCD_CTRL,EN 	
		  NOP
		  NOP
		  NOP				
		  BCLR  LCD_CTRL,EN 	
		  RTC
;--------------		  
DATWRT4_asm:                   	
		  STAB	 TEMP		
		  ANDB   #$F0
		  LSRB
		  LSRB
		  STAB   LCD_DATA
		  BSET   LCD_CTRL,RS 	
		  BSET   LCD_CTRL,EN 	
		  NOP
		  NOP
		  NOP				
		  BCLR   LCD_CTRL,EN 	
		  LDAB   TEMP
		  ANDB   #$0F
    	LSLB
      LSLB
  		STAB   LCD_DATA
  		BSET   LCD_CTRL,RS
		  BSET   LCD_CTRL,EN 	
		  NOP
		  NOP
		  NOP				
		  BCLR   LCD_CTRL,EN 	
		  RTC
;-------------------
