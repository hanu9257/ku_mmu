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
void *swap = NULL;

int evict() {
    return 0;
}

int swap_out() {
    return 0;
}

int swap_in(ku_pte *accesed_pte) {
    return 0;
}

int map_on_pmem(ku_pte *accesed_pte) {
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

// TODO 1. 새로운 프로세스 생성 but 실제로 fork()를 통해 생성하는 것이 아닌 새로운 PCB만 생성
// TODO 2. PDBR(CR3) 새로운 프로세스를 위해 갱신
// TODO 3. 전에 생성된 프로세스라면 해당 프로세스의 PDBR 반환
// TODO 4. 프로세스당 256B Page Table 생성
// TODO 5. PCB를 PCB list에 insert
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

int pmem_full_flag = 0;

// TODO 1. PTE did not mapped
// TODO 2. swaped out?? or map new PTE?? Both case needs to consider free space in swap and pmem
// TODO 3. 해당 함수가 호출되면 이후 ku_trav()가 실행될 때 address translation 성공 필요
int ku_page_fault(char pid, char va) {
    ku_pte *page_table = tempPCB -> PTBR; // PCB를 통해 
	char pte_offset = (va & PFN_MASK) >> 2;
    ku_pte *target_pte = page_table+pte_offset;
    if(target_pte -> PFN == 0x0 && target_pte -> unused_bit == 0x0) {
        // TODO : Demand paging
        /*  
        1. pmem에 빈 자리가 있는지 확인
        2. 빈 자리가 없을 경우 avict() -> pmem_cnt가 끝까지 찼을 경우(왜냐하면 프로세스가 종료되지 않음 -> 이게 골때리네 free list가 굳이 필요가 없구나)
        3. 빈 자리가 있을 경우 그냥 넣기 -> pmem_cnt가 끝까지 차지 않았을 경우 
        */
        if(pmem_full_flag) {
            evict();
            map_on_pmem(target_pte);
        } else {
            map_on_pmem(target_pte);
        }    
    } else {
        // TODO : swap in
        swap_out();
        swap_in(target_pte);
        /* 
        swap이 발생했으므로 pmem에는 빈 자리가 존재할 수 없음 -> pmem_cnt를 통해 FIFO avict 
        */
    }
    return 0;
}