
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "s7.h"
static s7_scheme *s7;

/* c-object tests */
typedef struct {
size_t size;
double *data;
} g_block;

static int g_block_type = 0, g_simple_block_type = 0, g_c_tag_type = 0, g_cycle_type = 0;
static s7_pointer g_block_methods;

static s7_pointer g_block_let(s7_scheme *sc, s7_pointer args)
{
#define g_block_let_help "(block-let block) returns the block'e let."
if (s7_c_object_type(s7_car(args)) != g_block_type)
return(s7_wrong_type_arg_error(sc,"block-let", 1, s7_car(args),"a block"));
return(s7_c_object_let(s7_car(args)));
}

static s7_int max_vector_length = 0;
static s7_pointer g_make_block(s7_scheme *sc, s7_pointer args)
{
#define g_make_block_help "(make-block size) returns a new block of the given size"
g_block *g;
s7_pointer new_g, arg1;
s7_int size;
arg1 = s7_car(args);
if (!s7_is_integer(arg1))
return(s7_wrong_type_arg_error(sc,"make-block", 1, arg1,"an integer"));
size = s7_integer(arg1);
if ((size < 0) ||
(size > max_vector_length))
return(s7_out_of_range_error(sc,"make-block", 1, arg1,"should be positive and less than (*s7* 'max-vector-length)"));
g = (g_block *)calloc(1, sizeof(g_block));
if (!g) return(s7_error(sc, s7_make_symbol(sc,"out-of-memory"), s7_list(sc, 1, s7_make_string(sc,"unable to allocate block"))));
g->size = (size_t)size;
if (g->size > 0)
g->data = (double *)calloc(g->size, sizeof(double));
else g->data = NULL;
new_g = s7_make_c_object(sc, g_block_type, (void *)g);
s7_c_object_set_let(new_g, g_block_methods);
s7_openlet(sc, new_g);
return(new_g);
}

static s7_pointer g_make_simple_block(s7_scheme *sc, s7_pointer args)
{
#define g_make_simple_block_help "(make-simple-block size) returns a new simple-block of the given size"
g_block *g;
s7_pointer new_g, arg1;
s7_int size;
arg1 = s7_car(args);
if (!s7_is_integer(arg1))
return(s7_wrong_type_arg_error(sc,"make-simple-block", 1, arg1,"an integer"));
size = s7_integer(arg1);
if ((size < 0) ||
(size > max_vector_length))
return(s7_out_of_range_error(sc,"make-simple-block", 1, arg1,"should be positive and less than (*s7* 'max-vector-length)"));
g = (g_block *)calloc(1, sizeof(g_block));
if (!g) return(s7_error(sc, s7_make_symbol(sc,"out-of-memory"), s7_list(sc, 1, s7_make_string(sc,"unable to allocate block"))));
g->size = (size_t)size;
if (g->size > 0)
g->data = (double *)calloc(g->size, sizeof(double));
else g->data = NULL;
new_g = s7_make_c_object(sc, g_simple_block_type, (void *)g);
return(new_g);
}

static s7_pointer g_make_c_tag(s7_scheme *sc, s7_pointer args)
{
s7_int *tag;
tag = (s7_int *)calloc(1, sizeof(s7_int));
*tag = 23;
return(s7_make_c_object(sc, g_c_tag_type, (void *)tag));
}

static void g_c_tag_free(void *val)
{
free(val);
}

typedef struct { s7_pointer obj; } g_cycle;
static s7_pointer g_make_cycle(s7_scheme *sc, s7_pointer args)
{
g_cycle *g;
g = (g_cycle *)malloc(sizeof(g_cycle));
g->obj = s7_car(args);
return(s7_make_c_object(sc, g_cycle_type, (void *)g));
}

static s7_pointer g_cycle_ref(s7_scheme *sc, s7_pointer args)
{
g_cycle *g;
g = (g_cycle *)s7_c_object_value(s7_car(args));
return(g->obj);
}

static s7_pointer g_cycle_to_list(s7_scheme *sc, s7_pointer args)
{
g_cycle *g;
g = (g_cycle *)s7_c_object_value(s7_car(args));
return(s7_cons(sc, g->obj, s7_nil(sc)));
}


