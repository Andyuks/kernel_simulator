/* data_struct.h*/
#include <unistd.h>
#define MAX_BLOCKS 5
#define BLOCK_PAGES 4
#define MAX_FTL 16
#define MAX_PROGRAMS 20
#define TAM_TLB 4

#define PCB_STATE_IDLE 		0
#define PCB_STATE_RUNNING 	1
#define PCB_STATE_FINISHED 	2
#define PCB_STATE_DEAD 	3

/* structure of mm*/
struct mm_t
{
	int code; // pointer to dir start code segment
	int data; // pointer to dir start data segment
	int end;
	int pgb; // pointer to phys dir of page table
	int table_entries;
};

/* Structure that represents the context of a process */
struct context_t
{
	char prog_name[64];
	unsigned int ri;
	unsigned int pc;
	unsigned int op;
	unsigned int  br[16];
};

/* Structure that represents each process */
struct pcb_t 
{
	struct pcb_t * next;
	struct pcb_t * prev;
	pid_t pid;
	unsigned int state; /* 0: idle (wait), 1:running (run)*, 2:finished*/
	unsigned int ttl;
	unsigned int prio;
	struct context_t * context;
	struct mm_t mm;
};



/* Structure that contais the processess*/
struct processq_t
{
	struct pcb_t   * first;
	struct pcb_t   * last;
	unsigned int numpid;
	unsigned int maxpid;
};

/* structure that represents a tlb entry */
struct tlb_entry_t
{
	unsigned int page;
	unsigned int frame;
};


/* structure that represents a tlb*/
struct tlb_t
{
	unsigned int index;
	unsigned int maxentries;
	struct tlb_entry_t * entry;
};

/* structure that represents a mmu*/
struct mmu_t
{
	struct tlb_t tlb;
};


/* Structure that represents a thread */
struct thread_t
{
	struct pcb_t * cur_pcb;
	struct mmu_t mmu;
	int PTBR;
	int table_entries;
	struct context_t * context;
	unsigned int rd;
	unsigned int ra;
	unsigned int rb;
}; 

/* Structure that represents a core */
struct core_t
{
	struct pcb_t   * pcb_null;
	struct thread_t * thread;
}; 

/* Structure that represents a CPU */
struct cpu_t
{
	struct processq_t * processq;
	struct processq_t * expired;
	unsigned int nextpid;
	struct core_t * core;
}; 

/* structure that represents a gap*/
struct gap_t
{
	struct gap_t * next;
	struct gap_t * prev;
	unsigned int dir;
	unsigned int length;
};

/* Structure that represents a gap queue */
struct gapq_t
{
	struct gap_t   * first;
	struct gap_t   * last;
};

/* Structure that represents the memory*/
struct memory_t
{
	struct gapq_t * kernel_gapq;
	struct gapq_t * gapq;
	int maxprograms;
	unsigned char * data;
};



/* structure that represents a ftl page */
struct ftl_page_t 
{
	int dir; // default -1
};

/* Structure that representes the ftl*/
struct ftl_t
{
	struct ftl_page_t page[MAX_FTL];
};

/* structure that represents a disk page */
struct disk_page_t 
{
	int num; // default -1
	int data; // default -1
	int garbage; // default 0, 1 if garbage
};

/* structure that represents a disk block */
struct disk_block_t
{
	struct disk_page_t page[BLOCK_PAGES];
};

/* Structure to manage garbage collection*/
struct gc_t
{
	int assigned;
	int oldest; 
};

/* Structure that representes the disk*/
struct disk_t
{
	struct disk_block_t block[MAX_BLOCKS];
	struct gc_t gc;
};



/* Structure that represents the machine */
struct machine_t
{
	unsigned int maxcpus;
	unsigned int maxcores;
	unsigned int maxthreads;
	
	struct cpu_t * cpu;
	struct memory_t * memory;
	struct disk_t * disk;
	struct ftl_t * ftl;
};

/* Structure of the parameters to pass to the threads */
struct param_t
{
	struct machine_t * machine;
	int opt;
	int ismem;
};

/* data_struct.c functions */
void initialize_machine(struct machine_t * machine, int ncpu, int ncore, int nthreads, int maxpid, int opt, int ismem);
void free_machine(struct machine_t * machine, int ncpu, int ncore, int nthreads, int opt,  int ismem);
void initialize_pq (struct processq_t * processq, int maxpid);
void pq_addPCB (struct processq_t * processq, struct pcb_t * pcb, int opt);
void pq_delPCB(struct processq_t * processq, struct pcb_t * pcb);
void initialize_gq (struct gapq_t * gapq);
void init_cur(struct machine_t * machine);
void initialize_tlb(struct tlb_t * tlb);
void initialize_kernel_gq (struct gapq_t * gapq);
void init_ftl(struct ftl_t * ftl);
void init_disk(struct disk_t * disk);