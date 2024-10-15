#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// #include <SDL2/SDL.h>
// #include <GL/gl.h>

/// @brief CHIP8 instruction representation structure
typedef union __ChInstruction {
    struct {
        union {
            uint8_t nn; ///< [0:8] 8 bits of additional data
            struct {
                uint8_t n   : 4; ///< [0:4] frequently used as opcode extender
                uint8_t y   : 4; ///< [4:8] frequently used as second register index
            };
        };
        uint8_t      x      : 4; ///< [ 8:12] frequently used as first register index
        uint8_t      opcode : 4; ///< [12:16] instruction opcode
    };
    struct {
        unsigned int nnn  : 12; ///< [0:12] 12 bits of additional data
    };
    uint16_t instruction; ///< whole instruction
} ChInstruction;

/// @brief CHIP8 instruction opcode
typedef enum __ChOpcode {
    CH_OPCODE_SYS    = 0x0, ///< util instruction header (such as CLS and JMP)
    CH_OPCODE_JUMP   = 0x1, ///< pc = nnn
    CH_OPCODE_CALL   = 0x2, ///< call function starting on nnn
    CH_OPCODE_SE_B   = 0x3, ///< skip next instruction if vX == nn
    CH_OPCODE_SNE_B  = 0x4, ///< skip next instruction if vX != nn
    CH_OPCODE_SE_R   = 0x5, ///< skip next instruction if vX == vY
    CH_OPCODE_LD_B   = 0x6, ///< vX  = nn
    CH_OPCODE_ADD_B  = 0x7, ///< vX += nn
    CH_OPCODE_RR     = 0x8, ///< set of common vX-vY instructions (such as ADD, SUB, etc.)
    CH_OPCODE_SNE_R  = 0x9, ///< skip next instruction if vX != vY
    CH_OPCODE_LD_I   = 0xA, ///< I = nnn
    CH_OPCODE_JP_V0  = 0xB, ///< pc = v0 + nnn
    CH_OPCODE_RND    = 0xC, ///< vX = [random byte] & nn
    CH_OPCODE_DRW    = 0xD, ///< display n-byte sprite at (vX, vY)
    CH_OPCODE_KEY    = 0xE, ///< input-related instructions
    CH_OPCODE_SPEC   = 0xF, ///< special instructions (interactions with system clock, memory, etc.)
} ChOpcode;

/// @brief CHIP8 Register-register instruction opcode extension
typedef enum __ChOpcodeRr {
    CH_OPCODE_RR_LD   = 0x0, ///< vX = vY
    CH_OPCODE_RR_OR   = 0x1, ///< vX = vX | vY
    CH_OPCODE_RR_AND  = 0x2, ///< vX = vX & vY
    CH_OPCODE_RR_XOR  = 0x3, ///< vX = vX ^ vY
    CH_OPCODE_RR_ADD  = 0x4, ///< vX = vX + vY
    CH_OPCODE_RR_SUB  = 0x5, ///< vX = vX - vY
    CH_OPCODE_RR_SHR  = 0x6, ///< vX = vX >> 1
    CH_OPCODE_RR_SUBN = 0x7, ///< vX = vY - vX
    CH_OPCODE_RR_SHL  = 0xE, ///< vX = vX << 1
} ChOpcodeRr;

/// @brief CHIP8 Special instruction opcode extension
typedef enum __ChOpcodeSpec {
    CH_OPCODE_SPEC_GET_DT           = 0x06, ///< vX = dt
    CH_OPCODE_SPEC_GET_PRESSED      = 0x0A, ///< vX = [index of first key pressed]
    CH_OPCODE_SPEC_SET_DT           = 0x15, ///< dt = vX
    CH_OPCODE_SPEC_SET_ST           = 0x18, ///< st = vX
    CH_OPCODE_SPEC_ADD_I            = 0x1E, ///< I += vX
    CH_OPCODE_SPEC_GET_DIGIT_SPRITE = 0x29, ///< I = [sprite for digit vX location]
    CH_OPCODE_SPEC_STORE_BCD        = 0x33, ///< I[0..2] = [(vX / 100) % 10, (vX / 10) % 10, (vX / 1) % 10]
    CH_OPCODE_SPEC_STORE            = 0x55, ///< I[0..] = [v0, v1, ... vX]
    CH_OPCODE_SPEC_LOAD             = 0x65, ///< [v0, v1, ... vX] = I[0..]
} ChOpcodeSpec;