static s7_pointer g_cycle_set(s7_scheme *sc, s7_pointer args)
{
g_cycle *g;
g = (g_cycle *)s7_c_object_value(s7_car(args));
g->obj = s7_cadr(args);
return(g->obj);
}

static s7_pointer g_cycle_implicit_set(s7_scheme *sc, s7_pointer args)
{
g_cycle *g;
s7_pointer val;
s7_int index;
g = (g_cycle *)s7_c_object_value(s7_car(args));
if ((!s7_is_integer(s7_cadr(args))) ||
(s7_integer(s7_cadr(args)) != 0))
return(s7_out_of_range_error(sc,"implicit cycle-set!", 2, s7_cadr(args),"should be 0"));
g->obj = s7_caddr(args);
return(g->obj);
}

static void g_cycle_mark(void *val)
{
s7_mark(((g_cycle *)val)->obj);
}

static void g_cycle_free(void *val)
{
g_block *g = (g_block *)val;
free(g);
}

static s7_pointer g_to_block(s7_scheme *sc, s7_pointer args)
{
#define g_block_help "(block ...) returns a block c_object with the arguments as its contents."
s7_pointer p, b;
size_t i, len;
g_block *gb;
len = s7_list_length(sc, args);
b = g_make_block(sc, s7_cons(sc, s7_make_integer(sc, len), s7_nil(sc)));
gb = (g_block *)s7_c_object_value(b);
for (i = 0, p = args; i < len; i++, p = s7_cdr(p))
gb->data[i] = s7_number_to_real(sc, s7_car(p));
return(b);
}

static char *g_block_display(s7_scheme *sc, void *value)
{
g_block *b = (g_block *)value;
s7_int i, len;
char *buf;
char flt[64];
len = b->size;
if (len > s7_print_length(sc)) len = s7_print_length(sc);
buf = (char *)calloc((len + 1) * 64, sizeof(char));
if (!buf) s7_error(sc, s7_make_symbol(sc,"out-of-memory"), s7_list(sc, 1, s7_make_string(sc,"unable to allocate string to display block")));
snprintf(buf, (len + 1) * 64,"(block");
for (i = 0; i < len; i++)
{
snprintf(flt, 64," %.3f", b->data[i]);
strcat(buf, flt);
}
if (b->size > s7_print_length(sc))
strcat(buf," ...)");
else strcat(buf,")");
return(buf);
}

static char *g_block_display_readably(s7_scheme *sc, void *value)
{
char *buf;
s7_int i, len;
char flt[64];
g_block *b = (g_block *)value;
len = b->size;
buf = (char *)calloc((len + 1) * 64, sizeof(char));
snprintf(buf, (len + 1) * 64,"(block");
for (i = 0; i < len; i++)
{
snprintf(flt, 64," %.16g", b->data[i]);
strcat(buf, flt);
}
strcat(buf,")");
return(buf);
}

static s7_pointer g_block_to_string(s7_scheme *sc, s7_pointer args)
{
s7_pointer obj, choice;
char *descr;
obj = s7_car(args);
if (s7_is_pair(s7_cdr(args)))
choice = s7_cadr(args);
else choice = s7_t(sc);
if (choice == s7_make_keyword(sc,"readable"))
descr = g_block_display_readably(sc, s7_c_object_value(obj));
else descr = g_block_display(sc, s7_c_object_value(obj));
obj = s7_make_string(sc, descr);
free(descr);
return(obj);
}

static void g_block_free(void *value)
{
g_block *g = (g_block *)value;
if (g->data) free(g->data);
free(g);
}

static bool g_block_is_equal(void *val1, void *val2)
{
s7_int i, len;
g_block *b1 = (g_block *)val1;
g_block *b2 = (g_block *)val2;
if (val1 == val2) return(true);
len = b1->size;
if (len != b2->size) return(false);
for (i = 0; i < len; i++)
if (b1->data[i] != b2->data[i])
return(false);
return(true);
}

static void g_block_mark(void *val)
{
/* nothing to mark */
}

static s7_pointer g_is_block(s7_scheme *sc, s7_pointer args)
{
#define g_is_block_help "(block? obj) returns #t if obj is a block."
return(s7_make_boolean(sc, s7_c_object_type(s7_car(args)) == g_block_type));
}

static s7_pointer g_is_simple_block(s7_scheme *sc, s7_pointer args)
{
#define g_is_simple_block_help "(simple-block? obj) returns #t if obj is a simple-block."
return(s7_make_boolean(sc, s7_c_object_type(s7_car(args)) == g_simple_block_type));
}

