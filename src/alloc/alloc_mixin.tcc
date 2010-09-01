#ifndef __ALLOC_MIXIN_TCC__
#define __ALLOC_MIXIN_TCC__

#include "utils.hpp"
#ifndef NDEBUG
#include <typeinfo>
#endif

/**
 * The small object accessor creates small object allocators per
 * thread per type on the fly.
 */
template <class alloc_t>
__thread std::vector<alloc_t*, gnew_alloc<alloc_t*> >*
tls_small_obj_alloc_accessor<alloc_t>::allocs_tl = NULL;

/**
 * Allocation mixin using a TLS accessor for all new() and delete() calls.
 */
template<class accessor_t, class type_t>
void *alloc_mixin_t<accessor_t, type_t>::operator new(size_t size) {
#ifdef NDEBUG
    return accessor_t::template get_alloc<type_t>()->malloc(size);
#else
    // In debug mode we need to store the allocator so we can later
    // check that we're deallocating from the same core.
    void *alloc_ptr = accessor_t::template get_alloc<type_t>();
    void **tmp = (void**)accessor_t::template get_alloc<type_t>()->malloc(sizeof(void*) + size);
    tmp[0] = alloc_ptr;
    return &tmp[1];
#endif
}

template<class accessor_t, class type_t>
void alloc_mixin_t<accessor_t, type_t>::operator delete(void *ptr) {
#ifdef NDEBUG
    accessor_t::template get_alloc<type_t>()->free(ptr);
#else
    // In debug mode, check that we use the same allocator to free as
    // we used to create.
    void **tmp = (void**)ptr;
    bool multithreaded_allocator_misused = (tmp[-1] != accessor_t::template get_alloc<type_t>());
    if (multithreaded_allocator_misused) {
        const char *mangled_name = typeid((type_t*)&tmp[-1]).name();
        char *demangled_name = demangle_cpp_name(mangled_name);
        const char *name = demangled_name ? demangled_name : mangled_name;
        fail("You tried to delete an object of type [ %s ] from the wrong core.\n", name);
        // Here we would free demangled_name, except that fail() never returns so we never get here
    }
    
    accessor_t::template get_alloc<type_t>()->free((void*)&tmp[-1]);
#endif
}

#endif /* __ALLOC_MIXIN_TCC__ */
