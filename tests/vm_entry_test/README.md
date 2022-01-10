
# Example Output

```
> 0x7f4bd5fc8f8d push 0x6CBB0520
> 0x7f4bd5ed96a3 push r8
> 0x7f4bd5ed96ad push r9
> 0x7f4bd5f22602 push rdx
> 0x7f4bd5f22605 push r12
> 0x7f4bd5f2260c push r11
> 0x7f4bd5f22610 push rcx
> 0x7f4bd5f22619 push r10
> 0x7f4bd5fa22d2 push rsi
> 0x7f4bd5fa22d6 push r15
> 0x7f4bd5fa22de push rbp
> 0x7f4bd5fa22e8 push r13
> 0x7f4bd5fa22f2 push rdi
> 0x7f4bd5fa22f3 push rbx
> 0x7f4bd5f17ade push r14
> 0x7f4bd5f17ae6 push rax
> 0x7f4bd5f17aea pushfq
> 0x7f4bd5f786f3 mov r11, 0x7F4A95C90000
> 0x7f4bd5f786fd push r11
> 0x7f4bd5f78707 mov r8, [rsp+0x90]
> 0x7f4bd5f7870f not r8d
> 0x7f4bd5f78719 rol r8d, 0x01
> 0x7f4bd5f78722 bswap r8d
> 0x7f4bd5f78725 not r8d
> 0x7f4bd5f7872f lea r8, [r8+r11*1]
> 0x7f4bd5f78733 mov r11, 0x100000000
> 0x7f4bd5f7873f add r8, r11
> 0x7f4bd5f7874d mov r9, rsp
> 0x7f4bd5f78753 sub rsp, 0x180
> 0x7f4bd5f78763 and rsp, 0xFFFFFFFFFFFFFFF0
> 0x7f4bd5f7876c mov rbx, r8
> 0x7f4bd5f78775 mov rdx, 0x7F4A95C90000
> 0x7f4bd5f7877f sub rbx, rdx
> 0x7f4bd5f78782 lea rbp, [0x00007F4BD5F78782]
> 0x7f4bd5f7878e sub r8, 0x04
> 0x7f4bd5f7879a mov edx, [r8]
> 0x7f4bd5f787a1 xor edx, ebx
> 0x7f4bd5f50bcd inc edx
> 0x7f4bd5f50bd0 rol edx, 0x01
> 0x7f4bd5f50bda add edx, 0x50DE102C
> 0x7f4bd5f50be0 rol edx, 0x03
> 0x7f4bd5f50be4 push rbx
> 0x7f4bd5f50bee xor [rsp], edx
> 0x7f4bd5f50bf4 pop rbx
> 0x7f4bd5f50bf9 movsxd rdx, edx
> 0x7f4bd5f50c00 add rbp, rdx
> 0x7f4bd5ebab8f jmp rbp
> Starting Virtual Instruction Pointer Register: r8
> Starting Virtual Stack Pointer Register: r9
```