static s7_pointer g_block_ref(s7_scheme *sc, s7_pointer args)
{
g_block *g;
size_t index;
s7_pointer ind;
g = (g_block *)s7_c_object_value(s7_car(args));
if (s7_is_null(sc, s7_cdr(args))) /* this is for an (obj) test */
return(s7_make_integer(sc, 32));
ind = s7_cadr(args);
if (!s7_is_integer(ind))
{
if (s7_is_symbol(ind))
return(s7_symbol_local_value(sc, ind, g_block_methods));
return(s7_wrong_type_arg_error(sc,"(implicit) block-ref", 2, ind,"an integer"));
}
index = (size_t)s7_integer(ind);
if (index < g->size)
return(s7_make_real(sc, g->data[index]));
return(s7_out_of_range_error(sc,"(implicit) block-ref", 2, ind,"should be less than block length"));
}

static s7_pointer g_block_set(s7_scheme *sc, s7_pointer args)
{
g_block *g;
s7_int index;
s7_pointer obj;
obj = s7_car(args);
if (s7_is_immutable(obj))
return(s7_wrong_type_arg_error(sc,"block-set!", 1, obj,"a mutable block"));
if (!s7_is_integer(s7_cadr(args)))
return(s7_wrong_type_arg_error(sc,"block-set!", 2, s7_cadr(args),"an integer"));
g = (g_block *)s7_c_object_value(obj);
index = s7_integer(s7_cadr(args));
if ((index >= 0) && (index < g->size))
{
g->data[index] = s7_number_to_real(sc, s7_caddr(args));
return(s7_caddr(args));
}
return(s7_out_of_range_error(sc,"block-set", 2, s7_cadr(args),"should be less than block length"));
}

static s7_pointer g_block_length(s7_scheme *sc, s7_pointer args)
{
g_block *g = (g_block *)s7_c_object_value(s7_car(args));
return(s7_make_integer(sc, g->size));
}

static s7_int get_start_and_end(s7_scheme *sc, s7_pointer args, s7_int *start, s7_int end)
{
if (s7_is_pair(s7_cdr(args)))
{
s7_pointer p;
p = s7_cadr(args);
if (s7_is_integer(p))
{
s7_int nstart;
nstart = s7_integer(p);
if ((nstart < 0) || (nstart >= end))
{
s7_out_of_range_error(sc,"subblock", 2, p,"should be less than block length"); return(0);}
*start = nstart;
}
if (s7_is_pair(s7_cddr(args)))
{
p = s7_caddr(args);
if (s7_is_integer(p))
{
s7_int nend;
nend = s7_integer(p);
if (nend <= *start)
{
s7_out_of_range_error(sc,"subblock", 3, p,"should be greater than the start point"); return(0);}
if (nend < end) end = nend;
}
}
}
return(end - *start);
}

static s7_pointer g_block_copy(s7_scheme *sc, s7_pointer args)
{
s7_pointer new_g, obj;
g_block *g, *g1;
size_t len;
s7_int start = 0;
obj = s7_car(args);
if (s7_c_object_type(obj) != g_block_type)   /* obj might not be a block object if destination is one */
{
if (s7_is_float_vector(obj))
{
s7_pointer v, dest;
dest = s7_cadr(args);
g = (g_block *)s7_c_object_value(dest);
len = g->size;
if (s7_is_null(sc, s7_cddr(args)))
{
if (len > s7_vector_length(obj)) len = s7_vector_length(obj);
memcpy((void *)(g->data), (void *)(s7_float_vector_elements(obj)), len * sizeof(s7_double));
return(dest);
}
v = s7_make_float_vector_wrapper(sc, len, g->data, 1, NULL, false);
s7_gc_protect_via_stack(sc, v);
s7_copy(sc, s7_cons(sc, obj, s7_cons(sc, v, s7_cddr(args))));
return(dest);
}
return(s7_f(sc));
}
g = (g_block *)s7_c_object_value(obj);
len = g->size;
if (s7_is_pair(s7_cdr(args)))
{
new_g = s7_cadr(args);
if (s7_is_immutable(new_g))
return(s7_wrong_type_arg_error(sc,"block-copy!", 0, new_g,"a mutable block"));
if (s7_c_object_type(new_g) != g_block_type) /* fall back on the float-vector code using a wrapper */
{
s7_pointer v;
v = s7_make_float_vector_wrapper(sc, len, g->data, 1, NULL, false);
s7_gc_protect_via_stack(sc, v);
new_g = s7_copy(sc, s7_cons(sc, v, s7_cdr(args)));
return(new_g);
}
if (s7_is_pair(s7_cddr(args)))
len = get_start_and_end(sc, s7_cdr(args), &start, len);
}
else new_g = g_make_block(sc, s7_cons(sc, s7_make_integer(sc, len), s7_nil(sc)));
g1 = (g_block *)s7_c_object_value(new_g);
if (g1->size < len) len = g1->size;
memcpy((void *)(g1->data), (void *)(g->data + start), len * sizeof(double));
return(new_g);
}

