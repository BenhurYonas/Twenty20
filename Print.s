; Print.s
; Student names: change this to your names or look very silly
; Last modification date: change this to the last modification date or look very silly
; Runs on LM4F120 or TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; ST7735_OutChar   outputs a single 8-bit ASCII character
; ST7735_OutString outputs a null-terminated string 

       EXPORT   ConvertUDec
       EXPORT   ConvertDistance
       EXPORT   String
       AREA  DATA, ALIGN=2
String SPACE 12 ; unsigned char String[10]
       AREA    |.text|, CODE, READONLY, ALIGN=2
       THUMB

    MACRO
    UMOD  $Mod,$Divnd,$Divsr ;MOD,DIVIDEND,DIVISOR
    UDIV  $Mod,$Divnd,$Divsr ;Mod = DIVIDEND/DIVISOR
    MUL   $Mod,$Mod,$Divsr   ;Mod = DIVISOR*(DIVIDEND/DIVISOR)
    SUB   $Mod,$Divnd,$Mod   ;Mod = DIVIDEND-DIVISOR*(DIVIDEND/DIVISOR)
    MEND
 
 
; Local variables
; each allocated 1 byte of memory (8 bits; must store as LDRB/STRB)
first 	equ 0
second 	equ 1
third 	equ 2
fourth 	equ 3	

