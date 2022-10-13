#ifndef _PCB_H
#define _PCB_H


/* process control block  */
typedef struct pcb {
    int pid;
    void *exit_status;
    unsigned int num_threads;
    void *page_directory;
    struct pcb *parent_pcb;
    // hashtable_t alloc_pages;
} pcb_t;



#endif /* _PCB_H */