static s7_pointer g_blocks_are_morally_equal(s7_scheme *sc, s7_pointer args)
{
#define g_blocks_are_morally_equal_help "(morally-equal? block1 block2)"
s7_pointer v1, v2, arg1, arg2;
g_block *g1, *g2;
bool result;
uint32_t gc1, gc2;
size_t len;
arg1 = s7_car(args);
arg2 = s7_cadr(args);
if (arg1 == arg2)
return(s7_make_boolean(sc, true));
if (s7_is_let(arg1))             /* (block-let (block)) */
return(s7_make_boolean(sc, false));    /* checked == above */
g1 = (g_block *)s7_c_object_value(arg1);
if (s7_c_object_type(arg2) != g_block_type)
return(s7_make_boolean(sc, false));
g2 = (g_block *)s7_c_object_value(arg2);
len = g1->size;
if (len != g2->size)
return(s7_make_boolean(sc, false));
v1 = s7_make_float_vector_wrapper(sc, len, g1->data, 1, NULL, false);
gc1 = s7_gc_protect(sc, v1);
v2 = s7_make_float_vector_wrapper(sc, len, g2->data, 1, NULL, false);
gc2 = s7_gc_protect(sc, v2);
result = s7_is_morally_equal(sc, v1, v2);
s7_gc_unprotect_at(sc, gc1);
s7_gc_unprotect_at(sc, gc2);
return(s7_make_boolean(sc, result));
}

static s7_pointer g_block_append(s7_scheme *sc, s7_pointer args)
{
#define g_block_append_help "(append block...) returns a new block containing the argument blocks concatenated."
s7_int i, len = 0;
s7_pointer p, new_g;
g_block *g;
for (i = 0, p = args; s7_is_pair(p); p = s7_cdr(p), i++)
{
g_block *g1;
if (s7_c_object_type(s7_car(p)) != g_block_type)
return(s7_wrong_type_arg_error(sc,"block-append", i, s7_car(p),"a block"));
g1 = (g_block *)s7_c_object_value(s7_car(p));
len += g1->size;
}
new_g = g_make_block(sc, s7_cons(sc, s7_make_integer(sc, len), s7_nil(sc)));
g = (g_block *)s7_c_object_value(new_g);
for (i = 0, p = args; s7_is_pair(p); p = s7_cdr(p))
{
g_block *g1;
g1 = (g_block *)s7_c_object_value(s7_car(p));
memcpy((void *)(g->data + i), (void *)(g1->data), g1->size * sizeof(double));
i += g1->size;
}
return(new_g);
}

static s7_pointer g_block_reverse(s7_scheme *sc, s7_pointer args)
{
size_t i, j;
g_block *g, *g1;
s7_pointer new_g;
double *data;
if (!s7_is_null(sc, s7_cdr(args)))
return(s7_wrong_number_of_args_error(sc,"(block-)reverse", args));
g = (g_block *)s7_c_object_value(s7_car(args));
new_g = g_make_block(sc, s7_cons(sc, s7_make_integer(sc, g->size), s7_nil(sc)));
g1 = (g_block *)s7_c_object_value(new_g);
data = g->data;
for (i = 0, j = g->size - 1; i < g->size; i++, j--)
g1->data[i] = g->data[j];
return(new_g);
}

