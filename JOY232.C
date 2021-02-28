char recv(char* data, unsigned int bytes) __naked
{
    __asm

        LD IY,#2
        ADD IY,SP
        LD L,0(IY)
        LD H,1(IY)
        LD C,2(IY)
        LD B,3(IY)

        ;RECEIVE BC BYTES FROM JOYSTICK2 PIN1 TO (HL)
        ;MSX Z80 3.579545MHz, 57600bps, 62cycles/bit
        ;Z80+M1 TIMING IN BRACKETS REFER TO http://map.grauw.nl/resources/z80instr.php
        ;L (RETURN VALUE) HOLDS ERROR CODE: L=0x00 NO ERROR, L=0x01 LINE NOT READY

        RX:

            DI                  ;TIME CRITICAL OPERATION, DISABLE INTERRUPTS

            LD E,#0x01          ;FOR FASTER 'AND' OPERATION: 'AND E'(5) VS 'AND #0x01'(8)

            LD A,#0x0F          ;PSG REGISTER 15
            OUT (0xA0),A
            IN A,(0xA2)
            SET 6,A             ;SELECT JOYSTICK2
            OUT (0xA1),A

            LD A,#0x0E          ;PSG REGISTER 14
            OUT (0xA0),A

        RXMARK:

            IN A,(0xA2)         ;(12)*
            AND E               ;(5)
            JP NZ,RXSTARTBIT    ;(11)
            LD L,#0x01          ;ERROR: LINE NOT READY
            EI                  ;ENABLE INTERRUPTS
            RET

        RXSTARTBIT:             ;WAIT FOR STARTBIT

            IN A,(0xA2)         ;(12)* |
            AND E               ;(5)   | (28)
            JP NZ,RXSTARTBIT    ;(11)  |

        RXALIGNMENT:

            NOP                 ;(5) ALIGNMENT |
            NOP                 ;(5) ALIGNMENT |
            NOP                 ;(5) ALIGNMENT | (25)
            NOP                 ;(5) ALIGNMENT |
            NOP                 ;(5) ALIGNMENT |

        RXINITBITLOOP:

            LD D,B              ;(5) |
            LD B,#0x08          ;(8) | (32)

        RXBITLOOP:

            INC HL              ;(7) ALIGNMENT                      |
            DEC HL              ;(7) ALIGNMENT                      |
            IN A,(0xA2)         ;(12)* 0                            |
            RRCA                ;(5) SHIFT DATA BIT (0) -> CARRY    | (62)
            RR (HL)             ;(17) SHIFT CARRY -> (HL)           |
            DJNZ RXBITLOOP      ;(14/9)                             |

        RXNEXTBYTE:

            LD B,D              ;(5)  |
            INC HL              ;(7)  |
            DEC BC              ;(7)  |
            LD A,B              ;(5)  | (40)
            OR C                ;(5)  |
            JP NZ,RXSTARTBIT    ;(11) |

        RXEXIT:

            LD L,#0x00
            EI                  ;ENABLE INTERRUPTS
            RET

    __endasm;
}

void send(char* data, int bytes) __naked
{
    __asm

        LD IY,#2
        ADD IY,SP
        LD L,0(IY)
        LD H,1(IY)
        LD C,2(IY)
        LD B,3(IY)

        ;SEND BC BYTES FROM (HL) TO JOYSTICK2 PIN6
        ;TIME CRITICAL OPERATION, ALWAYS DISABLE INTERRUPTS BEFORE CALLING
        ;MSX Z80 3.579545MHz, 57600bps, 62cycles/bit
        ;Z80+M1 TIMING IN BRACKETS REFER TO http://map.grauw.nl/resources/z80instr.php

        TX:

            DI                  ;TIME CRITICAL OPERATION, DISABLE INTERRUPTS

            LD A,#0x0F          ;PSG REGISTER 15
            OUT (0xA0),A


            RES 2,A             ;PIN6 0V
            LD E,A              ;STORE IN E

            SET 2,A             ;PIN6 5V
            LD D,A              ;STORE IN D

        TXSTARTBIT:

            PUSH BC             ;(12)
            LD C,(HL)           ;(8)

            LD A,E              ;(5)
            OUT (0xA1),A        ;(12)* 0

            LD B,#8             ;(8)

        TXBITLOOP:

            NOP                 ;(5) ALIGNMENT  |
            NOP                 ;(5) ALIGNMENT  |
                                ;               |
            RRC C               ;(10)           |
                                ;               |
            LD A,E              ;(5) SET 0      |
            JR NC,TXBIT         ;(13/8)         |
            LD A,D              ;(5) SET 1      | (63)
                                ;               |
        TXBIT:                  ;               |
                                ;               |
            OUT (0xA1),A        ;(12)           |* -4 -3 -2 -1 0 +1 +2 +3
            DJNZ TXBITLOOP      ;(13/8)         |

        TXENDBITLOOP:

            POP BC              ;(11)

            NOP                 ;(5) ALIGNMENT
            NOP                 ;(5) ALIGNMENT
            NOP                 ;(5) ALIGNMENT
            ADD A,#0            ;(8) ALIGNMENT

        TXSTOPBIT:

            LD A,D              ;(5)
            OUT (0xA1),A        ;(12)* 0

        TXNEXTBYTE:

            INC HL              ;(7)
            DEC BC              ;(7)
            LD A,B              ;(5)
            OR C                ;(5)
            JP NZ,TXSTARTBIT    ;(11)

        TXEXIT:

            EI                  ;ENABLE INTERRUPTS
            RET                 ;(11)

    __endasm;
}

