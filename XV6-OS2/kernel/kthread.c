#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];
extern struct spinlock wait_lock;
struct spinlock new_lock;



void kthreadinit(struct proc *p)
{
  
  acquire(&p->lock);
 
  initlock(&p->counter_lock, "thread_counter_lock");

  initlock(&new_lock, "new_lock");

  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    initlock(&kt->tlock, "thread_lock");
  
    kt->tstate = TUNUSED;

    kt->pcb = p;

    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));
    
  }
 
  release(&p->lock);
  
}

struct kthread *mykthread()
{
  push_off();
  struct cpu *c = mycpu();
  struct kthread *kt = c->kthread;
  pop_off();
  return kt;
}

int
  alloctpid(struct proc *p){
    int tpid;
    acquire(&p->counter_lock);
    tpid = p->tcounter;
    p->tcounter++;
    release(&p->counter_lock);
    return tpid;
  }


struct kthread*
allockthread(struct proc *p){
 
  struct kthread *kt ;

  for (kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    acquire(&kt->tlock);
    if(kt->tstate == TUNUSED){
        
      kt->tpid = alloctpid(p);
  
      kt->tstate = TUSED;
  
      //feching the trapframe using the required method. the base trapframe of the process should be set before this fetch
      kt->trapframe = get_kthread_trapframe(p, kt);

      memset(&kt->context, 0, sizeof(kt->context));
      kt->context.ra = (uint64)forkret;
      kt->context.sp = kt->kstack +  PGSIZE; 

     

      return kt;
    }

    else{
      release(&kt->tlock);
    }

  }
 
  return 0;

}

void
 freekthread(struct kthread *kt){
  kt->tpid = 0;
  kt->tchan = 0;
  kt->tkilled = 0;
  kt->txstate = 0;
  kt->tstate = TUNUSED;
 }

struct trapframe *get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  return p->base_trapframes + ((int)(kt - p->kthread));
}



// MultiThreads
int kthread_create( void *(*start_func)(), void *stack, uint stack_size){
  int ktid;
  struct kthread *nkt;
  struct proc *p = myproc();
  nkt = allockthread(p);
  // Allocate kthread
  if(nkt  == 0){
    release(&nkt->tlock);
    return -1;
  }

  if(stack == 0){
    release(&nkt->tlock);
    return -1;

  }

  if(stack_size > kSTACK_SIZE){
    printf("im in\n");
    release(&nkt->tlock);
    return -1;

  }


 
  nkt->trapframe->epc = (uint64)start_func;
  nkt->trapframe->sp = (uint64)stack + stack_size;
  ktid = nkt->tpid;
  nkt->tstate = RUNNABLE;
  release(&nkt->tlock);
  return ktid;


}

int kthread_id(void){
  return mykthread()->tpid;
}

int kthread_kill(int ktid){

  struct proc *p = myproc();
  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    
    if(kt->tpid == ktid){
      acquire(&kt->tlock);
      kt->tkilled = 1;
      if(kt->tstate == SLEEPING){
        kt->tstate = RUNNABLE;
      }
      release(&kt->tlock);
      release(&p->lock);
      return 0;
    }
    
  }
  return -1;
}
void kthread_exit(int status){
  
  struct proc *p = myproc();
  struct kthread *kt = mykthread();
  struct kthread *klt;
  int should_terminate = 1;
  
  acquire(&new_lock);
  
  wakeup(kt);

  acquire(&kt->tlock);
  kt->txstate = status;
  kt->tstate = TZOMBIE;

  for (klt = p->kthread; klt < &p->kthread[NKT]; klt++)
  {
    if(klt != kt){
      acquire(&klt->tlock);
      if((klt->tstate != TZOMBIE) && (klt->tstate != TUNUSED)){
        should_terminate = 0;
      }
      release(&klt->tlock);

    }

  }

  release(&new_lock);
  

  if(should_terminate){
    release(&kt->tlock);
    exit(status);
  }
  

  sched();
  panic("zombie exit");

}

int kthread_join(int ktid, uint64 status){
  struct proc *p = myproc();
  struct kthread *kt ;

 

  for (kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    if(kt->tpid == ktid){
      break;
    }

  }

  if(kt->tpid == ktid){
  
  acquire(&new_lock);
  
  for(;;){
    
    acquire(&kt->tlock);
    if(kt->tstate == TZOMBIE || mykthread()->tpid == ktid){
        if(status != 0 && copyout(p->pagetable, status, (char *)&kt->txstate,
                                  sizeof(kt->txstate)) < 0) {                    
            release(&kt->tlock);
            release(&new_lock);
            return -1;
            }
          freekthread(kt);
          release(&kt->tlock);
          release(&new_lock);
          return 0;
    }
        release(&kt->tlock);
        sleep(kt, &new_lock);
    }

  }
  return -1;
 
  
  }
             


      
 


