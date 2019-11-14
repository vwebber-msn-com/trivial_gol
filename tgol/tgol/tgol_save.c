// tgol.c : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////////////
// PROGRAM MACROS
////////////////////////////////////////////////////////////////////////////////////////

// Macros Controlling Execution
enum board_sz { N = 8 };
#define TURN(a) (a=a^0b00000001)
#define PK_TURN(a) (a^0b00000001)
#define BRD_MAX N*N
#define PROG_ITERS 3

// Board Views
typedef struct {
	unsigned char row : N;
} board_row_bit_t;

// Basic Addressable Unit (BAU) structure which must be 
// changed for different hardware architectures
// which have a different BAU
typedef struct {
	unsigned char nib0 : 4, nib1 : 4;
} board_bau_t;

////////////////////////////////////////////////////////////////////////////////////////
// GOL OBJECT MEMBERS
////////////////////////////////////////////////////////////////////////////////////////

typedef union {
	board_row_bit_t			board_bit[N];
	unsigned __int64		board_raw;
	unsigned char 			board_type_a[N];
	board_bau_t				board_bau_a[N];
} board_t;

void set_bit(int cnt);
void reset_bit(int cnt);

board_t BLINKER  = { 0,0,0,0x10,0x10,0x10,0,0 };
board_t TOAD     = { 0,0,0,0x38,0x1C,0,0,0 };
board_t BEACON   = { 0,0x60,0x60,0x18,0x18,0,0,0 };

int lookup_bit_count[0x10] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};

typedef struct _table_t
{
	board_t			board_a[2];
	unsigned __int64	b_mask;
	unsigned char	b_i;
} table_t, *ptable_t;

table_t table = { {{.board_raw = 0},{.board_raw = 0}}, 0b01, 0};

////////////////////////////////////////////////////////////////////////////////////////
// GENERATION MASK OBJECT
////////////////////////////////////////////////////////////////////////////////////////

#define GEN_MASK_C0   (0x83)
#define GEN_MASK_C0T  (0x82)
#define GEN_MASK_C1   (0x07)
#define GEN_MASK_C1T  (0x05)
#define GEN_MASK_C7   (0xC1)
#define GEN_MASK_C7T  (0x41)

board_t GEN_MASK;
//board_t GEN_MASK = { 0x07,0x07,0x07,0,0,0,0,0 }; // First 3 rows left shifted
//board_t GEN_MASK = { 0xE0,0xE0,0xE0,0,0,0,0,0 }; // First 3 rows right shifted
//board_t GEN_MASK = { .board_bau_a={{nib0=0,nib1=0},{ nib0 = 0,nib1 = 0 },{ nib0 = 0,nib1 = 0 },{ nib0 = 0,nib1 = 0 },{ nib0 = 0,nib1 = 0 },{ nib0 = 0,nib1 = 0 },{ nib0 = 0,nib1 = 0 },{ nib0 = 0,nib1 = 0 }} };//                        0x14, 0x14, 0x14, 0, 0, 0, 0, 0 };

#define GEN_MASK_RESET {GEN_MASK.board_raw = 0; GEN_MASK.board_type_a[7]=GEN_MASK_C0;GEN_MASK.board_type_a[0]=GEN_MASK_C0T;GEN_MASK.board_type_a[1]=GEN_MASK_C0;}

////////////////////////////////////////////////////////////////////////////////////////
// GOL OBJECT FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////

void rotate_bit_mask(void)
{
	table.b_mask = (table.b_mask == 0x8000000000000000 ? 1 : table.b_mask << 1);
}
/*
void rotate_gen_mask(board_t *bp, int i)
{
	int row_base = ((N + ((i / N) - 1))%N);

	switch (i%N)
	{
	case 0:
		bp->board_raw = 0;
		for (int j = 0; j < 3; j++)
		{
			bp->board_type_a[row_base] = GEN_MASK_C0;
			row_base = (++row_base) % N;
		}
		break;

	case 1:
		bp->board_raw = 0;
		for (int j = 0; j < 3; j++)
		{
			bp->board_type_a[row_base] = GEN_MASK_C1;
			row_base = (++row_base) % N;
		}
		break;

	case 7:
		bp->board_raw = 0;
		for (int j = 0; j < 3; j++)
		{
			bp->board_type_a[row_base] = GEN_MASK_C7;
			row_base = (++row_base) % N;
		}
		break;

	default:
		bp->board_raw = bp->board_raw << 1;
		break;

	}

}
*/
void rotate_gen_mask_obj(int i)
{
	int row_base = ((N + ((i / N) - 1)) % N);

	switch (i%N)
	{
	case 0:
		GEN_MASK.board_raw = 0;
		for (int j = 0; j < 3; j++)
		{
			if (j!=1) GEN_MASK.board_type_a[row_base] = GEN_MASK_C0;
			else GEN_MASK.board_type_a[row_base] = GEN_MASK_C0T;
			row_base = (++row_base) % N;
		}
		break;

	case 1:
		GEN_MASK.board_raw = 0;
		for (int j = 0; j < 3; j++)
		{
			if (j != 1) GEN_MASK.board_type_a[row_base] = GEN_MASK_C1;
			else GEN_MASK.board_type_a[row_base] = GEN_MASK_C1T;
			row_base = (++row_base) % N;
		}
		break;

	case 7:
		GEN_MASK.board_raw = 0;
		for (int j = 0; j < 3; j++)
		{
			if (j != 1) GEN_MASK.board_type_a[row_base] = GEN_MASK_C7;
			else GEN_MASK.board_type_a[row_base] = GEN_MASK_C7T;
			row_base = (++row_base) % N;
		}
		break;

	default:
		GEN_MASK.board_raw = GEN_MASK.board_raw << 1;
		break;

	}

}

