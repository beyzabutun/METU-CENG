#include<p18f8722.inc>
	
CONFIG OSC=HSPLL, FCMEN=OFF, IESO=OFF,PWRT=OFF,BOREN=OFF, WDT=OFF, MCLRE=ON, LPT1OSC=OFF, LVP=OFF, XINST=OFF, DEBUG=OFF
    
    
val1        udata 0X20
val1
val2	    udata 0x22
val2
val3	    udata 0x24
val3
	    
UDATA_ACS
  t1	res 1	; used in delay
  t2	res 1	; used in delay
  t3	res 1	; used in delay
  state res 1	; controlled by RB0 button
	    
ORG     0x00
goto    main
	
init:
    clrf LATA
    clrf LATB
    clrf LATC
    clrf LATD
    clrf LATE
    
    clrf PORTA
    movlw b'00010000'
    movwf TRISA
    
    movlw h'00'
    movwf TRISB
    movwf TRISC
    movwf TRISD
    movwf val1
    movwf val2
    movwf val3
    
    clrf PORTE
    
    movlw b'00011000'
    movwf TRISE
       
    movlw h'0F'
    movwf ADCON1    ; Configure A/D
    ;turn on first four LEDs of corresponding ports
    movwf LATB 
    movwf LATC
    ;all LEDs of corresponding ports
    movlw h'FF'
    movwf LATD
    return
    
start:
    call delay
    movlw h'00'
    movwf LATB 
    movwf LATC 
    movwf LATD
    movwf val1
    movwf val2
    movwf val3
waitforRE4:
    btfss   PORTA, 4	    ;check if ra4 pressed
    goto    waitforRE4	    ;skip if pressed
    goto action_1
    
delay		; Time Delay Routine with 3 nested loops
    movlw 82	; Copy desired value to W
    movwf t3	; Copy W into t3
    _loop3:
	movlw 0xA0  ; Copy desired value to W
	movwf t2    ; Copy W into t2
	_loop2:
	    movlw 0x9F	; Copy desired value to W
	    movwf t1	; Copy W into t1
	    _loop1:
		decfsz t1,F ; Decrement t1. If 0 Skip next instruction
		goto _loop1 ; ELSE Keep counting down
		decfsz t2,F ; Decrement t2. If 0 Skip next instruction
		goto _loop2 ; ELSE Keep counting down
		decfsz t3,F ; Decrement t3. If 0 Skip next instruction
		goto _loop3 ; ELSE Keep counting down
		return   
    
B_Check:
    btfsc PORTE, 4
    goto B_Check
    movlw 0x4
    cpfseq val1
    goto B_Inc
B_Reset:		;Reset val1 and LATB
    movlw 0x0
    movwf val1
    movwf LATB
    goto checkB1_E3_E4
B_Inc:			;Increase the value of val1 and turn on one more LED in LATB
    rlncf LATB
    incf LATB
    incf val1 
checkB1_E3_E4:		;Check whether RE3 or RE4 is pressed
    btfsc PORTE, 3   
    goto case_C		
    btfsc PORTE, 4 
    goto B_Check
    goto checkB1_E3_E4   
case_B:			;select port B
    btfsc PORTE, 3	
    goto case_B
    goto checkB1_E3_E4

    
C_Check:		;Check whether we should reset the system
    btfsc PORTE, 4
    goto C_Check
    movlw 0x4
    cpfseq val2
    goto C_Inc
C_Reset:		;Reset PORTC
    movlw 0x0
    movwf val2
    movwf LATC
    goto checkC1_E3_E4
C_Inc:			;Increase the value of val2 and turn on one more LED in LATC
    rlncf LATC
    incf LATC
    incf val2
checkC1_E3_E4:		;Check whether RE3 or RE4 is pressed
    btfsc PORTE, 3   
    goto release_RE3		
    btfsc PORTE, 4 
    goto C_Check
    goto checkC1_E3_E4
case_C:			;select port C
    btfsc PORTE, 3	;check if re3 is pressed
    goto case_C
    goto checkC1_E3_E4
    
case_D:			;select port D and show results
    movlw 0x0
    cpfseq val3
    goto turn_1
    goto start
turn_1:
    movlw 0x1
    cpfseq val3
    goto turn_2
    movlw b'00000001'
    movwf LATD
    goto start
turn_2:
    movlw 0x2
    cpfseq val3
    goto turn_3
    movlw b'00000011'
    movwf LATD
    goto start
turn_3:
    movlw 0x3
    cpfseq val3
    goto turn_4
    movlw b'00000111'
    movwf LATD
    goto start
turn_4:
    movlw 0x4
    cpfseq val3
    goto turn_5
    movlw b'00001111'
    movwf LATD
    goto start
turn_5:
    movlw 0x5
    cpfseq val3
    goto turn_6
    movlw b'00011111'
    movwf LATD
    goto start
turn_6:
    movlw 0x6
    cpfseq val3
    goto turn_7
    movlw b'00111111'
    movwf LATD
    goto start
turn_7:
    movlw 0x7
    cpfseq val3
    goto turn_8
    movlw b'01111111'
    movwf LATD
    goto start
turn_8:
    movlw b'11111111'
    movwf LATD
    goto start

check1_A4_E3:
    btfsc PORTA, 4	;check if ra4 is pressed
    goto action_2
    btfsc PORTE, 3	;skip if ra4 is pressed	
    goto case_B
    goto check1_A4_E3    
check2_A4_E3:
    btfsc PORTA, 4      ;check if ra4 pressed
    goto action_1	;skip if ra4 is not pressed	
    btfsc PORTE, 3
    goto case_B
    goto check2_A4_E3 
action_1:		;addition
    btfsc PORTA, 4      ;check if ra4 is released
    goto action_1	;skip if it is released
    call check1_A4_E3
    movlw 0x0
    movwf val3
    addwf val1,w
    addwf val2,w
    movwf val3
    goto case_D
action_2:		;substraction
    btfsc PORTA, 4      ;check if ra4 is released
    goto action_2	;skip if it is released	
    call check2_A4_E3
    movlw 0x0
    addwf val1,w
    cpfsgt val2		;check whether val1 or val2 is bigger
    goto v1_v2
    subwf val2,w
    movwf val3 
    goto case_D
v1_v2:
    movlw 0x0
    addwf val2,w
    subwf val1,w
    movwf val3 
    goto case_D
    
release_RE3:
    btfsc PORTE, 3
    goto release_RE3
    return
release_RE4:
    btfsc PORTE, 4
    goto release_RE4
    return
main:
    call init
    goto start
    end
    