static s7_pointer g_block_reverse_in_place(s7_scheme *sc, s7_pointer args)
{
#define g_block_reverse_in_place_help "(block-reverse! block) returns block with its data reversed."
size_t i, j;
g_block *g;
double *data;
s7_pointer obj;
obj = s7_car(args);
if (s7_c_object_type(obj) != g_block_type)
return(s7_wrong_type_arg_error(sc,"block-reverse!", 0, obj,"a block"));
if (s7_is_immutable(obj))
return(s7_wrong_type_arg_error(sc,"block-reverse!", 0, obj,"a mutable block"));
g = (g_block *)s7_c_object_value(obj);
if (g->size < 2) return(obj);
data = g->data;
if ((g->size & 0xf) == 0)
{
for (i = 0, j = g->size - 1; i < j; )
{
double temp;
temp = data[i]; data[i] = data[j]; data[j--] = temp; i++;
temp = data[i]; data[i] = data[j]; data[j--] = temp; i++;
temp = data[i]; data[i] = data[j]; data[j--] = temp; i++;
temp = data[i]; data[i] = data[j]; data[j--] = temp; i++;
temp = data[i]; data[i] = data[j]; data[j--] = temp; i++;
temp = data[i]; data[i] = data[j]; data[j--] = temp; i++;
temp = data[i]; data[i] = data[j]; data[j--] = temp; i++;
temp = data[i]; data[i] = data[j]; data[j--] = temp; i++;
}
}
else
{
for (i = 0, j = g->size - 1; i < j; i++, j--)
{
double temp;
temp = data[i];
data[i] = data[j];
data[j] = temp;
}
}
return(obj);
}

static s7_pointer g_block_fill(s7_scheme *sc, s7_pointer args)
{
s7_pointer obj, val;
size_t i, len;
s7_int start = 0;
double fill_val;
g_block *g;
double *data;
obj = s7_car(args);
if (s7_is_immutable(obj))
return(s7_wrong_type_arg_error(sc,"block-fill!", 0, obj,"a mutable block"));
val = s7_cadr(args);
g = (g_block *)s7_c_object_value(obj);
fill_val = s7_number_to_real(sc, val);
len = g->size;
if (s7_is_pair(s7_cddr(args)))
len = get_start_and_end(sc, s7_cdr(args), &start, len);
data = (double *)(g->data + start);
if (fill_val == 0.0)
{
if ((g->size & 0x7) == 0)
{
for (i = 0; i < len; )
{
data[i++] = 0.0;
data[i++] = 0.0;
data[i++] = 0.0;
data[i++] = 0.0;
data[i++] = 0.0;
data[i++] = 0.0;
data[i++] = 0.0;
data[i++] = 0.0;
}
}
else memset((void *)data, 0, len * sizeof(double));
}
else
{
for (i = 0; i < len; i++)
data[i] = fill_val;
}
return(obj);
}

static s7_pointer g_blocks(s7_scheme *sc, s7_pointer args)
{
return(s7_copy(sc, s7_list(sc, 1, args)));
}

static s7_pointer g_subblock(s7_scheme *sc, s7_pointer args)
{
#define g_subblock_help "(subblock block (start 0) end) returns a portion of the block."
s7_pointer p, new_g, obj;
s7_int start = 0, new_len, i;
g_block *g, *g1;
obj = s7_car(args);
if (s7_c_object_type(obj) != g_block_type)
return(s7_wrong_type_arg_error(sc,"subblock", 1, obj,"a block"));
g = (g_block *)s7_c_object_value(obj);
new_len = get_start_and_end(sc, args, &start, g->size);
new_g = g_make_block(sc, s7_cons(sc, s7_make_integer(sc, new_len), s7_nil(sc)));
g1 = (g_block *)s7_c_object_value(new_g);
memcpy((void *)(g1->data), (void *)(g->data + start), new_len * sizeof(double));
return(new_g);
}

/* function port tests */
static unsigned char *fout = NULL;
static unsigned int fout_size = 0, fout_loc = 0;
static void foutput(s7_scheme *sc, unsigned char c, s7_pointer port)
{
if (fout_size == fout_loc)
{
if (fout_size == 0)
{
fout_size = 128;
fout = (unsigned char *)malloc(fout_size * sizeof(unsigned char));
}
else
{
fout_size += 128;
fout = (unsigned char *)realloc(fout, fout_size * sizeof(unsigned char));
}
}
fout[fout_loc++] = c;
}

