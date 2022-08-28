.global _start

.section .text

_start:
	ldr r0, =input
	bl check_input // write length to r2
	bl to_lower
	bl check_palindrome // check palindrome in r0 of length r2
	// Here your execution starts
	b exit

possible_uppercase:
	cmp r3, #65 // if char is after A
	addge r3, #32 // 32 is diff between A and a
	bx lr



to_lower:
 // r0 array
 // r1 counter
 // r2 length
 	mov r1, #0
	push {lr} // needed because nested call
	to_lower_loop:
		ldrb r3, [r0, r1]
		cmp r3, #90 // if char is becore Z
		blle possible_uppercase
		strb r3, [r1, r0] // write character that might have bee updated
		add r1, #1 // increment counter
		cmp r1, r2 // have we reached length
		popeq {pc} // if so jump back
		b to_lower_loop // if not move on




check_input:
	// You could use this symbol to check for your input length
	// you can assume that your input string is at least 2 characters
	// long and ends with a null byte
	mov r2, #0 // using r2 to count length
	loop:
		add r2, #1 //  increment length
		ldrb r3, [r0, r2] // use r3 to store character
		cmp r3, #0   // is it null terminated
		bne loop      // we have not found last character
	bx lr // back to start

// checks if r0 + r1 and r0 + r2 fits our palindrome condition
check_reg:
	mov r3, #0 // used for result
	ldrb r4, [r0, r1] // first char
	ldrb r5, [r0, r2] // second char
	cmp r4, r5 // check that they are equal
	moveq r3, #1 // if equal res is 1
	bx lr

// iterates register skipping over any spaces encountered
iterate_reg:
	// we have to iterate at least once
	add r1, #1
	sub r2, #1
	inner_iteration:
		first_counter_loop:
			ldrb r4, [r0, r1]
			// if space
			cmp r4, #32
			// skip space
			addeq r1, #1
			// might be more spaces
			beq inner_iteration
		second_counter_loop:
			ldrb r4, [r0, r2]
			// if space
			cmp r4, #32
			// skip space
			subeq r2, #1
			// might be more spaces after this one
			beq inner_iteration
	bx lr


check_palindrome:
	sub r2, #1 // dont care about terminating character
	mov r1, #0 // this will be start counter
	paliloop:
		bl check_reg // check r0[r1] == r0[r2]
		cmp r3, #1 // used as equal condtion
		bne palindrome_not_found // not eq
		bl iterate_reg
		cmp r1, r2 // have the counters crossed?
		bge palindrome_found // we have checked entire word, know it's palindrome
		b paliloop // need to check more of word
	bx lr
	// Here you could check whether input is a palindrome or not

write_res:
	mov r1, #0 // used for word counter
	ldr r6, =0xff201000 // jtag uart address
	ldr r10, =0xFF201004
	mov r9, #0b1000000000
	str r9, [r10]

	write_loop:
		ldrb r3, [r0, r1] // load char
		str r3, [r6] // write char
		cmp r3, #0   // have we reach termination
		beq exit    // if so exit
		add r1, #1  // if not increment counter
		b write_loop // repeat


palindrome_found:
	// Switch on only the 5 rightmost LEDs
	// Write 'Palindrome detected' to UART
	ldr r6, =0xff200000 // led address
	mov r7, #0x1f // 0b0000011111
    str r7, [r6]  // write led code
	ldr r0, =found_output // set string param
	b write_res // write string


palindrome_not_found:
	ldr r6, =0xff200000 //led code
	mov r7, #0xfe0 // 0b1111100000
    str r7, [r6]
	ldr r0, =not_found_output
	b write_res // write string
	// Switch on only the 5 leftmost LEDs
	// Write 'Not a palindrome' to UART


exit:
	// Branch here for exit
	b exit


.section .data
.align
	// This is the input you are supposed to check for a palindrome
	// You can modify the string during development, however you
	// are not allowed to change the label 'input'!
	//input: .asciz "level"
	input: .asciz "a man a plan a canal panama"
	found_output: .asciz "Palindrome detected"
	not_found_output: .asciz "Not a palindrome"
	// input: .asciz "8448"
    // input: .asciz "KayAk"
    // input: .asciz "step on no pets"
    // input: .asciz "Never odd or even"


.end
