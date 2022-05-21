#define PAGE_SIZE 4
#define PTE_MASK 0xFE
#define PFN_MASK 0xFC
#pragma pack(1)

/*
If every bit is 0 then it is unmapped 
6bit PFN
1bit unused bit
1bit present bit
*/
typedef struct ku_pte {
    unsigned int present_bit : 1;
    unsigned int unused_bit : 1;
    unsigned int PFN : 6;
} ku_pte;

typedef struct test
{
    unsigned int a :1;
} test;

/*
char pid
void* PRBR
PCB *prev
PCB *next
*/
typedef struct PCB {
    char pid;
    ku_pte *PTBR;
    struct PCB *prev;
    struct PCB *next;
} PCB;

// free list + alloc list node 개수의 합은 PFN 개수와 동일
typedef struct node {
    // char pid;
    ku_pte *rmap;
    // unsigned int prev_index;
    unsigned int next_index;
} node;

/*
int free_head;
int alloc_head;
void *alloc_area;
node *free_and_alloc;
unsigned int full : 1;f
 */
typedef struct space {
    int free_head;
    int alloc_head;
    int free_tail;
    void *alloc_area;
    node *free_and_alloc;
} space;

typedef struct linked_list {
    PCB* head;
} linked_list;

space ku_mmu_swap, ku_mmu_pmem;
/* For controling PCB */
linked_list PCB_list;
linked_list *PCB_list_ptr = &PCB_list;

PCB *tempPCB;

void insertPCB(PCB * input);
PCB *searchPCB(char searching_pid);
int evict();
int swap_out();
int swap_in(ku_pte *accesed_pte);
int map_on_pmem(ku_pte *accesed_pte);

void insertPCB(PCB* input) {
    if(PCB_list_ptr -> head == NULL)
        PCB_list_ptr -> head = input;
    else {
        PCB_list_ptr -> head -> next = input;
        input -> prev = PCB_list_ptr -> head;
    }
    PCB_list_ptr -> head = input;
}

PCB *searchPCB(char searching_pid) {
    tempPCB = PCB_list_ptr -> head;
    while(tempPCB != NULL) {
        if(tempPCB -> pid == searching_pid)
            return tempPCB;
        else
            tempPCB = tempPCB -> prev;
    }
    return NULL;
}

// TODO pmem space에서 alloc list? array?를 구현해 매핑되는 pte의 정보를 가지고 있으면 구현에 용이할 것 같음
// TODO swap space는 pmem과 달리 순서대로 값이 해제되지 않음 -> free list, alloc list 구현 필요할 듯

/**
 * @brief 
 * 
 * @return 0 on success, -1 on error.
 */
int swap_out() {
    /* pmem_index에 매핑되어있는 pte를 swap out시킴 */
    unsigned int swap_dest = ku_mmu_swap.free_head;
    if(!swap_dest) return -1; // swap_dest의 index가 0일 경우 == swap이 꽉 차있을 경우?
    ku_mmu_swap.free_head = ku_mmu_swap.free_and_alloc[ku_mmu_swap.free_head].next_index;
    ku_pte *swap_out_pte = ku_mmu_pmem.free_and_alloc[ku_mmu_pmem.alloc_head].rmap;
    if(!swap_out_pte) return -1;
    swap_out_pte -> PFN = (swap_dest&PTE_MASK)>>1;
    swap_out_pte -> unused_bit = swap_dest&0x1;
    swap_out_pte -> present_bit = 0;
    return 0;
}

/**
 * @brief Swap in accessed pte.
 * @param accesed_pte PTE wants to swap in to pysical memory.
 * @return 0 on success, -1 on error.
 */
int swap_in(ku_pte *accesed_pte) {
    // if(!pmem.free_head)
    //     if(swap_out()) return -1;
    // accesed_pte -> PFN = &pmem.free_and_alloc[pmem.free_head]; // accessed pte에게 PFN 할당
    // accesed_pte -> present_bit = 1;
    // accesed_pte -> unused_bit = 0;
    unsigned int take_back = ((accesed_pte -> PFN)<<1) + accesed_pte -> unused_bit;
    ku_mmu_swap.free_and_alloc[take_back].rmap = NULL; // free 된 swap을 free list에 insert
    ku_mmu_swap.free_and_alloc[take_back].next_index = ku_mmu_swap.alloc_head;
    ku_mmu_swap.alloc_head = take_back;
    if(map_on_pmem(accesed_pte)) return -1;
    // swap.free_and_alloc[take_back].pid = 0;
    // TODO pmem_index에 accessed_pte 매핑
    // TODO accessed_pte의 p-bit을 1로 변경
    return 0;
}

