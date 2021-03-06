#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "s7.h"

/* define *listener-prompt* in scheme, add two accessors for C get/set */

static const char *listener_prompt(s7_scheme *sc)
{
	return(s7_string(s7_name_to_value(sc, "*listener-prompt*")));
}

static void set_listener_prompt(s7_scheme *sc, const char *new_prompt)
{
	s7_symbol_set_value(sc, s7_make_symbol(sc, "*listener-prompt*"), s7_make_string(sc, new_prompt));
}

/* now add a new type, a struct named "dax" with two fields, a real "x" and a list "data" */
/*   since the data field is an s7 object, we'll need to mark it to protect it from the GC */

typedef struct {
	s7_double x;
	s7_pointer data;
} dax;

static s7_pointer dax_to_string(s7_scheme *sc, s7_pointer args)
{
	char *data_str, *str;
	s7_pointer result;
	int data_str_len;
	dax *o = (dax *)s7_c_object_value(s7_car(args));
	data_str = s7_object_to_c_string(sc, o->data);
	data_str_len = strlen(data_str);
	str = (char *)calloc(data_str_len + 32, sizeof(char));
	snprintf(str, data_str_len + 32, "\"<dax %.3f %s>\"", o->x, data_str);
	result = s7_make_string(sc, str);
	free(str);
	return(result);
}

static void free_dax(void *val)
{
	if (val) free(val);
}

static bool equal_dax(void *val1, void *val2)
{
	return(val1 == val2);
}

static void mark_dax(void *val)
{
	dax *o = (dax *)val;
	if (o) s7_mark(o->data);
}

static int dax_type_tag = 0;

static s7_pointer make_dax(s7_scheme *sc, s7_pointer args)
{
	dax *o;
	o = (dax *)malloc(sizeof(dax));
	o->x = s7_real(s7_car(args));
	if (s7_cdr(args) != s7_nil(sc))
		o->data = s7_cadr(args);
	else o->data = s7_nil(sc);
	return(s7_make_c_object(sc, dax_type_tag, (void *)o));
}

static s7_pointer is_dax(s7_scheme *sc, s7_pointer args)
{
	return(s7_make_boolean(sc,
		s7_is_c_object(s7_car(args)) &&
		s7_c_object_type(s7_car(args)) == dax_type_tag));
}

static s7_pointer dax_x(s7_scheme *sc, s7_pointer args)
{
	dax *o;
	o = (dax *)s7_c_object_value(s7_car(args));
	return(s7_make_real(sc, o->x));
}

static s7_pointer set_dax_x(s7_scheme *sc, s7_pointer args)
{
	dax *o;
	o = (dax *)s7_c_object_value(s7_car(args));
	o->x = s7_real(s7_cadr(args));
	return(s7_cadr(args));
}

static s7_pointer dax_data(s7_scheme *sc, s7_pointer args)
{
	dax *o;
	o = (dax *)s7_c_object_value(s7_car(args));
	return(o->data);
}

static s7_pointer set_dax_data(s7_scheme *sc, s7_pointer args)
{
	dax *o;
	o = (dax *)s7_c_object_value(s7_car(args));
	o->data = s7_cadr(args);
	return(o->data);
}

int main(int argc, char **argv)
{
	s7_scheme *s7;
	char buffer[512];
	char response[1024];

	s7 = s7_init();


	s7_define_variable(s7, "*listener-prompt*", s7_make_string(s7, ">"));

	dax_type_tag = s7_make_c_type(s7, "dax");
	s7_c_type_set_free(s7, dax_type_tag, free_dax);
	s7_c_type_set_equal(s7, dax_type_tag, equal_dax);
	s7_c_type_set_mark(s7, dax_type_tag, mark_dax);
	s7_c_type_set_to_string(s7, dax_type_tag, dax_to_string);

	s7_define_function(s7, "make-dax", make_dax, 2, 0, false, "(make-dax x data) makes a new dax");
	s7_define_function(s7, "dax?", is_dax, 1, 0, false, "(dax? anything) returns #t if its argument is a dax object");

	s7_define_variable(s7, "dax-x",
		s7_dilambda(s7, "dax-x", dax_x, 1, 0, set_dax_x, 2, 0, "dax x field"));

	s7_define_variable(s7, "dax-data",
		s7_dilambda(s7, "dax-data", dax_data, 1, 0, set_dax_data, 2, 0, "dax data field"));

	while (1)
	{
		fprintf(stdout, "\n%s ", listener_prompt(s7));
		fgets(buffer, 512, stdin);
		if ((buffer[0] != '\n') ||
			(strlen(buffer) > 1))
		{
			snprintf(response, 1024, "(write %s)", buffer);
			s7_eval_c_string(s7, response); /* evaluate input and write the result */
		}
	}
}