/// @brief CHIP8 vm register storage
typedef struct __ChRegisters {
    uint8_t  v[16]; ///< general purpose registers
    uint16_t i;     ///< 16-byte register
    uint16_t pc;    ///< program counter
    uint8_t  dt;    ///< delay timer
    uint8_t  st;    ///< beep delay
    uint16_t sp;    ///< stack pointer
} ChRegisters;

/// @brief CHIP8 memory represetnation structure
typedef struct __ChMemory {
    uint8_t bytes[4096]; ///< memory bytes
} ChMemory;

/// @brief CHIP8 stack representation structure
typedef struct __ChStack {
    uint16_t stack[16]; /// stack fields
} ChStack;

/// @brief CHIP8 video memory representation structure
typedef struct __ChVideoMemory {
    uint64_t rows[32]; ///< set of video memory rows
} ChVideoMemory;

/// @brief CHIP8 virtual machine state
typedef struct __ChVm {
    ChRegisters   registers;   ///< registers
    ChStack       stack;       ///< stack
    ChVideoMemory videoMemory; ///< video memory
    ChMemory      memory;      ///< heap
} ChVm;

/// @brief CHIP8 program representation enumeration
typedef struct __ChProgram {
    uint16_t instructionCount; ///< instruction count
    uint16_t instructions[];   ///< instructions
} ChProgram;

