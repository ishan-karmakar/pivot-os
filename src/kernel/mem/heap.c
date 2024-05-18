#include <sys.h>
#include <mem/heap.h>
#include <mem/vmm.h>
#include <mem/pmm.h>
#include <kernel/logging.h>
#include <libc/string.h>

// To lazy to write it, so copied from https://github.com/dreamos82/Dreamos64/blob/master/src/kernel/mem/heap.c
// TODO: Remake this so I actually understand what I'm doing
void init_heap(heap_info_t *hi, vmm_info_t *vmm_info) {
    // Let's allocate the new heap, we rely on the vmm_alloc function for this part.
    uint64_t *heap_addr = valloc(PAGE_SIZE, 0, vmm_info);
    hi->heap_start = (heap_mem_node_t *) ((uint64_t) heap_addr);
    hi->heap_end = hi->heap_start;
    log(Info, "HEAP", "Initialized heap");
}

static size_t align(size_t size) {
    return (size / HEAP_ALLOC_ALIGNMENT + 1) * HEAP_ALLOC_ALIGNMENT;
}

static heap_mem_node_t* create_heap_node( heap_mem_node_t *current_node, size_t size, heap_info_t *hi ) {
    // Here we create a new node for the heap.
    // We basically take the current node and split it in two.
    // The new node will be placed just after the space used by current_node
    // And current_node will be the node containing the information regarding the current kmalloc call.
    uint64_t header_size = sizeof(heap_mem_node_t);
    heap_mem_node_t* new_node = (heap_mem_node_t *) ((void *)current_node + sizeof(heap_mem_node_t) + size);
    new_node->is_free = true;
    new_node->size = current_node->size - (size + header_size);
    new_node->prev = current_node;
    new_node->next = current_node->next;

    if( current_node->next != NULL) {
        current_node->next->prev = new_node;
    }

    current_node->next = new_node;

    if( current_node == hi->heap_end) {
        hi->heap_end = new_node;
    }

    return new_node;
}

static uint64_t compute_heap_end(heap_info_t *hi) {
    return (uint64_t) hi->heap_end + hi->heap_end->size + sizeof(heap_mem_node_t);
}

static uint8_t can_merge(heap_mem_node_t *cur_node) {
    // This function checks if the current node can be merged to both left and right
    // There return value is a 2 bits field: bit #0 is set if the node can be merged right
    // bit #1 is set if the node can be merged left. Bot bits set means it can merge in both diections
    heap_mem_node_t *prev_node = cur_node->prev;
    heap_mem_node_t *next_node = cur_node->next;
    uint8_t available_merges = 0;
    if( prev_node != NULL && prev_node->is_free ) {
        uint64_t prev_address = (uint64_t) prev_node + sizeof(heap_mem_node_t) + prev_node->size;
        if ( prev_address == (uint64_t) cur_node ) {
            available_merges = available_merges | MERGE_LEFT;
        }
    }
    if( next_node != NULL && next_node->is_free ) {
        uint64_t next_address = (uint64_t) cur_node + sizeof(heap_mem_node_t) + cur_node->size;
        if ( next_address == (uint64_t) cur_node->next ) {
            available_merges = available_merges | MERGE_RIGHT;
        }

    }

    return available_merges;
}

static void merge_memory_nodes(heap_mem_node_t *left_node, heap_mem_node_t *right_node) {
    if(left_node == NULL || right_node == NULL) {
        return;
    }
    if(((uint64_t) left_node +  left_node->size + sizeof(heap_mem_node_t)) == (uint64_t) right_node) {
        //We can combine the two nodes:
        //1. Sum the sizes
        left_node->size = left_node->size + right_node->size + sizeof(heap_mem_node_t);
        //2. left_node next item will point to the next item of the right node (since the right node is going to disappear)
        left_node->next = right_node->next;
        //3. Unless we reached the last item, we should also make sure that the element after the right node, will be linked
        //   to the left node (via the prev field)
        if(right_node->next != NULL){
            heap_mem_node_t *next_node = right_node->next;
            next_node->prev = left_node;
        }
    }
}

