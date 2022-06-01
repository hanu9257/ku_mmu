#define PAGE_SIZE 4
#define PTE_MASK 0xFE
#define PFN_MASK 0xFC
#pragma pack(1)

struct linked_list;
struct ku_pte;
struct PCB;
struct PCB_list;
struct node;
struct space;
void insertPCB(struct PCB *input);
struct PCB *searchPCB(char searching_pid);
int swap_out();
int swap_in(struct ku_pte *accesed_pte);
int map_on_pmem(struct ku_pte *accesed_pte);
struct node *get_node(struct linked_list *list);
void put_node(struct linked_list *list, struct node *input);

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
    unsigned int page_index;
    ku_pte *rmap;
    struct node *prev;
    struct node *next;
} node;

typedef struct linked_list
{
    struct node *head; /* recent */
    struct node *tail; /* old */
} linked_list;

typedef struct PCB_list
{
    struct PCB *head; /* recent */
} PCB_list;

typedef struct space
{
    void *alloc_area;
    struct linked_list *free_list;
    struct linked_list *alloc_list;
} space;

space ku_mmu_swap, ku_mmu_pmem;

PCB_list ku_mmu_PCB_list;

PCB *tempPCB;

/**
 * @brief Insert new process's PCB to PCB list.
 *
 * @param input New Process's PCB wants to insert.
 */
void insertPCB(PCB *input)
{
    if (ku_mmu_PCB_list.head == NULL)
        ku_mmu_PCB_list.head = input;
    else
    {
        input->prev = ku_mmu_PCB_list.head;
        ku_mmu_PCB_list.head = input;
    }
    ku_mmu_PCB_list.head = input;
}

/**
 * @brief Search given process's PCB in PCB list.
 *
 * @param searching_pid Process's ID wants to search.
 * @return PCB* Searching process's PCB.
 */
PCB *searchPCB(char searching_pid)
{
    tempPCB = ku_mmu_PCB_list.head;
    while (tempPCB != NULL)
    {
        if (tempPCB->pid == searching_pid)
            return tempPCB;
        else
            tempPCB = tempPCB->prev;
    }
    return NULL;
}

void reset_node(node *input)
{
    input->next = NULL;
    input->prev = NULL;
    input->rmap = NULL;
}

/**
 * @brief Swap out pte to swap space by FIFO.
 *
 * @return 0 on success, -1 on error.
 */
int swap_out()
{
    /* pmem_index에 매핑되어있는 pte를 swap out시킴 */
    node *swap_dest = get_node(ku_mmu_swap.free_list);
    if (!swap_dest)
        return -1; // swap_dest의 index가 0일 경우 == swap이 꽉 차있을 경우
    /* swap out된 pte 값 변경 */
    node *swap_src = get_node(ku_mmu_pmem.alloc_list);
    if(!swap_src)
        return -1;
    ku_pte *swap_out_pte = swap_src->rmap;
    if (!swap_out_pte)
        return -1;
    swap_dest->rmap = swap_out_pte;
    swap_out_pte->PFN = (swap_dest->page_index & PTE_MASK) >> 1;
    swap_out_pte->unused_bit = swap_dest->page_index & 0x1;
    swap_out_pte->present_bit = 0;
    reset_node(swap_src);
    put_node(ku_mmu_pmem.free_list, swap_src);
    put_node(ku_mmu_swap.alloc_list, swap_dest);
    return 0;
}

node *search_node(linked_list *list, unsigned int pte_index)
{
    node *temp = list->tail->next;
    while(temp->next)
    {
        if(temp->page_index == pte_index)
            return temp;
        else
            temp = temp->next;
    }
    return NULL;
}

/**
 * @brief Swap in accessed pte to physical memory.
 * @param accesed_pte PTE wants to swap in to pysical memory.
 * @return 0 on success, -1 on error.
 */
int swap_in(ku_pte *accesed_pte)
{
    unsigned int taking_pte_index = ((accesed_pte->PFN) << 1) + accesed_pte->unused_bit;
    node *src_node = search_node(ku_mmu_swap.alloc_list, taking_pte_index);
    if(src_node == NULL)
        return -1;
    node *prev_node = src_node->prev;
    node *next_node = src_node->next;
    prev_node->next = next_node;
    if(next_node)
        next_node->prev = prev_node;
    accesed_pte->PFN = src_node->rmap->PFN;
    accesed_pte->present_bit = 1;
    accesed_pte->unused_bit = 0;
    free(src_node);
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
    node *mapping_dest;
    mapping_dest = get_node(ku_mmu_pmem.free_list);
    if (mapping_dest == NULL) 
    {
        if (swap_out() == -1)
            return -1;
        mapping_dest = get_node(ku_mmu_pmem.free_list);
    }
    
    accesed_pte->PFN = mapping_dest->page_index; // accessed pte에게 PFN 할당
    accesed_pte->present_bit = 1;
    accesed_pte->unused_bit = 0;

    /* 할당된 pte reverse mapping */
    mapping_dest->rmap = accesed_pte;
    put_node(ku_mmu_pmem.alloc_list, mapping_dest);
    return 0;
}

/**
 * @brief Initalize memory space to use in KU_MMU.
 *
 * @param input Space name.
 * @param space_size Demanding space size.
 */

int init_linked_list(linked_list **list)
{
    *list = (linked_list *)malloc(sizeof(linked_list));
    if (list == NULL)
        return -1;
    node *dummy_node = (node *)malloc(sizeof(node));
    dummy_node->rmap = 0;
    (*list)->head = (*list)->tail = dummy_node;
    return 0;
}

void put_node(linked_list *list, node *input)
{
    input->next = NULL;
    input->prev = list->head;
    list->head->next = input;
    list->head = input;
}

node *get_node(linked_list *list)
{
    node *src_node = list->tail->next;
    if(src_node == NULL)
        return NULL;
    node *dest_node = (node *)malloc(sizeof(node));
    dest_node->page_index = src_node->page_index;
    dest_node->rmap = src_node->rmap;
    src_node->rmap = NULL;
    src_node->page_index = 0;
    free(src_node->prev);
    list->tail = src_node;
    return dest_node;
}

void init_space(space *input, unsigned int space_size)
{
    input->alloc_area = (char *)malloc(space_size);
    unsigned int num_of_page_in_input = space_size / PAGE_SIZE;
    init_linked_list(&(input->free_list));
    init_linked_list(&(input->alloc_list));
    for (int i = 1; i < num_of_page_in_input; i++)
    {
        node *temp_node = (node *)malloc(sizeof(node));
        temp_node->page_index = i;
        put_node(input->free_list, temp_node);
    }
}

// 메모리와 스왑 공간 동적할당
void *ku_mmu_init(unsigned int pmem_size, unsigned int swap_size)
{
    ku_mmu_PCB_list.head = malloc(sizeof(node));
    init_space(&ku_mmu_pmem, pmem_size);
    init_space(&ku_mmu_swap, swap_size);
    ku_mmu_PCB_list.head = NULL;
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