static s7_pointer fout_open(s7_scheme *sc, s7_pointer args)
{
return(s7_open_output_function(sc, foutput));
}

static s7_pointer fout_get_output(s7_scheme *sc, s7_pointer args)
{
foutput(sc, 0, s7_car(args)); /* make sure it's null-terminated */
return(s7_make_string_with_length(sc, (const char *)fout, fout_loc - 1));
}

static s7_pointer fout_close(s7_scheme *sc, s7_pointer args)
{
fout_loc = 0;
return(s7_car(args));
}

static const char *fin = NULL;
static unsigned int fin_size = 0, fin_loc = 0;
static s7_pointer finput(s7_scheme *sc, s7_read_t peek, s7_pointer port)
{
switch (peek)
{
case S7_READ_BYTE:
return(s7_make_integer(sc, (int)fin[fin_loc++]));
case S7_READ_CHAR:
return(s7_make_character(sc, fin[fin_loc++]));
case S7_PEEK_CHAR:
return(s7_make_character(sc, fin[fin_loc]));
case S7_READ_LINE:
{
unsigned int i;
s7_pointer result;
for (i = fin_loc; (i < fin_size) && (fin[i] != '\\n'); i++);
result = s7_make_string_with_length(sc, (char *)(fin + fin_loc), i - fin_loc);
fin_loc = i + 1;
return(result);
}
case S7_IS_CHAR_READY:
return(s7_make_boolean(sc, fin_loc < fin_size));
case S7_READ:
return(s7_error(sc, s7_make_symbol(sc,"read-error"), s7_make_string(sc,"can't read yet!")));
}
}

static s7_pointer fin_open(s7_scheme *sc, s7_pointer args)
{
/* arg = string to read */
s7_pointer str;
fin_loc = 0;
str = s7_car(args);
fin = s7_string(str); /* assume caller will GC protect the string */
fin_size = s7_string_length(str);
return(s7_open_input_function(sc, finput));
}

/* dilambda test */
static s7_pointer g_dilambda_test(s7_scheme *sc, s7_pointer args) { return(s7_f(sc)); }
static s7_pointer g_dilambda_set_test(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }

/* hash-table tests */
static s7_pointer g_hloc(s7_scheme *sc, s7_pointer args) { return(s7_make_integer(sc, 0)); }
static s7_pointer g_heq(s7_scheme *sc, s7_pointer args) { return(s7_make_boolean(sc, s7_is_eq(s7_car(args), s7_cadr(args)))); }

/* optimizer tests */
static s7_pointer g_cf10(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }
static s7_pointer g_cf11(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }
static s7_pointer g_cs11(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }

static s7_pointer g_cf20(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }
static s7_pointer g_cf21(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }
static s7_pointer g_cf22(s7_scheme *sc, s7_pointer args) { return(s7_cadr(args)); }

static s7_pointer g_cf30(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }
static s7_pointer g_cf31(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }
static s7_pointer g_cf32(s7_scheme *sc, s7_pointer args) { return(s7_cadr(args)); }
static s7_pointer g_cf33(s7_scheme *sc, s7_pointer args) { return(s7_caddr(args)); }

static s7_pointer g_cf41(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }
static s7_pointer g_cf42(s7_scheme *sc, s7_pointer args) { return(s7_cadr(args)); }
static s7_pointer g_cf43(s7_scheme *sc, s7_pointer args) { return(s7_caddr(args)); }
static s7_pointer g_cf44(s7_scheme *sc, s7_pointer args) { return(s7_cadddr(args)); }
static s7_pointer g_rs11(s7_scheme *sc, s7_pointer args) { return(s7_make_integer(sc, s7_integer(s7_car(args)) + 1)); }

static s7_pointer g_cf51(s7_scheme *sc, s7_pointer args) { return(s7_car(args)); }

