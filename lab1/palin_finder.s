.global _start

.section .text

_start:
	ldr r12, =input
	bl get_length // write length to r2
	mov r11, r3 // store length
	bl to_lower // convert to lowercase
	b check_palindrome // check palindrome in r12 of length r11

// char at r3 is <= Z, this checks if it is >= A, if so it makes it lowercase
possible_uppercase:
	cmp r3, #65 // if char is after A
	addge r3, #32 // 32 is diff between A and a
	bx lr

// converts string to lowercase
to_lower:
 // r0 counter
 	mov r0, #0
	push {lr} // needed because nested call
	to_lower_loop:
		ldrb r3, [r12, r0]
		cmp r3, #90 // if char is becore Z
		blle possible_uppercase // update character if it really is uppercase
		strb r3, [r12, r0] // write character that might have bee updated
		add r0, #1 // increment counter
		cmp r0, r11 // have we reached length
		popeq {pc} // if so jump back
		b to_lower_loop // if not, move on

get_length:
	mov r3, #0 // using r3 to return length
	loop:
		add r3, #1 //  increment length
		ldrb r2, [r12, r3] // use r2 to store character
		cmp r2, #0   // is it null terminated
		bne loop      // we have not found last character
	bx lr // back to start

check_palindrome:
	mov r1, r11 // count from last
	sub r1, #1 // dont care about terminating character
	mov r0, #0 // count from first
	paliloop:
		ldrb r2, [r12, r0] // beginning char
		ldrb r3, [r12, r1] // end char
		cmp r2, r3 // first and last char
		bne palindrome_not_found // not eq
		bl iterate_counters // iterate counters
		cmp r0, r1 // have the counters crossed?
		bge palindrome_found // if they have crossed we have checked entire word, know it's palindrome
		b paliloop // need to check more of word

// iterates register skipping over any spaces encountered
iterate_counters:
	// we have to iterate at least once
	add r0, #1
	sub r1, #1
	first_counter_loop:
		ldrb r2, [r12, r0]
		cmp r2, #32 // is char space?
		addeq r0, #1 // skip space
		beq first_counter_loop 	// might be more spaces
	second_counter_loop:
		ldrb r2, [r12, r1]
		cmp r2, #32 // is char space
		subeq r1, #1 // is so skip it
		beq second_counter_loop // might be more spaces after this one
	bx lr


palindrome_found:
	// Switch on only the 5 rightmost LEDs
	// Write 'Palindrome detected' to UART
	ldr r0, =0xff200000 // led address
	mov r1, #0x1f // 0b0000011111 led code
    str r1, [r0]  // write led code
	ldr r0, =found_output // set string param
	b write_res // write string


palindrome_not_found:
	ldr r0, =0xff200000 //led code
	mov r1, #0xfe0 // 0b1111100000
    str r1, [r0] // write led code
	ldr r0, =not_found_output
	b write_res // write string

write_res:
	mov r1, #0 // used for word counter
	ldr r2, =0xff201000 // jtag uart address
	write_loop:
		ldrb r3, [r0, r1] // load char
		str r3, [r2] // write char
		cmp r3, #0   // have we reach termination
		beq exit    // if so exit
		add r1, #1  // if not increment counter
		b write_loop // repeat


exit:
	// Branch here for exit
	b exit


.section .data
.align
	// This is the input you are supposed to check for a palindrome
	// You can modify the string during development, however you
	// are not allowed to change the label 'input'!
	input: .asciz "level"
	found_output: .asciz "Palindrome detected"
	not_found_output: .asciz "Not a palindrome"
	// input: .asciz "8448"
    // input: .asciz "KayAk"
    // input: .asciz "step on no pets"
    // input: .asciz "Never odd or even"


.end