static void expand_heap(size_t required_size, heap_info_t *hi) {
    //size_t number_of_pages = required_size / KERNEL_PAGE_SIZE + 1;
    size_t number_of_pages = required_size / PAGE_SIZE;
    if (required_size % PAGE_SIZE)
        number_of_pages++;
    uint64_t heap_end = compute_heap_end(hi);
    heap_mem_node_t *new_tail = (heap_mem_node_t *) heap_end;
    new_tail->next = NULL;
    new_tail->prev = hi->heap_end;
    new_tail->size = PAGE_SIZE * number_of_pages;
    new_tail->is_free = true;
    hi->heap_end->next = new_tail;
    hi->heap_end = new_tail;
    // After updating the new tail, we check if it can be merged with previous node
    uint8_t available_merges = can_merge(new_tail);
    if ( available_merges & MERGE_LEFT) {
        merge_memory_nodes(new_tail->prev, new_tail);
    }

}

void *halloc(size_t size, heap_info_t *hi) {
    heap_mem_node_t *current_node = hi->heap_start;
    // If size is 0 we don't need to do anything
    if( size == 0 ) {
        log(Verbose, "heap", "Size is null");
        return NULL;
    }

    //log(Verbose, "(kmalloc) Current heap free size: 0x%x - Required: 0x%x", current_node->size, align(size + sizeof(heap_mem_node_t)));

    while( current_node != NULL ) {
        // The size of a node contains also the size of the header, so when creating nodes we add headers
        // We need to take it into account
        size_t real_size = size + sizeof(heap_mem_node_t);
        //We also need to align it!
        real_size = align(real_size);
        if( current_node->is_free) {
            if( current_node->size >= real_size ) {
                // Ok we have found a node big enough
                if( current_node->size - real_size > HEAP_MINIMUM_ALLOCABLE_SIZE ) {
                    // We can keep shrinking the heap, since we still have enough space!
                    // But we need a new node for the allocated area
                    create_heap_node(current_node, real_size, hi);
                    // Let's update current_node status
                    current_node->is_free = false;
                    current_node->size = real_size;
                } else {
                    // The current node space is not enough for shrinking, so we just need to mark the current_node as busy.
                    // Size should not be touched.
                    current_node->is_free = false;
                    //current_node->size -= real_size;
                }
                return (void *) current_node + sizeof(heap_mem_node_t);
            }
        }

        if( current_node == hi->heap_end ) {
            expand_heap(real_size, hi);
            if( current_node->prev != NULL) {
                // If we are here it means that we were at the end of the heap and needed an expansion
                // So after the expansion there are chances that we reach the end of the heap, and the
                // loop will end here. So let's move back of one item in the list, so we are sure the next item to be picked
                // will be the new one.
                current_node = current_node->prev;
            }
        }
        current_node = current_node->next;
    }
    return NULL;
}




void hfree(void *ptr, heap_info_t *hi) {
    // Before doing anything let's check that the address provided is valid: not null, and within the heap space
    if(ptr == NULL) {
        return;
    }

    if ( (uint64_t) ptr < (uint64_t) hi->heap_start || (uint64_t) ptr > (uint64_t) hi->heap_end) {
        return;
    }

    // Now we can search for the node containing our address
    heap_mem_node_t *current_node = hi->heap_start;
    while( current_node != NULL ) {
        if( ((uint64_t) current_node + sizeof(heap_mem_node_t)) == (uint64_t) ptr) {
            current_node->is_free = true;
            uint8_t available_merges = can_merge(current_node);

            if( available_merges & MERGE_RIGHT ) {
                merge_memory_nodes(current_node, current_node->next);
            }

            if( available_merges & MERGE_LEFT ) {
                merge_memory_nodes(current_node->prev, current_node);
            }
            return;

        }
        current_node = current_node->next;
    }
}

// Frees old ptr and allocates a new one while copying data
// Old ptr must be exactly what was returned from kmalloc (cannot be middle of memory)
void *hrealloc(void *ptr, size_t new_size, heap_info_t *hi) {
    void *new_ptr = halloc(new_size, hi);
    heap_mem_node_t *node = (heap_mem_node_t*) (ptr - sizeof(heap_mem_node_t));
    size_t old_size = node->size;
    memcpy(new_ptr, ptr, old_size);
    hfree(ptr, hi);
    return new_ptr;
}