static s7_pointer sload(s7_scheme *sc, s7_pointer args)
{
if (s7_is_string(s7_car(args)))
{
if (s7_is_pair(s7_cdr(args)))
{
if (s7_is_let(s7_cadr(args)))
return(s7_load_with_environment(sc, s7_string(s7_car(args)), s7_cadr(args)));
return(s7_wrong_type_arg_error(sc,"load", 2, s7_cadr(args),"an environment"));
}
return(s7_load(sc, s7_string(s7_car(args))));
}
return(s7_wrong_type_arg_error(sc,"load", 1, s7_car(args),"file name"));
}
static s7_pointer scall(s7_scheme *sc, s7_pointer args) { return(s7_call(sc, s7_car(args), s7_cadr(args))); }
static s7_pointer sread(s7_scheme *sc, s7_pointer args)
{
if (s7_is_pair(args))
return(s7_read(sc, s7_car(args)));
return(s7_read(sc, s7_current_input_port(sc)));
}
static s7_pointer swind(s7_scheme *sc, s7_pointer args) { return(s7_dynamic_wind(sc, s7_car(args), s7_cadr(args), s7_caddr(args))); }
static s7_pointer seval(s7_scheme *sc, s7_pointer args)
{
if (s7_is_pair(s7_cdr(args)))
return(s7_eval(sc, s7_car(args), s7_cadr(args)));
return(s7_eval(sc, s7_car(args), s7_curlet(sc)));
}
static s7_pointer sevalstr(s7_scheme *sc, s7_pointer args)
{
if (s7_is_string(s7_car(args)))
{
if (s7_is_pair(s7_cdr(args)))
{
if (s7_is_let(s7_cadr(args)))
return(s7_eval_c_string_with_environment(sc, s7_string(s7_car(args)), s7_cadr(args)));
return(s7_wrong_type_arg_error(sc,"eval-string", 2, s7_cadr(args),"an environment"));
}
return(s7_eval_c_string_with_environment(sc, s7_string(s7_car(args)), s7_curlet(sc)));
}
return(s7_wrong_type_arg_error(sc,"eval-string", 1, s7_car(args),"string of code"));
}

