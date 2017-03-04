/*
"Maszyna MIK"
autor: Konstanty Aleksander Cieslinski <kscieslinski@gmail.com>
wersja: 1.0
data: 12.12.2016

Uwagi: 
Mam nadzieje, ze nie bedzie zbyt wielu, 
wynikajacych z nieznajomosci zargonu informatycznego zmylek w postacii nazw zmiennych 
przez nikogo w branzy nie uzywanych.( np. pomocnicza = dummy ). 

Program description:
It's a MIK engine code interpreter. Program loads the primary state of engine from input. 
Anything after the base record is treated as data for the program which is being interpreted. 
The final result is printed on the stdin, or in case of core_dump, on the stderr.      
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_VALUE 256
#define REGISTER_SIZE 16
#define MEMORY_SIZE 256
#define INSTRUCTIONS_SIZE 256
#define BASE_AP 16
#define BASE_QX 8
#define BASE_YZ 2
#define BASE_09 10
#define END_OF_SECTION_CHAR 37
#define SPACE 32
#define HORIZONTAL_TAB 9
#define LINE_FEED 10
#define VERTICAL_TAB 11
#define FORM_FEED 12
#define CONSIDER_ONLY_FIRST_FOUR_BITES 15
#define SKIP_FOUR_BITES 4
#define SKIP_EIGHT_BITES 8
#define SKIP_TWELVE_BITES 12
#define END_OF_FILE EOF
#define END_OF_ARRAY '\0'

struct engine{
	int Register[REGISTER_SIZE];
	int Memory[MEMORY_SIZE];
	int Instructions[INSTRUCTIONS_SIZE];
	int program_counter;
};

/* Function scales the result of operation modulo maximum value */
int scale( int current_result ){
	if( ( current_result % MAX_VALUE ) < 0 ) return current_result + MAX_VALUE;
	else return current_result % MAX_VALUE;
}

/* Function assigns the next index after given. For current < modulo returns index current+1, else returns index 0. */
int determinate_next( int current, int modulo ){
	return ( current + 1 ) % modulo;
}

/* Procedure called with operation code 0. Divides two integer numbers. The result is saved in register with index a, 
and the rest in register with next index */
void divide( struct engine *mik, int a, int b, int c ){
	int next, dummy_c, dummy_b;
	dummy_c = mik->Register[c];
	if( dummy_c != 0 ){
	    dummy_b = mik->Register[b];
	    mik->Register[a] = scale( dummy_b / dummy_c );
	    next = determinate_next( a, REGISTER_SIZE );
	    mik->Register[next] = scale( dummy_b % dummy_c );
	}	
}

/* Procedure called with operation code 0. */
void push( struct engine *mik, int a, int b ){
	mik->Register[a] = scale( mik->Register[a] - 1 );
	mik->Memory[mik->Register[a]] = mik->Register[b];
}

/* Procedure called with operation code 0. */
void halt( struct engine *mik, int a ){
	exit( mik->Register[a] );
}

/* Procedure called with operation code 1. */
void recall( struct engine *mik, int a, int b, int c ){
	int dummy_c = mik->program_counter;
	mik->program_counter = mik->Memory[mik->Register[a]];
	mik->Register[a] = scale( mik->Register[a] + mik->Register[c] + 1 );
	mik->Register[b] = dummy_c;
}

/* Procedure called with operation code 1. */
void pop( struct engine *mik, int a, int b ){
	mik->Register[b] = mik->Memory[mik->Register[a]];
	mik->Register[a] = scale( mik->Register[a] + 1 );
}

/* Procedure called with operation code 1. */
void return_from_subroutine( struct engine *mik, int a, int c ){
	int dummy_c = mik->program_counter;
	mik->program_counter = mik->Register[c];
	mik->Register[a] = dummy_c;
}

/* Procedure called with operation code 2. */
void compare( struct engine *mik, int a, int b, int c ){
	if( mik->Register[b] < mik->Register[c] ) mik->Register[a] = 1;
	else mik->Register[a] = 0;
}

