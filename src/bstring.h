// ----- Bstring: growable string declarations ----- //
#include <unistd.h>
#include <stdbool.h>

struct Bstring
{
    char *data;
    size_t length;
    size_t capacity;
};

#define BSTRING_INIT_CAPACITY 16

struct Bstring *bstring_init(size_t capacity, const char *const s);
bool bstring_append(struct Bstring *self, const char *const s);
void bstring_free(struct Bstring *self);
