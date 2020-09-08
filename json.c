//Copyright 2020 airbrett
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of this software
//and associated documentation files (the "Software"), to deal in the Software without restriction,
//including without limitation the rights to use, copy, modify, merge, publish, distribute,
//sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all copies or
//substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "json.h"

#include <stdlib.h>
#include <stdbool.h>

#define SKIP_WHITESPACE(str) \
while (*(str) == ' ' || *(str) == '\t' || *(str) == '\r' || *(str) == '\n')\
	(str)++;

static char* scan_val(char* json)
{
	bool done = false;

	while (!done)
	{
		switch (*json)
		{
		case ',':
		case '}':
			done = true;
			break;
		case '\\':
			json++;
			break;
		}

		json++;
	}

	return json;
}

static char* scan_str(char* json)
{
	bool done = false;

	if (*json == '\"')
	{
		json++;

		while (!done)
		{
			switch (*json)
			{
			case '\"':
				done = true;
				break;
			case '\\':
				json++;
				break;
			}

			json++;
		}
	}

	return json;
}

static char* scan_obj(char* json)
{
	int Depth = 0;

	if (*json == '{')
	{
		Depth++;
		json++;

		while (true)
		{
			switch (*json)
			{
			case '{':
				Depth++;
				break;
			case '}':
				Depth--;
				break;
			case '\\':
				json++;
				break;
			}

			if (Depth == 0)
				break;

			json++;
		}
	}

	return json;
}

static char* scan_array(char* json)
{
	int Depth = 0;

	if (*json == '[')
	{
		Depth++;
		json++;

		while (true)
		{
			switch (*json)
			{
			case '[':
				Depth++;
				break;
			case ']':
				Depth--;
				break;
			case '\\':
				json++;
				break;
			}

			if (Depth == 0)
				break;

			json++;
		}
	}

	return json;
}

static char* parse_value(char* json, unsigned char* vtype, char** vbegin, int* vlen)
{
	if (*json == '\"')
	{
		*vtype = JSON_TYPE_STR;
		*vbegin = json + 1;//clip "
		json = scan_str(json);
		*vlen = json - *vbegin - 1;//clip "
		json++;
	}
	else if (*json == '{')
	{
		*vtype = JSON_TYPE_OBJ;
		*vbegin = json;
		json = scan_obj(json);
		*vlen = json - *vbegin + 1;
		json++;
	}
	else if (*json == '[')
	{
		*vtype = JSON_TYPE_ARR;
		*vbegin = json;
		json = scan_array(json);
		*vlen = json - *vbegin + 1;
		json++;
	}
	else
	{
		*vtype = JSON_TYPE_STR;
		*vbegin = json;
		json = scan_val(json);
		*vlen = json - *vbegin - 1;
	}

	return json;
}

char json_obj(char** state, char* json, unsigned char* v1type, char** v1begin, int* v1len, unsigned char* v2type, char** v2begin, int* v2len)
{
	if (json != NULL)
	{
		*state = json;
		SKIP_WHITESPACE(*state);

		if (**state != '{')
			return -1;

		(*state)++;
	}

	if (**state == '}')
		return 0;

	SKIP_WHITESPACE(*state);

	if (**state == '{')
	{
		*state = parse_value(*state, v1type, v1begin, v1len);
		*v2type = JSON_TYPE_NOTHING;
	}
	else if (**state == '\"')
	{
		//name
		*v1type = JSON_TYPE_STR;
		*v1begin = *state + 1;
		*state = scan_str(*state);
		*v1len = *state - *v1begin - 1;
		(*state)++;

		//value
		while (**state == ' ' || **state == '\t' || **state == '\r' || **state == '\n' || **state == ':')\
			(*state)++;

		*state = parse_value(*state, v2type, v2begin, v2len);
	}
	else
	{
		return -1;
	}

	SKIP_WHITESPACE(*state);

	if (**state == ',')
		(*state)++;

	return 1;
}

char json_arr(char** state, char* json, unsigned char* vtype, char** vbegin, int* vlen)
{
	if (json != NULL)
	{
		*state = json;
		SKIP_WHITESPACE(*state);

		if (**state != '[')
			return -1;

		(*state)++;
	}

	if (**state == ']')
		return 0;

	SKIP_WHITESPACE(*state);

	*state = parse_value(*state, vtype, vbegin, vlen);

	SKIP_WHITESPACE(*state);

	if (**state == ',')
		(*state)++;

	return 1;
}
