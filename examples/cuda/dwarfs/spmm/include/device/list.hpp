namespace list
{
    /**
     * simple list head
     */
    typedef struct list_node {
        struct list_node *next;
    } list_node_t;
    typedef list_node_t  list_head_t;
    /**
     * list structure
     */
    typedef struct list {
        list_head_t head;
        list_node_t *tail;
    } list_t;

    /**
     * initialize list
     */
    static inline void list_init(
        list_t *list
        ) {
        list->tail = &list->head;
        list->head.next = &list->head;
    }

    /**
     * clear a list
     */
    static inline void list_clear(
        list_t *list
        ) {
        list_init(list);
    }

    /**
     * check if list is empty
     */
    static inline int list_empty(
        list_t *list
        ) {
        return list->head.next == &list->head;
    }
    /**
     * append to list
     */
    static inline void list_append(
        list_t *list
        ,list_node_t *node
        ) {
        list->tail->next = node;
        list->tail = node;
        node->next = &list->head;
    }
    /**
     * prepend to list
     */
    static inline void list_prepend(
        list_t *list
        ,list_node_t *node
        ) {
        if (list_empty(list))
            list->tail = node;
        
        node->next = list->head.next;
        list->head.next = node;
    }
    /**
     * extend list with another
     */
    static inline void list_extend(
        list_t *extend_to
        ,list_t *with
        ) {
        if (!list_empty(with)) {
            extend_to->tail->next = with->head.next;
            extend_to->tail = with->tail;
            extend_to->tail->next = &extend_to->head;
            list_clear(with);
        }
    }

    static inline list_node_t *list_front(
        list_t *list
        ) {
        return list->head.next;
    }
    
    /**
     * pop from the front of the list
     */
    static inline void list_pop_front(
        list_t *list
        ) {
        if (!list_empty(list)) {            
            list_node_t *tmp = list->head.next;
            list->head.next = tmp->next;
            tmp->next = nullptr;

            // update tail if necessary
            if (list_empty(list))
                list->tail = &list->head;
        }
    }

    /**
     * move a list
     */
    static inline void list_move(
        list_t *dst
        ,list_t *src
        ) {
        if (!list_empty(src) && (dst != src)) {
            dst->head.next = src->head.next;
            dst->tail =  src->tail;
            dst->tail->next = &dst->head;
            list_clear(src);
        }
    }    
}
