#ifndef __MESSAGE_HUB_HPP__
#define __MESSAGE_HUB_HPP__

#include <pthread.h>
#include "utils.hpp"
#include <strings.h>
#include "containers/intrusive_list.hpp"
#include "utils.hpp"
#include "config/args.hpp"
#include "config/code.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "alloc/alloc_mixin.hpp"
#include "btree/node.hpp"

// TODO: perhaps we can issue cache prefetching commands to the CPU to
// speed up the process of sending messages across cores.

int key_to_cpu(btree_key *key, unsigned int ncpus);

int key_to_slice(btree_key *key, unsigned int ncpus, unsigned int nslice);

struct cpu_message_t : public intrusive_list_node_t<cpu_message_t>
{
    enum msg_type_t {
        mt_btree,
        mt_lock,
        mt_perfmon,
        mt_log,
        mt_read_large_value,
        mt_write_large_value
    };
    explicit cpu_message_t(msg_type_t _type)
        : type(_type), request(NULL)
        {}
    virtual ~cpu_message_t() {}
    
    msg_type_t type;
    request_t *request;
    unsigned int return_cpu;

    void send(int cpu);
};

struct event_queue_t;

struct message_hub_t {
public:
    typedef intrusive_list_t<cpu_message_t> msg_list_t;
    
public:
    void init(unsigned int cpu_id, unsigned int _ncpus, worker_t *eqs[]);
    ~message_hub_t();
    
    // Collects a message for a given CPU onto a local list.
    void store_message(unsigned int ncpu, cpu_message_t *msg);

    // Pushes messages collected locally global lists available to all
    // CPUs.
    void push_messages();
    
    // Pulls the messages stored in global lists for a given CPU.
    void pull_messages(unsigned int ncpu, msg_list_t *msg_list);

public:
    struct cpu_queue_t {
        // Lists of messages for each CPU local to this thread. 
        msg_list_t msg_local_list;

        // Lists of messages for each CPU global to all threads (needs to
        // be protected when accessed)
        msg_list_t msg_global_list;

        // Spinlock for the global list 
        pthread_spinlock_t lock;

        // An event queue for the given core
        event_queue_t *eq;
    };
    cpu_queue_t queues[MAX_CPUS];
    unsigned int ncpus;
    unsigned int current_cpu;
};

#endif // __MESSAGE_HUB_HPP__
