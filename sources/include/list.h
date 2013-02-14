struct list_head {
  struct list_head *next;
};

#define STATIC_LINKED_LIST_INIT(mem, type, field, size) \
  do {                                                  \
    type *elements = (type *) mem;                      \
    uint32_t nb_elements = size / sizeof(type);         \
    uint32_t i;                                         \
    for (i = 0; i < nb_elements; i++) {                 \
      elements[i].field.next = &elements[i + 1].field;  \
    }                                                   \
    elements[nb_elements - 1].field.next = NULL;        \
  } while (0);