/* Procedure called with operation code 2. */
void shift_left( struct engine *mik, int a, int b ){
	mik->Register[a] = scale( mik->Register[b] << 1 );
}

/* Procedure called with operation code 3. */
void substract( struct engine *mik, int a, int b, int c ){
	mik->Register[a] = scale( mik->Register[b] - mik->Register[c] );
}

/* Procedure called with operation code 3. */
void shift_right( struct engine *mik, int a, int b ){
	mik->Register[a] = scale( mik->Register[b] >> 1 );
}

/* Procedure called with operation code 4. */
void load_indexed( struct engine *mik, int a, int b, int c){
	mik->Register[a] = scale( mik->Memory[mik->Register[b] + mik->Register[c]] );
}

/* Procedure called with operation code 4. */
void add( struct engine *mik, int a, int b, int c ){
	mik->Register[a] = scale( mik->Register[b] + mik->Register[c] );
}

/* Procedure called with operation code 5. */
void store_indexed( struct engine *mik, int a, int b, int c ){
	int dummy;
	dummy = scale( mik->Register[b] + mik->Register[c] );
	mik->Memory[dummy] = mik->Register[a];
}

/* Procedure called with operation code 5. */
void bitwise_or( struct engine *mik, int a, int b, int c ){
	mik->Register[a] = scale( mik->Register[b] | mik->Register[c] ); 
}

/* Procedure called with operation code 6. */
void multiply( struct engine *mik, int a, int b, int c ){
	int next = determinate_next( a, REGISTER_SIZE );
	mik->Register[next] = ( mik->Register[b] * mik->Register[c] ) / MAX_VALUE;
	mik->Register[a] = ( mik->Register[b] * mik->Register[c] ) % MAX_VALUE;
}

/* Procedure called with operation code 6. */
void bitwise_and( struct engine *mik, int a, int b, int c ){
	mik->Register[a] = scale( mik->Register[b] & mik->Register[c] );
}

/* Procedure called with operation code 7. */
void call_indexed( struct engine *mik, int a, int b ){
	int dummy_c = mik->program_counter;
	mik->program_counter = scale( mik->Memory[mik->Register[a] + mik->Register[b]] );
	mik->Register[a] = dummy_c;	
}

/* Procedure called with operation code 7. */
void bitwise_exclusive_or( struct engine *mik, int a, int b, int c ){
	mik->Register[a] = scale( mik->Register[b] ^ mik->Register[c] );
}

/* Procedure called with operation code 8. */
void jump_if_zero( struct engine *mik, int a, int b, int c ){
	if(mik->Register[a] == 0){
    	mik->program_counter = scale( ( b * BASE_AP ) + c );
	}
}

/* Procedure called with operation code 9. */
void jump_if_not_zero( struct engine *mik, int a, int b, int c ){
	if ( mik->Register[a] != 0 ){
		mik->program_counter = scale( ( b * BASE_AP ) + c );
	}
}

/* Procedure called with operation code 10. */
void call_subroutine( struct engine *mik, int a, int b, int c ){
	mik->Register[a] = mik->program_counter;
	mik->program_counter = scale( ( b * BASE_AP ) + c );
}

/* Procedure called with operation code 11. */
void call( struct engine *mik, int a, int b, int c ){
	mik->Register[a] = scale( mik->Register[a] - 1 );
	mik->Memory[mik->Register[a]] = mik->program_counter;
	mik->program_counter = scale( ( b * BASE_AP ) + c );
}

/* Procedure called with operation code 12 */
void load_register( struct engine *mik, int a, int b, int c ){
	int dummy;
	dummy = scale( b * BASE_AP + c );
	mik->Register[a] = mik->Memory[dummy];
}

/* Procedure called with operation code 13 */
void store_register( struct engine *mik, int a, int b, int c ){
	int dummy;
	dummy = scale( b * BASE_AP + c );
	mik->Memory[dummy] = mik->Register[a];
}

/* Procedure called with operation code 14 */
void load_constant( struct engine *mik, int a, int b, int c ){
	mik->Register[a] = scale( ( b * BASE_AP ) + c );
}

