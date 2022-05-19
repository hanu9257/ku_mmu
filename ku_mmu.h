/*
If every bit is 0 then it is unmapped 
{6bit PFN, 1bit unused bit, 1bit present bit}
*/
struct ku_pte
{
    unsigned int PFN : 6;
    unsigned int unused_bit : 1;
    unsigned int present_bit : 1;
}ku_pte;


unsigned int ku_mmu_init(unsigned int pmem_size, unsigned int swap_size) {
// TODO 메모리와 스왑 공간 동적할당
}

int ku_run_proc(char fpid, void **ku_cr3) {
// TODO 
}

int ku_page_fault(char pid, char va) {
// TODO
}g