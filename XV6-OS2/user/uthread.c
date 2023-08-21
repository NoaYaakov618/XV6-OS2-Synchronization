#include "uthread.h"
#include "user.h"
struct uthread uthreads_table [MAX_UTHREADS];
struct uthread *current_thread;
int on = 0;




int uthread_create(void (*start_func)(), enum sched_priority priority){
    struct uthread *t;
    for (t = uthreads_table; t < &uthreads_table[MAX_UTHREADS]; t++)
    {
        if(t->state == FREE){
            // Set up the new thread's stack
            if(t->ustack == 0){
                return -1;
            }
            t->context.sp = (uint64)t->ustack + STACK_SIZE;
            t->context.ra = (uint64)start_func;
            t->priority = priority;
            t->state = RUNNABLE;
            return 0;
        }
    }
    return -1;
    }


    

void uthread_yield(){
    struct uthread *next = get_next_max_thread();

    if(next == 0 || next->priority < current_thread->priority){
        next = current_thread;

    }
    struct uthread *old = current_thread;
    current_thread->state = RUNNABLE;
    next->state = RUNNING;
    current_thread = next;
    uswtch(&old->context,&next->context);

}

void uthread_exit(){
    struct uthread *next = get_next_max_thread();
     if(next == 0 ){
        exit(0);
    }
    struct uthread *old = current_thread;
    current_thread->state = FREE;
    next->state = RUNNING;
    current_thread = next;
    uswtch(&old->context,&next->context);

}


int uthread_start_all(){
   
    if(on == 0){
        on = 1;
 
    struct uthread *temp;
    struct uthread *max_thread = uthreads_table;
    enum sched_priority max_priority = max_thread->priority;
    for (temp = uthreads_table; temp < &uthreads_table[MAX_UTHREADS]; temp++)
    {
        if(temp->state == RUNNABLE){
            if(temp->priority > max_priority){
                max_priority = temp->priority;
                max_thread = temp;
            }
        }
    }

    current_thread = max_thread;
    current_thread->state = RUNNING;
    struct context name;
    uswtch(&name,&current_thread->context);
    return 0;

    }

    else{
        return -1;
    }


}
enum sched_priority uthread_set_priority(enum sched_priority priority){
    enum sched_priority prev_priority = current_thread->priority;
    current_thread->priority = priority;
    return prev_priority;
}

enum sched_priority uthread_get_priority(){
    return current_thread->priority;
}

struct uthread* uthread_self(){
    return current_thread;
}

struct uthread* get_next_max_thread(){
    struct uthread* temp = current_thread++;
    struct uthread* max_thread = temp;
    enum sched_priority max_priority = temp->priority;

    for (temp = current_thread++;  temp < &uthreads_table[MAX_UTHREADS]; temp++)
    {
         if(temp->state == RUNNABLE){
            if(temp->priority > max_priority){
                max_priority = temp->priority;
                max_thread = temp;
            }
         }
    }

    if(max_thread->state == RUNNABLE){
        return max_thread;
    }
    else{
        return 0 ;
    }


}