void chVmRun( ChVm *vm ) {
    system(" ");

    while (true) {
        if (vm->registers.pc + 2 >= 0xFFF)
            break;

        ChInstruction instruction = {
            .instruction = *(uint16_t *)(vm->memory.bytes + vm->registers.pc),
        };
        // reorder instruction bytes
        instruction.instruction = 0
            | (instruction.instruction << 8)
            | (instruction.instruction >> 8)
        ;
        vm->registers.pc += 2;

        switch (instruction.opcode) {
        case CH_OPCODE_SYS : {
            switch (instruction.nnn) {
            case 0x0E0: {
                // CLS
                printf("\x1b[2J\x1b[H");
                for (uint32_t y = 0; y < 32; y++) {
                    vm->videoMemory.rows[y] = 0;

                    for (uint32_t x = 0; x < 64; x++)
                        putchar(' ');
                    putchar('|');
                    putchar('\n');
                }
                printf("\x1b[H");
                break;
            }
            case 0x0EE: {
                // RET
                if (vm->registers.sp == 0) {
                    puts("ERROR: stack underflow\n");
                    goto chVmRun__end;
                } else {
                    vm->registers.pc = vm->stack.stack[--vm->registers.sp];
                }
                break;
            }
            }
            break;
        }

        case CH_OPCODE_JUMP   : {
            vm->registers.pc = instruction.nnn;
            break;
        }

        case CH_OPCODE_CALL   : {
            if (vm->registers.sp >= 16) {
                puts("ERROR: stack overflow\n");
                goto chVmRun__end;
            } else {
                vm->stack.stack[vm->registers.sp++] = vm->registers.pc;
            }
            break;
        }

        case CH_OPCODE_SE_B   : {
            if (vm->registers.v[instruction.x] == instruction.nn) {
                vm->registers.pc += 2;
            }
            break;
        }

        case CH_OPCODE_SNE_B  : {
            if (vm->registers.v[instruction.x] != instruction.nn) {
                vm->registers.pc += 2;
            }
            break;
        }

        case CH_OPCODE_SE_R   : {
            if (instruction.n != 0) {
                printf("ERROR: invalid 'SE' instruction: n = %hhd (0 expected)\n", instruction.n);
                goto chVmRun__end;
            }

            if (vm->registers.v[instruction.x] == vm->registers.v[instruction.y]) {
                vm->registers.pc += 2;
            }
            break;
        }

        case CH_OPCODE_LD_B   : {
            vm->registers.v[instruction.x] = instruction.nn;
            break;
        }

        case CH_OPCODE_ADD_B  : {
            vm->registers.v[instruction.x] += instruction.nn;
            break;
        }

        case CH_OPCODE_RR     : {
            uint8_t *x = vm->registers.v + instruction.x;
            uint8_t y = vm->registers.v[instruction.y];

            switch ((ChOpcodeRr)instruction.n) {
            case CH_OPCODE_RR_LD   : {
                *x = y;
                break;
            }

            case CH_OPCODE_RR_OR   : {
                *x = *x | y;
                break;
            }

            case CH_OPCODE_RR_AND  : {
                *x = *x & y;
                break;
            }

            case CH_OPCODE_RR_XOR  : {
                *x = *x ^ y;
                break;
            }

            case CH_OPCODE_RR_ADD  : {
                uint16_t res = (uint16_t)*x + y;
                vm->registers.v[15] = res >> 8;
                *x = res;
                break;
            }

            case CH_OPCODE_RR_SUB  : {
                vm->registers.v[15] = *x > y;
                *x = *x - y;
                break;
            }

            case CH_OPCODE_RR_SHR  : {
                vm->registers.v[15] = *x & 1;
                *x = *x >> 1;
                break;
            }

            case CH_OPCODE_RR_SUBN : {
                vm->registers.v[15] = y > *x;
                *x = y - *x;
                break;
            }

            case CH_OPCODE_RR_SHL  : {
                vm->registers.v[15] = *x >> 7;
                *x = *x << 1;
                break;
            }

            default: {
                puts("ERROR: unknown register-register sub-instruction.\n");
                goto chVmRun__end;
            }
            }
            break;
        }

        case CH_OPCODE_SNE_R  : {
            if (instruction.n != 0) {
                puts("ERROR: invalid instruction\n");
                goto chVmRun__end;
            }

            if (vm->registers.v[instruction.x] != vm->registers.v[instruction.y]) {
                vm->registers.pc += 2;
            }
            break;
        }

        case CH_OPCODE_LD_I   : {
            vm->registers.i = instruction.nnn;
            break;
        }

        case CH_OPCODE_JP_V0  : {
            vm->registers.pc = vm->registers.v[0] + instruction.nnn;
            break;
        }

        case CH_OPCODE_RND    : {
            // TODO
            break;
        }

        case CH_OPCODE_DRW    : {
            // update data in video memory
            uint8_t vx = vm->registers.v[instruction.x];
            uint8_t vy = vm->registers.v[instruction.y];
            uint16_t i = vm->registers.i;

            for (uint8_t row = 0; row < instruction.n; row++) {
                if (vy + row >= 32)
                    break;

                uint8_t cbyte = vm->memory.bytes[i + row];
                vm->videoMemory.rows[vy + row] ^= (uint64_t)cbyte << vx;
            }

            // actually, redraw region
            printf("\x1b[%hhd;%hhdH", vy + 1, vx + 1);
            for (uint8_t row = 0; row < instruction.n; row++) {
                if (vy + row >= 32)
                    break;

                uint8_t colors = vm->videoMemory.rows[vy + row] >> vx;

                for (uint8_t x = 0; x < 8; x++)
                    putchar((colors >> (7 - x)) & 1 ? '#' : ' ');
                // 8 left 1 down
                printf("\x1b[8D\x1b[1B");
            }
            printf("\n");
            break;
        }

        case CH_OPCODE_KEY   : {
            break;
        }

        case CH_OPCODE_SPEC    : {
            switch ((ChOpcodeSpec)instruction.nn) {
            case CH_OPCODE_SPEC_GET_DT           : {
                vm->registers.v[instruction.x] = vm->registers.dt;
                break;
            }
            case CH_OPCODE_SPEC_GET_PRESSED      : {
                // TODO
                break;
            }
            case CH_OPCODE_SPEC_SET_DT           : {
                vm->registers.dt = vm->registers.v[instruction.x];
                break;
            }
            case CH_OPCODE_SPEC_SET_ST           : {
                vm->registers.st = vm->registers.v[instruction.x];
                break;
            }
            case CH_OPCODE_SPEC_ADD_I            : {
                vm->registers.i += vm->registers.v[instruction.x];
                break;
            }
            case CH_OPCODE_SPEC_GET_DIGIT_SPRITE : {
                if (vm->registers.v[instruction.x] >= 16) {
                    printf("ERROR: '%hhd' is not hexadecimal digit\n", vm->registers.v[instruction.x]);
                    goto chVmRun__end;
                }
                vm->registers.i = vm->registers.v[instruction.x] * 5;
                break;
            }
            case CH_OPCODE_SPEC_STORE_BCD        : {
                if (vm->registers.i + 2 >= 0xFFF) {
                    printf("ERROR: cannot write 3 bytes at at %X address.\n", vm->registers.i);
                    goto chVmRun__end;
                }
                uint8_t *digits = vm->memory.bytes + vm->registers.i;
                digits[0] = (vm->registers.v[instruction.x] / 100) % 10;
                digits[1] = (vm->registers.v[instruction.x] /  10) % 10;
                digits[2] = (vm->registers.v[instruction.x] /   1) % 10;
                break;
            }
            case CH_OPCODE_SPEC_STORE            : {
                if (vm->registers.i + instruction.x >= 0xFFF) {
                    printf("ERROR: cannot write %d bytes at %X address\n",
                        (int)instruction.x,
                        vm->registers.i
                    );
                    goto chVmRun__end;
                }
                memcpy(vm->memory.bytes + vm->registers.i, vm->registers.v, instruction.n);
                break;
            }
            case CH_OPCODE_SPEC_LOAD             : {
                uint8_t *ptr = vm->memory.bytes + vm->registers.i;

                if (vm->registers.i + instruction.x >= 0xFFF) {
                    printf("ERROR: cannot read %d bytes from %X address\n",
                        (int)instruction.x,
                        vm->registers.i
                    );
                    goto chVmRun__end;
                }
                memcpy(vm->registers.v, vm->memory.bytes + vm->registers.i, instruction.x);
                break;
            }
            default: {
                printf("ERROR: unknown special instruction '%X'\n", instruction.nn);
            }
            }
            break;
        }

        }
    }

chVmRun__end:
    return;
}



