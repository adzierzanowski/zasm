# Z80 Assembler


This is a Zilog Z80 assembler.

I've already written a Z80 assembler in Python which is available in my
[z80 repository](https://github.com/adzierzanowski/z80/). The difference is that
`zasm` is a 2-pass asm whereas the python one passed the source several times and
is significantly slower.

## Syntax

```
; Comment (until newline)

label:
  ld hl, [msg + 2]           ; Indirect addressing is using square brackets.
                             ;   Expressions (including labels and characters)
                             ;   are allowed.
  jp $                       ; $ = current address
  RET NZ                     ; Instructions, conditions and directives are
                             ;   case-insensitive.

; Directives
include "another/file.s"     ; Include another source
org 0x8000                   ; Assume the code is loaded at address 0x8000
db "Hello, World!", 10, 0    ; Emit bytes
dw 'a'                       ; Emit words (this example will result in
                             ;   0x61 0x00 being emitted)
ds 100, 45                   ; Emit 100 bytes filled with value 45
def IDENTIFIER, 1            ; Define UART_PORT identifier as number 1
```

### Literals

Number formats allowed are:
* decimal: `97`,
* hexadecimal: `0x61` (however, `61h` is currently unsupported),
* octal: `0141`,
* and character-based: `'a'`.

Strings are delimited by `"`.

## CLI arguments

#### `-e`, `--export-labels`

Save labels and their corresponding addresses to a file.

#### `-l`, `--import-labels`

Import labels from a file.

#### `-v`, `--verbosity`

Make the output verbose with level `1` or `2`.

#### `-o`, `--output`

Emit the resulting binary into a file.

## TODO

* Implement binary include
