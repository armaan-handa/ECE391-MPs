.text

.globl keyboard_linker, rtc_linker, pit_linker

keyboard_linker:
    pushal  # push all registers
    pushfl  # push all flags
    call keyboard_handler
    popfl   # pop all flags
    popal   # pop all registers
    iret

rtc_linker:
   pushal  # push all registers
   pushfl  # push all flags
   call rtc_handler
   popfl   # pop all flags
   popal   # pop all registers
   iret

pit_linker:
   pushal  # push all registers
   pushfl  # push all flags
   call pit_handler
   popfl   # pop all flags
   popal   # pop all registers
   iret
