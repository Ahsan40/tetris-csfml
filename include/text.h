#define TXT_ATTR_INVISIBLE (1 << 0)

typedef struct text {
    char               *type;
    char               *scene;
    char               *font;
    char               *text;
    size_t              size;
    struct vector2ui    pos;
    size_t              attr;
    size_t              id;
} Text;

struct slist;

struct slist  *load_texts_from_json(const char *filename);
void            text_destroy(void *text);