;-----------------------ConvertUDec-----------------------
; Convert a 32-bit number in unsigned decimal format
; Input: R0, 32-bit number to be transferred
; Output: none
; Lab 11 requirement is for at least one local variable on the stack with symbolic binding
; Converts a 32-bit number in unsigned decimal format
; Output: store the conversion in global variable String[12]
; Fixed format 4 digits, one space after, null termination
; Examples
;    4 to "   4 "  
;   31 to "  31 " 
;  102 to " 102 " 
; 2210 to "2210 "
;10000 to "**** "  any value larger than 9999 converted to "**** "
; Invariables: This function must not permanently modify registers R4 to R11
ConvertUDec
; copy your lab 11 solution here
	SUB SP, #16 ; allocate memory for local variables
	MOV R1, #0x00 ; clear counter
	MOV R12, #0x270F ; check for > 9999
	CMP R0, R12
	BHI StarUDec
	
	MOV R12, #0x0A 
	UMOD R2, R0, R12 ; obtain LS Digit
	ADD R2, #0x30 ; ASCII translation of number
	STRB R2, [SP, #fourth] ; store into fourth memory slot
	UDIV R0, R0, R12 ; shift number to right, clearing previous LS digit
	ADD R1, #0x01
	CMP R0, #0x00 ; if number is zero, no more digits to convert/store
	BEQ SpaceUDec
	
	UMOD R2, R0, R12
	ADD R2, #0x30
	STRB R2, [SP, #third]
	UDIV R0, R0, R12
	ADD R1, #0x01
	CMP R0, #0x00
	BEQ SpaceUDec
	
	UMOD R2, R0, R12
	ADD R2, #0x30
	STRB R2, [SP, #second]
	UDIV R0, R0, R12
	ADD R1, #0x01
	CMP R0, #0x00
	BEQ SpaceUDec
	
	UMOD R2, R0, R12
	ADD R2, #0x30
	STRB R2, [SP, #first]
	ADD R1, #0x01


SpaceUDec
	MOV R12, #0x04
	SUBS R1, R12, R1 ;if all four are filled, no spaces needed
	BEQ StoreUDec
	
	MOV R12, #0x20
	STRB R12, [SP, #first] ; add space to first memory slot
	SUBS R1, #0x01
	BEQ StoreUDec
	
	STRB R12, [SP, #second]
	SUBS R1, #0x01
	BEQ StoreUDec
	
	STRB R12, [SP, #third]
	SUBS R1, #0x01
	BEQ StoreUDec
	
	STRB R12, [SP, #fourth]
	B StoreUDec
	
StarUDec
	MOV R12, #0x2A 
	STRB R12, [SP, #first]
	STRB R12, [SP, #second]
	STRB R12, [SP, #third]
	STRB R12, [SP, #fourth]
	
StoreUDec
	LDR R2, =String
	LDRB R1, [SP, #first]
	STR R1, [R2]
	ADD R2, #0x01
	
	LDRB R1, [SP, #second]
	STR R1, [R2]
	ADD R2, #0x01
	
	LDRB R1, [SP, #third]
	STR R1, [R2]
	ADD R2, #0x01
	
	LDRB R1, [SP, #fourth]
	STR R1, [R2]
	ADD R2, #0x01
	
	MOV R12, #0x20
	STR R12, [R2]
	ADD R2, #0x01
	
	MOV R12, #0x00
	STR R12, [R2]
	ADD R2, #0x01
	
	
	ADD SP, #16
    
    BX  LR
;* * * * * * * * End of ConvertUDec * * * * * * * *

; -----------------------ConvertDistance-----------------------
; Converts a 32-bit distance into an ASCII string
; Input: R0, 32-bit number to be converted (resolution 0.001cm)
; Output: store the conversion in global variable String[12]
; Fixed format 1 digit, point, 3 digits, space, units, null termination
; Examples
;    4 to "0.004 cm"  
;   31 to "0.031 cm" 
;  102 to "0.102 cm" 
; 2210 to "2.210 cm"
;10000 to "*.*** cm"  any value larger than 9999 converted to "*.*** cm"
; Invariables: This function must not permanently modify registers R4 to R11
; Lab 11 requirement is for at least one local variable on the stack with symbolic binding
ConvertDistance
; copy your lab 11 solution here
	SUB SP, #16
	MOV R1, #0x00
	MOV R12, #0x270F
	CMP R0, R12
	BHI StarDist
	
	MOV R12, #0x0A
	UMOD R2, R0, R12
	ADD R2, #0x30
	STRB R2, [SP, #fourth]
	UDIV R0, R0, R12
	ADD R1, #0x01
	CMP R0, #0x00
	BEQ SpaceDist
	
	UMOD R2, R0, R12
	ADD R2, #0x30
	STRB R2, [SP, #third]
	UDIV R0, R0, R12
	ADD R1, #0x01
	CMP R0, #0x00
	BEQ SpaceDist
	
	UMOD R2, R0, R12
	ADD R2, #0x30
	STRB R2, [SP, #second]
	UDIV R0, R0, R12
	ADD R1, #0x01
	CMP R0, #0x00
	BEQ SpaceDist
	
	UMOD R2, R0, R12
	ADD R2, #0x30
	STRB R2, [SP, #first]
	ADD R1, #0x01


SpaceDist
	MOV R12, #0x04
	SUBS R1, R12, R1
	BEQ StoreDist
	
	MOV R12, #0x30
	STRB R12, [SP, #first]
	SUBS R1, #0x01
	BEQ StoreDist
	
	STRB R12, [SP, #second]
	SUBS R1, #0x01
	BEQ StoreDist
	
	STRB R12, [SP, #third]
	SUBS R1, #0x01
	BEQ StoreDist
	
	STRB R12, [SP, #fourth]
	B StoreDist
	
StarDist
	MOV R12, #0x2A
	STRB R12, [SP, #first]
	STRB R12, [SP, #second]
	STRB R12, [SP, #third]
	STRB R12, [SP, #fourth]


StoreDist
	LDR R2, =String
	LDRB R1, [SP, #first]
	STR R1, [R2]
	ADD R2, #0x01
	
	MOV R12, #0x2E
	STR R12, [R2]
	ADD R2, #0x01
	
	LDRB R1, [SP, #second]
	STR R1, [R2]
	ADD R2, #0x01
	
	LDRB R1, [SP, #third]
	STR R1, [R2]
	ADD R2, #0x01
	
	LDRB R1, [SP, #fourth]
	STR R1, [R2]
	ADD R2, #0x01
	
	MOV R12, #0x20
	STR R12, [R2]
	ADD R2, #0x01
	
	MOV R12, #0x63
	STR R12, [R2]
	ADD R2, #0x01
	
	MOV R12, #0x6D
	STR R12, [R2]
	ADD R2, #0x01
	
	MOV R12, #0x00
	STR R12, [R2]
	ADD R2, #0x01
	
	
	ADD SP, #16

    BX   LR
 
     ALIGN
;* * * * * * * * End of ConvertDistance * * * * * * * *

     ALIGN          ; make sure the end of this section is aligned
     END            ; end of file
