#ifndef _TCB_H
#define _TCB_H


typedef struct tcb {
    int tid;
    pcb_t *parent_pcb;
    ureg_t uregs;
    handler_t swexn_handler;
} tcb_t;




#endif /* _TCB_H */