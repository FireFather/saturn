/*
  This code is adapted from R. De Man and Daniel Shaw's Cfish nnue probe code:
  https://github.com/dshawul/nnue-probe
*/

#pragma once

#ifndef __cplusplus
#ifndef _MSC_VER
#include <stdalign.h>
#endif
#endif

/**
* Calling convention
*/
#ifdef __cplusplus
#   define EXTERNC extern "C"
#else
#   define EXTERNC
#endif

#if defined (_WIN32)
#   define _CDECL __cdecl
#ifdef DLL_EXPORT
#   define DLLExport EXTERNC __declspec(dllexport)
#else
#   define DLL_EXPORT EXTERNC __declspec(dllimport)
#endif
#else
#   define _CDECL
#   define DLLExport EXTERNC
#endif

/**
* Internal piece representation
*     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
*     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12
*
* Make sure the pieces you pass to the library from your engine
* use this format.
*/
enum colors
{
	white_nnue, black_nnue
};
enum pieces
{
	blank = 0, wking, wqueen, wrook, wbishop, wknight, wpawn,
	bking, bqueen, brook, bbishop, bknight, bpawn
};

/**
* nnue data structure
*/

typedef struct dirty_piece
{
	int dirty_num;
	int pc[3];
	int from[3];
	int to[3];
} dirty_piece;

typedef struct accumulator
{
	alignas(64) int16_t accumulation[2][256];
	int computed_accumulation;
} Accumulator;

typedef struct nnue_data
{
	Accumulator accumulator;
	dirty_piece dirtyPiece;
} nnue_data;

/**
* position data structure passed to core subroutines
*  See nnue_evaluate for a description of parameters
*/
typedef struct Position
{
	int player;
	int* pieces;
	int* squares;
	nnue_data* nnue[3];
} Position;

int nnue_evaluate_pos(const Position* pos);

/**
* Load NNUE file
*/
void _CDECL nnue_init
(
	const char* eval_file             /** Path to NNUE file */
);

/**
* Evaluation subroutine suitable for chess engines.
* -------------------------------------------------
* Piece codes are
*     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
*     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12,
*
* Squares are
*     A1=0, B1=1 ... H8=63
*
* Input format:
*     piece[0] is white king, square[0] is its location
*     piece[1] is black king, square[1] is its location
*     ..
*     piece[x], square[x] can be in any order
*     ..
*     piece[n+1] is set to 0 to represent end of array
*
* Returns
*   Score relative to side to move in approximate centipawns
*/
int _CDECL nnue_evaluate
(
	int player,                       /** Side to move: white=0 black=1 */
	int* pieces,                      /** Array of pieces */
	int* squares                      /** Corresponding array of squares each piece stands on */
);