int main( void ) {
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80, // F
    };
    const uint8_t program[] = {
        // IBM Logo
        0x00, 0xe0, 0xa2, 0x2a, 0x60, 0x0c, 0x61, 0x08, 0xd0, 0x1f, 0x70, 0x09, 0xa2, 0x39, 0xd0, 0x1f,
        0xa2, 0x48, 0x70, 0x08, 0xd0, 0x1f, 0x70, 0x04, 0xa2, 0x57, 0xd0, 0x1f, 0x70, 0x08, 0xa2, 0x66,
        0xd0, 0x1f, 0x70, 0x08, 0xa2, 0x75, 0xd0, 0x1f, 0x12, 0x28, 0xff, 0x00, 0xff, 0x00, 0x3c, 0x00,
        0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0x38, 0x00, 0x3f,
        0x00, 0x3f, 0x00, 0x38, 0x00, 0xff, 0x00, 0xff, 0x80, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0x80, 0x00,
        0x80, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0x80, 0xf8, 0x00, 0xfc, 0x00, 0x3e, 0x00, 0x3f, 0x00, 0x3b,
        0x00, 0x39, 0x00, 0xf8, 0x00, 0xf8, 0x03, 0x00, 0x07, 0x00, 0x0f, 0x00, 0xbf, 0x00, 0xfb, 0x00,
        0xf3, 0x00, 0xe3, 0x00, 0x43, 0xe5, 0x05, 0xe2, 0x00, 0x85, 0x07, 0x81, 0x01, 0x80, 0x02, 0x80,
        0x07, 0xe1, 0x06, 0xe7,


        // CHIP8 logo
        // 0x00, 0xe0, 0x61, 0x01, 0x60, 0x08, 0xa2, 0x50, 0xd0, 0x1f, 0x60, 0x10, 0xa2, 0x5f, 0xd0, 0x1f,
        // 0x60, 0x18, 0xa2, 0x6e, 0xd0, 0x1f, 0x60, 0x20, 0xa2, 0x7d, 0xd0, 0x1f, 0x60, 0x28, 0xa2, 0x8c,
        // 0xd0, 0x1f, 0x60, 0x30, 0xa2, 0x9b, 0xd0, 0x1f, 0x61, 0x10, 0x60, 0x08, 0xa2, 0xaa, 0xd0, 0x1f,
        // 0x60, 0x10, 0xa2, 0xb9, 0xd0, 0x1f, 0x60, 0x18, 0xa2, 0xc8, 0xd0, 0x1f, 0x60, 0x20, 0xa2, 0xd7,
        // 0xd0, 0x1f, 0x60, 0x28, 0xa2, 0xe6, 0xd0, 0x1f, 0x60, 0x30, 0xa2, 0xf5, 0xd0, 0x1f, 0x12, 0x4e,
        // 0x0f, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x1f, 0x3f, 0x71, 0xe0, 0xe5, 0xe0, 0xe8, 0xa0,
        // 0x0d, 0x2a, 0x28, 0x28, 0x28, 0x00, 0x00, 0x18, 0xb8, 0xb8, 0x38, 0x38, 0x3f, 0xbf, 0x00, 0x19,
        // 0xa5, 0xbd, 0xa1, 0x9d, 0x00, 0x00, 0x0c, 0x1d, 0x1d, 0x01, 0x0d, 0x1d, 0x9d, 0x01, 0xc7, 0x29,
        // 0x29, 0x29, 0x27, 0x00, 0x00, 0xf8, 0xfc, 0xce, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x49, 0x4a, 0x49,
        // 0x48, 0x3b, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x01, 0xf0, 0x30, 0x90, 0x00, 0x00, 0x80,
        // 0x00, 0x00, 0x00, 0xfe, 0xc7, 0x83, 0x83, 0x83, 0xc6, 0xfc, 0xe7, 0xe0, 0xe0, 0xe0, 0xe0, 0x71,
        // 0x3f, 0x1f, 0x00, 0x00, 0x07, 0x02, 0x02, 0x02, 0x02, 0x39, 0x38, 0x38, 0x38, 0x38, 0xb8, 0xb8,
        // 0x38, 0x00, 0x00, 0x31, 0x4a, 0x79, 0x40, 0x3b, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
        // 0x00, 0x00, 0xa0, 0x38, 0x20, 0xa0, 0x18, 0xce, 0xfc, 0xf8, 0xc0, 0xd4, 0xdc, 0xc4, 0xc5, 0x00,
        // 0x00, 0x30, 0x44, 0x24, 0x14, 0x63, 0xf1, 0x03, 0x07, 0x07, 0x77, 0x17, 0x63, 0x71, 0x00, 0x00,
        // 0x28, 0x8e, 0xa8, 0xa8, 0xa6, 0xce, 0x87, 0x03, 0x03, 0x03, 0x87, 0xfe, 0xfc, 0x00, 0x00, 0x60,
        // 0x90, 0xf0, 0x80, 0x70
    };

    ChVm vm = {0};
    memcpy(vm.memory.bytes, font, sizeof(font));
    memcpy(vm.memory.bytes + 0x200, program, sizeof(program));
    vm.registers.pc = 0x200;

    chVmRun(&vm);
}