/* Function returns first index of the array with non zero content. */
int state_from_where_not_only_zeros( int Array[], int size, int primary){
	while( Array[size] == 0 && size >= primary) size--;
	return size;
}

/* Procedure prints the array, from primary index till the last non zero one. Ends printing with '%' sign. */
void print_section( int Array[], int size, int primary ){
	int i = state_from_where_not_only_zeros( Array, size - 1, primary );
	while( primary <= i ){
		fprintf( stderr, "\n%d ", Array[primary]);
		primary++;
	}
	fprintf( stderr, "\n%%\n" );
}

/* Procedure printfs complete current state of engine to stderr. */
void core_dump( struct engine *mik, int a ){
	print_section( mik->Register, REGISTER_SIZE, 0 );
	print_section( mik->Memory, MEMORY_SIZE, 0 );
	print_section( mik->Instructions, mik->Register[a], 0 );
	print_section( mik->Instructions, INSTRUCTIONS_SIZE, mik->Register[a] );
}

/* Procedure prints string which is code in engine's memory, starting from the given index. 
It is an ancillary procedure for system call. */
void print_string( int Array[], int primary, int size ){
	while( Array[primary] != END_OF_ARRAY && primary < size ){
		printf( "%c", Array[primary] );
		primary++;
	}
}

/* Procedure called with operation code 15. */
void system_call( struct engine *mik, int a, int b, int c ){
	int dummy_a, next;
	next = determinate_next( a, REGISTER_SIZE );
	switch( ( b * BASE_AP ) + c ){
		case 0:
	    	core_dump( mik, a );
	    	break;
		case 1:
	    	if( scanf("%d", &dummy_a) != 1 ) mik->Register[next] = 0;
	    	else {
	        	mik->Register[next] = 1;
	        	mik->Register[a] = scale( dummy_a );
	    	}
	    	break;
		case 2:
	    	printf("%d", mik->Register[a]);
	    	break;
		case 3:
	    	dummy_a = getchar();
	    	if( dummy_a == END_OF_FILE ) mik->Register[next] = 0;
			else {
	        	mik->Register[next] = 1;
	        	mik->Register[a] = scale( dummy_a );
	    	}
	    	break;
		case 4:
	    	putchar( mik->Register[a] );
	    	break;
		case 5:
	    	print_string( mik->Memory, mik->Register[a], MEMORY_SIZE );
	    	break;
		default:
	    	break;
	}
}

/* Main procedure. Determinate operation_code, a, b, c, base on the instruction pointed by instruction_pointer. 
According to the result perform particular function. */ 
void start_program( struct engine *mik ){
	int operaction_code, a, b, c, instruction;
	while(1){
		instruction = mik->Instructions[mik->program_counter];
		operaction_code = instruction >> SKIP_TWELVE_BITES;
		a = ( instruction >> SKIP_EIGHT_BITES ) & CONSIDER_ONLY_FIRST_FOUR_BITES;
		b = ( instruction >> SKIP_FOUR_BITES ) & CONSIDER_ONLY_FIRST_FOUR_BITES;
		c = instruction & CONSIDER_ONLY_FIRST_FOUR_BITES;
		mik->program_counter = determinate_next( mik->program_counter, INSTRUCTIONS_SIZE );
		switch( operaction_code ){
			case 0:
				if( b !=c ) divide( mik, a, b, c );
				if ( a != b && b == c ) push( mik , a, b );
				if ( a == b && b == c ) halt( mik, a );
				break;
			case 1:
				if( a != b && a != c ) recall( mik, a, b, c ); 
				if ( a != b && a == c ) pop( mik, a, b );
				if ( a == b ) return_from_subroutine( mik, a, c ); 
				break;
			case 2:
				if( b != c ) compare( mik, a, b, c );
				if( b == c ) shift_left( mik, a, b );
				break;
			case 3:
				if( b != c ) substract( mik, a, b, c );
				if( b == c ) shift_right( mik, a, b );
				break;
			case 4:
				if( b <= c ) load_indexed( mik, a, b, c );
				if ( b > c ) add( mik, a, b, c );
				break;
			case 5:
				if( b <= c ) store_indexed( mik, a, b, c );
				if( b > c ) bitwise_or( mik, a, b, c );
				break;
			case 6:
				if( b <= c ) multiply( mik, a, b, c );
				if ( b > c ) bitwise_and( mik, a, b, c );
				break;
			case 7:	
				if( b <= c ) call_indexed( mik, a, b );
				if( b > c) bitwise_exclusive_or( mik, a, b, c );
				break;
			case 8:
				jump_if_zero( mik, a, b, c );
				break;
			case 9:
				jump_if_not_zero( mik, a, b, c );
				break;
			case 10:
				call_subroutine( mik, a, b, c );
				break;
			case 11:
				call( mik, a, b, c );
				break;
			case 12:
				load_register( mik, a, b, c );
				break;
			case 13:
				store_register( mik, a, b, c );
				break;
			case 14:
				load_constant( mik, a, b, c );
				break;
			case 15:
				system_call( mik, a, b, c );
				break;
		}
	}
}

