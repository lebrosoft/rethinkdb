#ifndef __BTREE_MODIFY_FSM_HPP__
#define __BTREE_MODIFY_FSM_HPP__

#include "btree/fsm.hpp"

template <class config_t>
class btree_modify_fsm : public btree_fsm<config_t> {
public:
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::node_t node_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename btree_fsm_t::transition_result_t transition_result_t;
    typedef typename cache_t::buf_t buf_t;
    typedef typename config_t::large_buf_t large_buf_t;
    
    using btree_fsm_t::key;
    
public:

    enum state_t {
        start_transaction,
        acquire_superblock,
        acquire_root,
        acquire_node,
        acquire_large_value,
        operating,
        acquire_sibling,
        update_complete,
        committing
    };

public:
    explicit btree_modify_fsm(btree_key *_key)
        : btree_fsm_t(_key),
          state(start_transaction),
          sb_buf(NULL), buf(NULL), last_buf(NULL), sib_buf(NULL),
          node_id(NULL_BLOCK_ID), last_node_id(NULL_BLOCK_ID), sib_node_id(NULL_BLOCK_ID),
          operated(false), have_computed_new_value(false), new_value(NULL),
          update_needed(false), dest_reached(false), key_found(false), old_large_buf(NULL)
        {}

    transition_result_t do_transition(event_t *event);

    virtual bool is_finished() {
        return state == committing && transaction == NULL;
    }

    /* btree_modify_fsm calls operate() when it finds the leaf node.
     * 'old_value' is the previous value or NULL if the key was not present
     * before. If operate() succeeds (returns true), 'new_value' should be
     * filled with a pointer to the new value or NULL if the key should be
     * deleted; *new_value should remain valid as long as the FSM is alive.
     */
    virtual transition_result_t operate(btree_value *old_value, large_buf_t *old_large_buf, btree_value **new_value) = 0; //, large_buf_t **large_buf) = 0;
    virtual void on_operate_completed() {} // TODO: Rename this.
//    void large_value_filled(); // XXX

public:
    using btree_fsm<config_t>::transaction;
    using btree_fsm<config_t>::cache;

    transition_result_t do_start_transaction(event_t *event);
    transition_result_t do_acquire_superblock(event_t *event);
    transition_result_t do_acquire_root(event_t *event);
    void insert_root(block_id_t root_id);
    transition_result_t do_acquire_node(event_t *event);
    transition_result_t do_acquire_large_value(event_t *event);
    transition_result_t do_acquire_sibling(event_t *event);
    bool do_check_for_split(node_t **node);
    void split_node(buf_t *node, buf_t **rnode, block_id_t *rnode_id, btree_key *median);

    // Some relevant state information
    state_t state;

    buf_t *sb_buf, *buf, *last_buf, *sib_buf;
    block_id_t node_id, last_node_id, sib_node_id; // TODO(NNW): Bufs may suffice for these.

    // When we reach the leaf node and it's time to call operate(), we store the result in
    // 'new_value' until we are ready to insert it.
    bool operated;
    bool have_computed_new_value; // XXX Rename this -- name is too confusing next to "operated".

    btree_value *new_value;

    bool expired;

protected:
    bool update_needed;

private:
    bool dest_reached;
    bool key_found;

    union {
        byte old_value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value old_value;
    };

    large_buf_t *old_large_buf;
    //large_buf_t *new_large_value;
};

// TODO: Figure out includes.
#include "conn_fsm.hpp"

#include "btree/modify_fsm.tcc"

#endif // __BTREE_MODIFY_FSM_HPP__
