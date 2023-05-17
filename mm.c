/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "5조",
    /* First member's full name */
    "심우근",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "강영훈",
    "bovik@cs.cmu.edu",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int list;

    for (list = 0; list < LISTLIMIT; list++)
    {
        segregated_free_lists[list] = NULL;
    }

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2 * WSIZE);
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */

    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    // 요청받은 크기를 2워드 배수(8byte)로 반올림. 그리고 힙 공간 요청
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    insert_node(bp, size); // 가용 리스트에 새로 할당받은 영역 추가

    return coalesce(bp); // 가용 블록 합치기
}

static void insert_node(void *ptr, size_t size)
{
    int idx = get_list_index(size);
    void *search_ptr = segregated_free_lists[idx];
    void *insert_ptr = NULL;

    while (search_ptr != NULL && size > GET_SIZE(HDRP(search_ptr)))
    {
        insert_ptr = search_ptr;
        search_ptr = NEXT(search_ptr);
    }

    SET_PTR(PRED_P(ptr), search_ptr);
    SET_PTR(SUCC_P(ptr), insert_ptr);

    if (search_ptr != NULL)
    {
        SET_PTR(SUCC_P(search_ptr), ptr);
    }

    if (insert_ptr != NULL)
    {
        SET_PTR(PRED_P(insert_ptr), ptr);
    }

    else
    {
        segregated_free_lists[idx] = ptr;
    }
}

static void delete_node(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    int idx = get_list_index(size);

    if (NEXT(ptr) != NULL)
    {
        SET_PTR(SUCC_P(NEXT(ptr)), PREV(ptr));
    }

    if (PREV(ptr) != NULL)
    {
        SET_PTR(PRED_P(PREV(ptr)), NEXT(ptr));
    }
    else
    {
        segregated_free_lists[idx] = NEXT(ptr);
    }

    return;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize; // 들어갈 자리가 없을때 늘려야 하는 힙의 용량

    char *bp;

    /* Ignore spurious*/
    if (size == 0)
        return NULL;
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {                               // 뒷 블록이 가용 블록인 경우
        delete_node(bp);            // bp 블록 삭제
        delete_node(NEXT_BLKP(bp)); // bp의 뒷 블록 삭제

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    { // 앞 블록이 가용 블록인 경우
        delete_node(bp);
        delete_node(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    { // 앞 뒷 블록이 모두 가용 블록인 경우
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    insert_node(bp, size); // bp가 가용 블록의 위치이므로 가용 블록 추가
    return bp;
}

static int get_list_index(size_t size)
{
    int index = 0;
    while (size > 1)
    {
        size >>= 1;
        index++;
    }
    return index < LISTLIMIT ? index : LISTLIMIT - 1;
}

static void *find_fit(size_t asize)
{
    for (int idx = get_list_index(asize); idx < LISTLIMIT; idx++)
    {
        for (char *bp = segregated_free_lists[idx]; bp != NULL; bp = NEXT(bp))
        {
            if (asize <= GET_SIZE(HDRP(bp)))
            {
                return bp;
            }
        }
    }

    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    delete_node(bp);

    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        insert_node(bp, (csize - asize));
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    insert_node(bp, size);

    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    /* size가 0이면 블록을 해제하고 NULL을 반환합니다. */
    if (size == 0)
    {
        mm_free(bp);
        return NULL;
    }

    /* size가 0보다 크거나 같으면 진행합니다. */
    if (size >= 0)
    {

        /* 블록의 이전 크기를 가져옵니다. */
        size_t oldsize = GET_SIZE(HDRP(bp));

        /* 헤더와 바닥글을 포함하여 블록의 새 크기를 계산합니다. */
        size_t newsize = size + (2 * WSIZE);

        /* 새 크기가 이전 크기보다 작거나 같은 경우 블록을 반환합니다. */
        if (newsize <= oldsize)
        {
            return bp;
        }

        /* 다음 블록이 할당되어 있는지 확인합니다. 그렇다면 두 블록을 병합하고 새 블록을 반환합니다. */
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t csize;
        if (!next_alloc && ((csize = oldsize + GET_SIZE(HDRP(NEXT_BLKP(bp))))) >= newsize)
        {
            delete_node(NEXT_BLKP(bp));
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
            return bp;
        }

        /* 그렇지 않으면 요청한 크기의 새 블록을 할당하고 이전 블록의 데이터를 새 블록으로 복사하고 이전 블록을 해제하고 새 블록을 반환합니다. */
        else
        {
            void *new_ptr = mm_malloc(newsize);
            place(new_ptr, newsize);
            memcpy(new_ptr, bp, MIN(newsize, oldsize));
            mm_free(bp);
            return new_ptr;
        }
    }

    /* 크기가 음수이면 NULL을 반환합니다. */
    return NULL;
}