int init_board(const char *pattern)
{
	int rc = EXIT_SUCCESS;
	                                                                       // 0x8000000000000000;
	if (_stricmp(pattern,"RANDOM") == 0) table.board_a[table.b_i].board_raw = 0x8034099556782305;
	else if (_stricmp(pattern, "TOAD") == 0) table.board_a[table.b_i].board_raw = TOAD.board_raw;
	else if (_stricmp(pattern, "BLINKER") == 0) table.board_a[table.b_i].board_raw = BLINKER.board_raw;
	else if (_stricmp(pattern, "BEACON") == 0) table.board_a[table.b_i].board_raw = BEACON.board_raw;
	else rc = EXIT_FAILURE;

	if (!(rc)) printf("\n%s", pattern);

	return rc;

}

void print_board(void)
{
	unsigned __int64 mask = 1;

	printf("\n");

	for (unsigned char r = 0; r < N; ++r)
	{
		for (unsigned char c = 0; c < N; ++c)
		{
			(table.board_a[table.b_i].board_raw & mask) ? printf("X ") : printf(". ");
			mask = mask << 1;
		}
		printf("\n");
	}

}

/*
void print_board_test()
{
	unsigned __int64 mask = 1;

	printf("\n");

	for (unsigned char r = 0; r < N; ++r)
	{
		for (unsigned char c = 0; c < N; ++c)
		{
			(GEN_MASK.board_raw & mask) ? printf("X ") : printf(". ");
			mask = mask << 1;
		}
		printf("\n");
	}
}

void print_board_test_ptr(board_t *bp)
{
	unsigned __int64 mask = 1;

	printf("\n");

	for (unsigned char r = 0; r < N; ++r)
	{
		for (unsigned char c = 0; c < N; ++c)
		{
			(bp->board_raw & mask) ? printf("X ") : printf(". ");
			mask = mask << 1;
		}
		printf("\n");
	}
}



void print_board_test1()
{
	int j = 0;

	while (j < (N*N))
	{
		print_board_test_ptr(&GEN_MASK);
		rotate_gen_mask(&GEN_MASK, ++j);
	}

}
*/

void set_bit(int cnt)
{
	switch (cnt)
	{
	case 3:
		table.board_a[PK_TURN(table.b_i)].board_raw = (table.board_a[PK_TURN(table.b_i)].board_raw)^table.b_mask;
		break;
	default:
		break;
	};

}

void reset_bit(int cnt)
{
	switch (cnt)
	{
	case 3:
	case 2:
		break;
	default:
		table.board_a[PK_TURN(table.b_i)].board_raw = table.board_a[PK_TURN(table.b_i)].board_raw & ~(table.b_mask);
		break;
	};
}

unsigned char count_set_bits_row(board_t set, unsigned char row)
{
	int c_total = 0;

	c_total += lookup_bit_count[set.board_bau_a[row].nib0];
	c_total += lookup_bit_count[set.board_bau_a[row].nib1];

	return c_total;
}

unsigned char count_set_bits(board_t set, unsigned char i)
{
	
	int c_total = 0;
	int row_base = ((N + ((i / N) - 1)) % N);

	for (unsigned char j = 0; j < 3; j++)
	{
		c_total += count_set_bits_row(set, row_base);
		row_base = (++row_base)%N;
	}

	return c_total;
}

void generation(void)
{
	unsigned char i = 0;
	unsigned char c_total = 0;
	board_t set_bits;

	table.board_a[PK_TURN(table.b_i)].board_raw = table.board_a[table.b_i].board_raw;

	GEN_MASK_RESET;
	
	while (i < (N*N))
	{
		set_bits.board_raw = 0;
		set_bits.board_raw = table.board_a[table.b_i].board_raw & GEN_MASK.board_raw;
		
		c_total = count_set_bits(set_bits, i);

		(table.board_a[table.b_i].board_raw & table.b_mask) ? reset_bit(c_total) : set_bit(c_total);
		rotate_bit_mask();
		rotate_gen_mask_obj(++i);
	}
	TURN(table.b_i);
}

int main(int argc, const char* argv[])
{
	int rc = EXIT_FAILURE;

	printf("%d \n", argc);

	do
	{
		if (argc == 1)
		{
			if ((rc = init_board((const char *)"RANDOM")) == EXIT_FAILURE) break;
		}
		else if (argc == 2)
		{
			if ((rc = init_board(argv[1])) == EXIT_FAILURE) break;
		}
		else break;

		for (int j = 0; j <= PROG_ITERS; j++)
		{
			print_board();
			generation();
		}
		
	} while (0);

	return rc;
}