/* Procedure fills all the array cells with zeros. */
void reset_single_array( int Array[], int array_size ){
	int i;

	for( i = 0; i < array_size; i++) Array[i] = 0;
}

/* Procedure resets full engine, through ancillary procedure "reset_sinle_array" */  
void reset( struct engine *mik ){
	reset_single_array( mik->Register, REGISTER_SIZE );
	reset_single_array( mik->Memory, MEMORY_SIZE );
	reset_single_array( mik->Instructions, INSTRUCTIONS_SIZE );
}

/* Procedure checks if given in ASCII code, sign is a white char */
bool check_if_white_char( int sign ){
	if( ( sign == SPACE ) || ( sign == HORIZONTAL_TAB ) || ( sign == VERTICAL_TAB ) || ( sign == LINE_FEED ) || ( sign == FORM_FEED )) return 1;
	else return 0;
}

/* Function determinates the base of the given sign. Else occurs for ( sign >= 'Y' && sign <= 'Z' ) */ 
int determinate_base( int sign ){
	if( sign >= '0' && sign  <= '9' ) return BASE_09;
	if( sign >= 'A' && sign <= 'P' ) return BASE_AP;
	if( sign >= 'Q' && sign <= 'X' ) return BASE_QX;
	else return BASE_YZ;
}

/* Function change the given dexary digit into decimal notation. Else occurs for digit == 'Z'. */
int change_to_decimal( int digit ){
	if( digit >= '0' && digit  <= '9' ) return digit % '0';
	if( digit >= 'A' && digit <= 'P' ) return digit % 'A';
	if( digit >= 'Q' && digit <= 'X' ) return digit % 'Q';
	if( digit == 'Y' ) return 1;
	else return 0; 
}

/* Function encodes word given in dexary notation, and return it decimal form. */
int load_word( int buffor, int *end_section ){
	int sum = 0;
	sum = change_to_decimal( buffor );
	
	while( 1 ){
		buffor = getchar();
		if( buffor == END_OF_SECTION_CHAR ){
			*end_section = 1;
			return sum;
		}
		if( check_if_white_char( buffor ) == 1 ) return sum;
		else{
			sum *= determinate_base( buffor );
			sum += change_to_decimal( buffor );
		}
	}
}

/* Procedure loads given section into the corresponding array */
int load_section( int Array[], int word_counter ){
	int buffor;
	int end_section = 0;
	
	while( ( buffor = getchar() )!= END_OF_SECTION_CHAR ){
		if( check_if_white_char( buffor ) == 0){
			Array[word_counter] = load_word( buffor, &end_section );
			word_counter++;
		}	
		if( end_section == 1 ) break;
	}
	return word_counter;
}

/* Procedure loads the primary state of engine */
void load_primary_state( struct engine *mik ){
	load_section( mik->Register, 0 );
	load_section( mik->Memory, 0 );
	mik->program_counter = load_section( mik->Instructions, 0 );
	load_section( mik->Instructions, mik->program_counter);
}

int main(){
	struct engine mik;
	
	reset( &mik );
	load_primary_state( &mik );
	start_program( &mik );
	
	return 0;
}
