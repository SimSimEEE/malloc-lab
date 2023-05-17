#include <stdio.h>

/* Basic constants and macros */
#define WSIZE 4                /* Word and header/footer size (bytes) */
#define DSIZE 8                /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12)    /* Extend heap by this amount (bytes) */
#define INITCHUNKSIZE (1 << 6) // 64
#define LISTLIMIT 10

// calculate max value
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

// size와 할당 여부를 하나로 합친다
#define PACK(size, alloc) ((size) | (alloc))

// 포인터 p가 가리키는 주소의 값을 가져온다.
#define GET(p) (*(unsigned int *)(p))

// 포인터 p가 가리키는 곳에 val을 역참조로 갱신
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// 포인터 p가 가리키는 곳의 값에서 하위 3비트를 제거하여 블럭 사이즈를 반환(헤더+푸터+페이로드+패딩)
#define GET_SIZE(p) (GET(p) & ~0X7)
// 포인터 p가 가리키는 곳의 값에서 맨 아래 비트를 반환하여 할당상태 판별
#define GET_ALLOC(p) (GET(p) & 0X1)

// 블럭포인터를 통해 헤더 포인터,푸터 포인터 계산
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 블럭포인터 -> 블럭포인터 - WSIZE : 헤더위치 -> GET_SIZE으로 현재 블럭사이즈계산 -> 다음 블럭포인터 반환
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
// 블럭포인터 -> 블럭포인터 - DSIZE : 이전 블럭 푸터 -> GET_SIZE으로 이전 블럭사이즈계산 -> 이전 블럭포인터 반환
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

// 포인터 p가 가리키는 메모리에 포인터 ptr을 입력
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

// 가용 블럭 리스트에서 next 와 prev의 포인터를 반환
#define PRED_P(ptr) ((char *)(ptr))
#define SUCC_P(ptr) ((char *)(ptr) + WSIZE)

/* Puts pointers in the next and previous elements of free list (explicit list)*/
#define SET_PREDP(bp, ptr) (PRED_P(bp) = ptr)
#define SET_SUCCP(bp, ptr) (SUCC_P(bp) = ptr)

// segregated list 내에서 next와 prev의 포인터를 반환
#define NEXT(ptr) (*(char **)(ptr))
#define PREV(ptr) (*(char **)(SUCC_P(ptr)))

extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);
extern void *mm_realloc(void *ptr, size_t size);

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t newsize);

static void list_add(void *bp);
static void list_remove(void *bp);

static void insert_node(void *ptr, size_t size);
static void delete_node(void *ptr);

static int get_list_index(size_t size);

static char *heap_listp = NULL;
static char *free_blck_listp = NULL;

static void *segregated_free_lists[LISTLIMIT];

static char *before_bp = NULL;

/*
 * Students work in teams of one or two.  Teams enter their team name,
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct
{
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;
