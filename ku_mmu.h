#define PAGE_SIZE 4
#define PTE_MASK 0xFE
#define PFN_MASK 0xFC
#pragma pack(1)

typedef struct linked_list;
typedef struct ku_pte;
typedef struct PCB;
typedef struct node;
typedef struct space;
void insertPCB(struct PCB *input);
struct PCB *searchPCB(char searching_pid);
int swap_out();
int swap_in(struct ku_pte *accesed_pte);
int map_on_pmem(struct ku_pte *accesed_pte);

typedef struct ku_pte
{
    unsigned int present_bit : 1;
    unsigned int unused_bit : 1;
    unsigned int PFN : 6;
} ku_pte;

typedef struct PCB
{
    char pid;
    ku_pte *PTBR;
    struct PCB *prev;
    struct PCB *next;
} PCB;

// free list + alloc list node 개수의 합은 PFN 개수와 동일
typedef struct node
{
    // char pid;
    ku_pte *rmap;
    struct node *prev;
    struct node *next;
} node;


typedef struct linked_list
{
    void *head; /* recent */
    void *tail; /* old */
} linked_list;

typedef struct space
{
    void *alloc_area;
    struct linked_list free_list;
    struct linked_list alloc_list;
} space;

space ku_mmu_swap, ku_mmu_pmem;

linked_list PCB_list;
linked_list *PCB_list_ptr = &PCB_list;

PCB *tempPCB;

/**
 * @brief Insert new process's PCB to PCB list.
 *
 * @param input New Process's PCB wants to insert.
 */
void insertPCB(PCB *input)
{
    if (PCB_list_ptr->head == NULL)
        PCB_list_ptr->head = input;
    else
    {
        ((PCB *)(PCB_list_ptr->head))->next = input;
        input->prev = PCB_list_ptr->head;
    }
    PCB_list_ptr->head = input;
}

/**
 * @brief Search given process's PCB in PCB list.
 *
 * @param searching_pid Process's ID wants to search.
 * @return PCB* Searching process's PCB.
 */
PCB *searchPCB(char searching_pid)
{
    tempPCB = PCB_list_ptr->head;
    while (tempPCB != NULL)
    {
        if (tempPCB->pid == searching_pid)
            return tempPCB;
        else
            tempPCB = tempPCB->prev;
    }
    return NULL;
}

/**
 * @brief Swap out pte to swap space by FIFO.
 *
 * @return 0 on success, -1 on error.
 */
int swap_out()
{
    /* pmem_index에 매핑되어있는 pte를 swap out시킴 */
    unsigned int swap_dest;
    if (!swap_dest)
        return -1; // swap_dest의 index가 0일 경우 == swap이 꽉 차있을 경우
    /* swap space 값 변경 */

    /* swap out된 pte 값 변경 */
    ku_pte *swap_out_pte;
    if (!swap_out_pte)
        return -1;
    swap_out_pte->PFN = (swap_dest & PTE_MASK) >> 1;
    swap_out_pte->unused_bit = swap_dest & 0x1;
    swap_out_pte->present_bit = 0;

    return 0;
}

/**
 * @brief Swap in accessed pte to physical memory.
 * @param accesed_pte PTE wants to swap in to pysical memory.
 * @return 0 on success, -1 on error.
 */
int swap_in(ku_pte *accesed_pte)
{
    unsigned int take_back = ((accesed_pte->PFN) << 1) + accesed_pte->unused_bit;
    node *taking_node;
    taking_node->rmap = NULL; // free 된 swap을 free list에 insert
    if (map_on_pmem(accesed_pte))
        return -1;
    return 0;
}

/**
 * @brief Map accessed PTE to pysical memory.
 * @param accesed_pte PTE wants to map.
 * @return 0 on success, -1 on error.
 */
int map_on_pmem(ku_pte *accesed_pte)
{
    if (1)
        if (swap_out())
            return -1;
    accesed_pte->PFN; // accessed pte에게 PFN 할당
    accesed_pte->present_bit = 1;
    accesed_pte->unused_bit = 0;

    /* 할당된 pte reverse mapping */
    return 0;
}

/**
 * @brief Initalize memory space to use in KU_MMU.
 *
 * @param input Space name.
 * @param space_size Demanding space size.
 */
void init_space(space *input, unsigned int space_size)
{
    input->alloc_area = (char *)malloc(space_size);
    unsigned int num_of_page_in_input = space_size / PAGE_SIZE;
    for (int i = 1; i < num_of_page_in_input - 1; i++)
    {
        // input -> free_and_alloc[i].pid = 0;
    }
    // input -> free_and_alloc[num_of_page_in_input].pid = 0;
}

// 메모리와 스왑 공간 동적할당
void *ku_mmu_init(unsigned int pmem_size, unsigned int swap_size)
{
    init_space(&ku_mmu_pmem, pmem_size);
    init_space(&ku_mmu_swap, swap_size);
    PCB_list_ptr->head = NULL;
    if (ku_mmu_pmem.alloc_area == NULL)
        return 0;
    else
        return ku_mmu_pmem.alloc_area;
}

// 새로운 프로세스 생성 but 실제로 fork()를 통해 생성하는 것이 아닌 새로운 PCB만 생성
// PDBR(CR3) 새로운 프로세스를 위해 갱신
// 전에 생성된 프로세스라면 해당 프로세스의 PDBR 반환
// 프로세스당 256B Page Table 생성
// PCB를 PCB list에 insert
int ku_run_proc(char fpid, void **ku_cr3)
{
    tempPCB = searchPCB(fpid);
    if (!tempPCB)
    { // PCBlist에 fpid에 해당하는 PCB가 없을 경우
        tempPCB = (PCB *)malloc(sizeof(PCB));
        tempPCB->PTBR = (ku_pte *)calloc(64, sizeof(ku_pte));
        tempPCB->pid = fpid;
        insertPCB(tempPCB);
    }
    *ku_cr3 = tempPCB->PTBR;
    return 0;
}

// PTE did not mapped
// swaped out?? or map new PTE?? Both case needs to consider free space in swap and pmem
// 해당 함수가 호출되면 이후 ku_trav()가 실행될 때 address translation 성공 필요
// ! swap 계열의 함수 작동 실패시 -1 반환 후 ku_page_fault 종료해야함
int ku_page_fault(char pid, char va)
{
    ku_pte *page_table = tempPCB->PTBR; // PCB를 통해
    char pte_offset = (va & PFN_MASK) >> 2;
    ku_pte *target_pte = page_table + pte_offset; // 여기서 3시간 날림 대체 char *랑 ku_pte * 크기가 왜 다를까?
    if (target_pte->PFN == 0x0 && target_pte->unused_bit == 0x0)
    {
        if (map_on_pmem(target_pte))
            return -1; // TODO pbit이 아닌 ubit 값이 갱신됨
    }
    else
    {
        if (swap_in(target_pte))
            return -1;
    }
    return 0;
}