void block_init(s7_scheme *sc);
void block_init(s7_scheme *sc)
{
s7_pointer cur_env, meq_func;
max_vector_length = s7_integer(s7_let_ref(sc, s7_symbol_value(sc, s7_make_symbol(sc,"*s7*")), s7_make_symbol(sc,"max-vector-length")));
cur_env = s7_outlet(sc, s7_curlet(sc));
g_block_type = s7_make_c_type(sc,"<block>");
s7_c_type_set_free(sc, g_block_type, g_block_free);
s7_c_type_set_equal(sc, g_block_type, g_block_is_equal);
s7_c_type_set_mark(sc, g_block_type, g_block_mark);
s7_c_type_set_ref(sc, g_block_type, g_block_ref);
s7_c_type_set_set(sc, g_block_type, g_block_set);
s7_c_type_set_length(sc, g_block_type, g_block_length);
s7_c_type_set_copy(sc, g_block_type, g_block_copy);
s7_c_type_set_reverse(sc, g_block_type, g_block_reverse);
s7_c_type_set_fill(sc, g_block_type, g_block_fill);
s7_c_type_set_to_string(sc, g_block_type, g_block_to_string);
s7_define_safe_function(sc,"make-block", g_make_block, 1, 0, false, g_make_block_help);
s7_define_safe_function(sc,"block", g_to_block, 0, 0, true, g_block_help);
s7_define_safe_function(sc,"block-let", g_block_let, 1, 0, false, g_block_let_help);
s7_define_safe_function(sc,"subblock", g_subblock, 1, 0, true, g_subblock_help);
s7_define_safe_function(sc,"block-append", g_block_append, 0, 0, true, g_block_append_help);
s7_define_safe_function(sc,"block-reverse!", g_block_reverse_in_place, 1, 0, false, g_block_reverse_in_place_help);
s7_define_safe_function(sc,"block?", g_is_block, 1, 0, false, g_is_block_help);
s7_define_safe_function_star(sc,"blocks", g_blocks,"(frequency 4) (scaler 1)","test for function*");
s7_define_safe_function(sc,"blocks-are-morally-equal", g_blocks_are_morally_equal, 2, 0, false, g_blocks_are_morally_equal_help);
g_block_methods = s7_eval_c_string(sc,"(openlet (immutable! (inlet 'float-vector? (lambda (p) #t) \
'vector-dimensions (lambda (p) (list (length p))) \
'empty (lambda (p) (zero? (length p))) \
'subsequence subblock \
'append block-append \
'morally-equal? blocks-are-morally-equal \
'reverse! block-reverse!)))");
s7_gc_protect(sc, g_block_methods);

g_simple_block_type = s7_make_c_type(sc,"<simple-block>");
s7_define_safe_function(sc,"make-simple-block", g_make_simple_block, 1, 0, false, g_make_simple_block_help);
s7_c_type_set_free(sc, g_simple_block_type, g_block_free);
s7_c_type_set_mark(sc, g_simple_block_type, g_block_mark);
s7_c_type_set_length(sc, g_simple_block_type, g_block_length);
s7_c_type_set_ref(sc, g_simple_block_type, g_block_ref);
s7_c_type_set_set(sc, g_simple_block_type, g_block_set);
s7_define_safe_function(sc,"simple-block?", g_is_simple_block, 1, 0, false, g_is_simple_block_help);

g_c_tag_type = s7_make_c_type(sc,"<c-tag>");
s7_define_safe_function(sc,"make-c-tag", g_make_c_tag, 0, 0, false,"no help here");
s7_c_type_set_free(sc, g_c_tag_type, g_c_tag_free);

g_cycle_type = s7_make_c_type(sc,"<cycle>");
s7_define_safe_function(sc,"make-cycle", g_make_cycle, 1, 0, false,"no help here");
s7_define_safe_function(sc,"<cycle>", g_make_cycle, 1, 0, false,"no help here"); /* for print readably */
s7_c_type_set_mark(sc, g_cycle_type, g_cycle_mark);
s7_c_type_set_free(sc, g_cycle_type, g_cycle_free);
s7_c_type_set_to_list(sc, g_cycle_type, g_cycle_to_list);
s7_c_type_set_set(sc, g_cycle_type, g_cycle_implicit_set);
s7_define_safe_function(sc,"cycle-ref", g_cycle_ref, 1, 0, false,"no help here");
s7_define_safe_function(sc,"cycle-set!", g_cycle_set, 2, 0, false,"no help here");

s7_define_safe_function(sc,"function-open-output", fout_open, 0, 0, false,"");
s7_define_safe_function(sc,"function-get-output", fout_get_output, 1, 0, false,"");
s7_define_safe_function(sc,"function-close-output", fout_close, 1, 0, false,"");
s7_define_safe_function(sc,"function-open-input", fin_open, 1, 0, false,"");

s7_define_safe_function(sc,"hash_heq", g_heq, 2, 0, false,"hash-table test");
s7_define_safe_function(sc,"hash_hloc", g_hloc, 1, 0, false,"hash-table test");

s7_define_safe_function(sc,"cf10", g_cf10, 1, 0, false,"");
s7_define_safe_function(sc,"cf11", g_cf11, 1, 0, false,"");
s7_define_safe_function(sc,"cs11", g_cs11, 1, 0, false,"");
s7_define_safe_function(sc,"rs11", g_rs11, 1, 0, false,"");

s7_define_safe_function(sc,"cf20", g_cf20, 2, 0, false,"");
s7_define_safe_function(sc,"cf21", g_cf21, 2, 0, false,"");
s7_define_safe_function(sc,"cf22", g_cf22, 2, 0, false,"");

s7_define_safe_function(sc,"cf30", g_cf30, 3, 0, false,"");
s7_define_safe_function(sc,"cf31", g_cf31, 3, 0, false,"");
s7_define_safe_function(sc,"cf32", g_cf32, 3, 0, false,"");
s7_define_safe_function(sc,"cf33", g_cf33, 3, 0, false,"");

s7_define_safe_function(sc,"cf41", g_cf41, 4, 0, false,"");
s7_define_safe_function(sc,"cf42", g_cf42, 4, 0, false,"");
s7_define_safe_function(sc,"cf43", g_cf43, 4, 0, false,"");
s7_define_safe_function(sc,"cf44", g_cf44, 4, 0, false,"");

s7_define_safe_function(sc,"cf51", g_cf51, 5, 0, false,"");

s7_define_function(sc,"sload",  sload,        1, 1, false,"test s7_load");
s7_define_function(sc,"scall",  scall,        2, 0, false,"test s7_call");
s7_define_function(sc,"sread",  sread,        0, 1, false,"test s7_read");
s7_define_function(sc,"swind",  swind,        3, 0, false,"test s7_dynamic_wind");
s7_define_function(sc,"seval",  seval,        1, 1, false,"test s7_eval");
s7_define_function(sc,"sevalstr",  sevalstr,  1, 1, false,"test s7_eval_c_string");
}


int main()
{
	return 0;
}