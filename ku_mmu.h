#define PAGE_SIZE 4
#define PTE_MASK 0xFE
#define PFN_MASK 0xFC

/*
If every bit is 0 then it is unmapped 
6bit PFN
1bit unused bit
1bit present bit
*/
typedef struct ku_pte {
    unsigned int PFN : 6;
    unsigned int unused_bit : 1;
    unsigned int present_bit : 1;
} ku_pte;

/*
char pid
void* PRBR
PCB *prev
PCB *next
*/
typedef struct PCB {
    char pid;
    void *PTBR;
    struct PCB *prev;
    struct PCB *next;
} PCB;

struct linked_list {
    PCB* head;
} linked_list;

/* For controling PCB */
struct linked_list *PCB_list = &linked_list;

PCB *tempPCB;

// TODO pmem : free list, allocated queue, swap : free list
int pmem_index = 0; 
int swap_index = 0;

void insertPCB(PCB * input);
PCB *searchPCB(char searching_pid);
int evict();
int swap_out();
int swap_in(ku_pte *accesed_pte);
int map_on_pmem(ku_pte *accesed_pte);

void insertPCB(PCB* input) {
    if(PCB_list -> head == NULL)
        PCB_list -> head = input;
    else {
        PCB_list -> head -> next = input;
        input -> prev = PCB_list -> head;
    }
    PCB_list -> head = input;
}

PCB *searchPCB(char searching_pid) {
    tempPCB = PCB_list -> head;
    while(tempPCB != NULL) {
        if(tempPCB -> pid == searching_pid)
            return tempPCB;
        else
            tempPCB = tempPCB -> prev;
    }
    return NULL;
}


void *pmem = NULL;
// TODO pmem space에서 alloc list? array?를 구현해 매핑되는 pte의 정보를 가지고 있으면 구현에 용이할 것 같음
void *swap = NULL;
// TODO swap space는 pmem과 달리 순서대로 값이 해제되지 않음 -> free list, alloc list 구현 필요할 듯
int pmem_is_full = 0; 

/**
 * @brief 
 * 
 * @return int 
 */
int evict() {
    return 0;
}


/**
 * @brief 
 * 
 * @return int 
 */
int swap_out() {
    /* pmem_index에 매핑되어있는 pte를 swap out시킴 */
    return 0;
}

/**
 * @brief Swap in accessed pte.
 * @param accesed_pte PTE wants to swap in to pysical memory.
 * @return 0 on success, -1 on error.
 */
int swap_in(ku_pte *accesed_pte) {
    if(pmem_is_full)
        if(swap_out()) return -1;
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
    if(pmem_is_full)
        if(swap_out()) return -1;
    // TODO pmem_index에 accessed_pte 매핑
    // TODO accessed_pte의 p-bit을 1로 변경
    return 0;
}

// 메모리와 스왑 공간 동적할당
void *ku_mmu_init(unsigned int pmem_size, unsigned int swap_size) {
    pmem = (char *)malloc(pmem_size);
    swap = (char *)malloc(swap_size);
    PCB_list -> head = NULL;
    if(pmem == NULL) return 0;
    else return pmem;
}

// 새로운 프로세스 생성 but 실제로 fork()를 통해 생성하는 것이 아닌 새로운 PCB만 생성
// PDBR(CR3) 새로운 프로세스를 위해 갱신
// 전에 생성된 프로세스라면 해당 프로세스의 PDBR 반환
// 프로세스당 256B Page Table 생성
// PCB를 PCB list에 insert
int ku_run_proc(char fpid, void **ku_cr3) {
// PCB가 있는지 탐색
    tempPCB = searchPCB(fpid);
    if(tempPCB == NULL) { // PCBlist에 PCB가 없을 경우
        tempPCB = (PCB *)malloc(sizeof(PCB));
        tempPCB -> PTBR = (ku_pte *)malloc(sizeof(ku_pte)*64);
        tempPCB -> pid = fpid;
        insertPCB(tempPCB);
    }
    *ku_cr3 = tempPCB -> PTBR;
    return 0;
}


// TODO 1. PTE did not mapped
// TODO 2. swaped out?? or map new PTE?? Both case needs to consider free space in swap and pmem
// TODO 3. 해당 함수가 호출되면 이후 ku_trav()가 실행될 때 address translation 성공 필요
// ! swap 계열의 함수 작동 실패시 -1 반환 후 ku_page_fault 종료해야함
int ku_page_fault(char pid, char va) {
    ku_pte *page_table = tempPCB -> PTBR; // PCB를 통해 
	char pte_offset = (va & PFN_MASK) >> 2;
    ku_pte *target_pte = page_table+pte_offset;
    if(target_pte -> PFN == 0x0 && target_pte -> unused_bit == 0x0)
        // TODO : Demand paging
        /*  
        1. pmem에 빈 자리가 있는지 확인
        2. 빈 자리가 없을 경우 avict() -> pmem_cnt가 끝까지 찼을 경우(왜냐하면 프로세스가 종료되지 않음 -> 이게 골때리네 free list가 굳이 필요가 없구나)
        3. 빈 자리가 있을 경우 그냥 넣기 -> pmem_cnt가 끝까지 차지 않았을 경우 
        */
        map_on_pmem(target_pte);  
    else 
        // TODO : swap in
        /* 
        swap이 발생했으므로 pmem에는 빈 자리가 존재할 수 없음 -> pmem_cnt를 통해 FIFO avict 
        각 함수의 정상 작동 여부에 따라 -1을 반환해야 할 수도 있음
        */
        swap_in(target_pte); 
    return 0;
}