/**
 * @brief Map accessed pte to pysical memory.
 * @param accesed_pte PTE wants to map.
 * @return 0 on success, -1 on error.
 */
int map_on_pmem(ku_pte *accesed_pte) {
    if(!ku_mmu_pmem.free_head)
        if(swap_out()) return -1;
    accesed_pte -> PFN = ku_mmu_pmem.free_head; // accessed pte에게 PFN 할당
    accesed_pte -> present_bit = 1;
    accesed_pte -> unused_bit = 0;
    ku_mmu_pmem.free_and_alloc[ku_mmu_pmem.free_head].rmap = accesed_pte; // 할당된 pte reverse mapping
    ku_mmu_pmem.alloc_head = ku_mmu_pmem.free_and_alloc[ku_mmu_pmem.alloc_head].next_index; // alloc head 변경
    // TODO pmem_index에 accessed_pte 매핑
    // TODO accessed_pte의 p-bit을 1로 변경
    return 0;
}

void init_space(space* input, unsigned int space_size) {
    input -> alloc_area = (char *)malloc(space_size);
    unsigned int num_of_page_in_input = space_size/PAGE_SIZE;
    input -> free_and_alloc = (node *)malloc(space_size*sizeof(node));
    for(int i = 1; i < num_of_page_in_input - 1; i++) {
        // input -> free_and_alloc[i].pid = 0;
        input -> free_and_alloc[i].next_index = i+1;
    }
    // input -> free_and_alloc[num_of_page_in_input].pid = 0;
    input -> free_and_alloc[num_of_page_in_input].next_index = 0;
    input -> free_head = 1;
    input -> alloc_head = 1;
}

// 메모리와 스왑 공간 동적할당
void *ku_mmu_init(unsigned int pmem_size, unsigned int swap_size) {
    init_space(&ku_mmu_pmem, pmem_size);
    init_space(&ku_mmu_swap, swap_size);
    PCB_list_ptr -> head = NULL;
    if(ku_mmu_pmem.alloc_area == NULL) return 0;
    else return ku_mmu_pmem.alloc_area;
}

// 새로운 프로세스 생성 but 실제로 fork()를 통해 생성하는 것이 아닌 새로운 PCB만 생성
// PDBR(CR3) 새로운 프로세스를 위해 갱신
// 전에 생성된 프로세스라면 해당 프로세스의 PDBR 반환
// 프로세스당 256B Page Table 생성
// PCB를 PCB list에 insert
int ku_run_proc(char fpid, void **ku_cr3) {
// PCB가 있는지 탐색
    tempPCB = searchPCB(fpid);
    if(!tempPCB) { // PCBlist에 fpid에 해당하는 PCB가 없을 경우
        tempPCB = (PCB *)malloc(sizeof(PCB));
        tempPCB -> PTBR = (ku_pte *)malloc(sizeof(ku_pte)*64);
        tempPCB -> pid = fpid;
        insertPCB(tempPCB);
    }
    *ku_cr3 = tempPCB -> PTBR;
    return 0;
}


// PTE did not mapped
// swaped out?? or map new PTE?? Both case needs to consider free space in swap and pmem
// TODO 3. 해당 함수가 호출되면 이후 ku_trav()가 실행될 때 address translation 성공 필요
// ! swap 계열의 함수 작동 실패시 -1 반환 후 ku_page_fault 종료해야함
int ku_page_fault(char pid, char va) {
    ku_pte *page_table = tempPCB -> PTBR; // PCB를 통해 
	char pte_offset = (va & PFN_MASK) >> 2;
    // printf("target table addr: %p offset: %d\n", page_table, pte_offset);
    ku_pte *target_pte = (char *)page_table + pte_offset; // 여기서 3시간 날림 대체 char *랑 ku_pte * 크기가 왜 다를까?
    // printf("target pte addr: %p\n", target_pte);
    if(target_pte -> PFN == 0x0 && target_pte -> unused_bit == 0x0)
        /*  
        1. pmem에 빈 자리가 있는지 확인
        2. 빈 자리가 없을 경우 avict() -> pmem_cnt가 끝까지 찼을 경우(왜냐하면 프로세스가 종료되지 않음 -> 이게 골때리네 free list가 굳이 필요가 없구나)
        3. 빈 자리가 있을 경우 그냥 넣기 -> pmem_cnt가 끝까지 차지 않았을 경우 
        */
        map_on_pmem(target_pte);  
    else 
        /* 
        swap이 발생했으므로 pmem에는 빈 자리가 존재할 수 없음 -> pmem_cnt를 통해 FIFO avict 
        각 함수의 정상 작동 여부에 따라 -1을 반환해야 할 수도 있음
        */
        swap_in(target_pte); 
    return 0;
}