#define OFFSET_MASK 0x03
#define PFN_MASK 0xFC

char ku_traverse(void *ku_cr3, char va)
{
	char *entry;
	char pa;

	char pte_offset = (va & PFN_MASK) >> 2;
	char page_offset = (va & OFFSET_MASK);

	if( va == 0 )
		return 0; /* NULL pointer */

	/* page table */
	entry = (char *)ku_cr3 + pte_offset;
	if( !(*entry & 0x01) || (*entry & 0x02) )
		return 0;

	pa = (*entry & PFN_MASK) + page_offset;
	return pa;
}

