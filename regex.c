#include "regex.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *strip_new_line(char * buff, const char *str)
{
	strcpy(buff,str);
	buff[strlen(buff)-1]='\0';
	return buff;
}

int regex_is_valid(const char *regex_string)
{
	int i, op=0;
	for(i=0;i<strlen(regex_string);i++)
	{
		if(regex_string[i]=='(')
		{
			if(op)
				return 0;
			op=1;
		}
		else if(regex_string[i]==')')
		{
			if(!op)
				return 0;
			op=0;
			if(i+1<strlen(regex_string))
			{
				char operator = regex_string[i+1];
				if(operator!='*' && operator!='+' && operator!='?')
					return 0;
			}else
				return 0;
		}
	}
	if(op)
		return 0;
	return 1;
}

tRegex *regex_new(const char *regex_string)
{
	if(!regex_is_valid(regex_string))
	{
		if(regex_log_func)
		{
			regex_log_func("\tregex_log: Invalid regular expression.\n");
		}	
		return NULL;
	}
	//
	tRegex *regex = malloc(sizeof(tRegex));
	regex->string = malloc(strlen(regex_string)+1);
	strcpy(regex->string, regex_string);
	regex->string[strlen(regex_string)]='\0';
	regex->incond=0;
	regex->cond_count=0;
	regex->cond_list=NULL;
	regex->base=malloc(1);
	regex->base[0]='\0';
	//
	regex->case_list = NULL;
	regex->tentativas=0;
	regex->break_point=0;
	//
	int i, base_index=0, base_size=1;
	for(i=0;i<strlen(regex_string);i++)
	{
		char c = regex_string[i];
		if(c=='(')
		{
			tCond *new_cond = malloc(sizeof(tCond));
			new_cond->simbolos_count=0;
			new_cond->simbolos=NULL;
			new_cond->base_index=base_index;
			//
			i++;
			while(regex_string[i]!=')')
			{
				
				char *p = realloc(new_cond->simbolos, new_cond->simbolos_count+1);
				new_cond->simbolos=p;
				new_cond->simbolos[new_cond->simbolos_count]=regex_string[i];
				new_cond->simbolos_count++;	
				i++;
			}
			i++;
			new_cond->operador = regex_string[i];
			//
			tCond **p_cond = realloc(regex->cond_list, sizeof(tCond)*(regex->cond_count+1));
			regex->cond_list = p_cond;
			regex->cond_list[regex->cond_count] = new_cond;
			regex->cond_count++;
			c = '$';
		}else{
			regex->incond++;
		}
		char *p_base = realloc(regex->base, base_size+1 );
		regex->base = p_base;
		regex->base[base_index] = c;
		regex->base[base_index+1] = '\0';
		base_size++;
		base_index++;
	}
	regex->case_list = malloc(sizeof(int)*regex->cond_count);
	for(i=0;i<regex->cond_count;i++)
	{
		regex_go_min_operator(regex, i);
	}
	return regex;
}

int regex_get_size_max(tRegex *regex)
{
	int i;
	int tam = regex->incond;
	for(i=0;i<regex->cond_count;i++)
		switch(regex->cond_list[i]->operador)
		{
			case '+':
			case '*':
				return -1;
			case '?':
				tam+=regex->cond_list[i]->simbolos_count;
		}
	return tam;
}

void regex_go_min_operator(tRegex *regex, int cond_index)
{
	switch(regex->cond_list[cond_index]->operador)
	{
		case '+':
			regex->case_list[cond_index]=1;
			break;
		case '?':
		case '*':
		default:
			regex->case_list[cond_index]=0;
			break;
	}
}

int regex_total_simb_cond(tRegex *regex)
{
	int total_simbolos = 0, i;
	for(i=0;i<regex->cond_count;i++)
	{
		total_simbolos += ( regex->case_list[i]*regex->cond_list[i]->simbolos_count );
	}
	return total_simbolos;
}

int regex_increment_case(tRegex *regex, int index)
{
	switch(regex->cond_list[index]->operador)
	{
		case '+':
		case '*':
			regex->case_list[index]++;
			break;
		case '?':
			regex->case_list[index]++;
			if(regex->case_list[index]>1)
				return 1;
			break;
	}
	return 0;
}

void regex_increment_cond(tRegex *regex, int dist)
{
	int i=0;
	while(1)
	{
		while(regex_increment_case(regex,i) || regex_total_simb_cond(regex)>dist )
		{
			regex_go_min_operator(regex, i);
			if(i+1>=regex->cond_count)
			{
				regex->break_point=1;
				return;
			}
			i++;
		}
		i=0;
		if(regex_total_simb_cond(regex)==dist)
			break;
	}	
}

char *regex_next_try(tRegex *regex, int match_size)
{
	int i;
	int cond_index=0;
	int dist = match_size - regex->incond;
	int total_simbolos = regex_total_simb_cond(regex);
	//
	char *test = NULL;
	int size=0;
	//
	if(dist < total_simbolos || (regex_get_size_max(regex)>-1 && regex_get_size_max(regex)<match_size) || regex->break_point )
	{
		test = NULL;
	}
	else if(!regex->cond_count)
	{
		if(!regex->tentativas)
		{
			test = malloc(strlen(regex->base));
			strcpy(test,regex->base);
		}
	}
	else
	{
		if(total_simbolos<dist)
		{
			regex_increment_cond(regex, dist);
			total_simbolos = regex_total_simb_cond(regex);
		}
		test = (char *)malloc(regex->incond+total_simbolos+1);
		int test_pos=0, j=0, k=0;
		for(i=0;i<strlen(regex->base);i++)
		{
			if(regex->base[i]=='$')
			{
				for(j=0;j<regex->case_list[cond_index];j++)
				{
					for(k=0;k<regex->cond_list[cond_index]->simbolos_count;k++)
						test[test_pos++]=regex->cond_list[cond_index]->simbolos[k];
				}
				cond_index++;
			}else
				test[test_pos++]=regex->base[i];
		}
		test[test_pos]='\0';
		regex_increment_cond(regex, dist);
	}

	regex->tentativas++;
	return test;
}

void regex_destroy(tRegex *regex)
{
	if(regex->string)
		free(regex->string);
	int i;
	for(i=0;i<regex->cond_count;i++)
	{
		regex_destroy_cond(regex->cond_list[i]);
	}
	if(regex->cond_list)
		free(regex->cond_list);
	if(regex->base)
		free(regex->base);
	if(regex->case_list)
		free(regex->case_list);
	if(regex)
		free(regex);
}

void regex_destroy_cond(tCond *cond)
{
	free(cond->simbolos);
	free(cond);
}

void regex_prepare(tRegex *regex)
{
	regex->tentativas=0;
	int i;
	for(i=0;i<regex->cond_count;i++)
	{
		regex_go_min_operator(regex, i);
	}
	regex->break_point=0;
}

int regex_compare(const char *test, const char *word)
{
	int i = 0, res = 1;
	for(i=0;i<strlen(word);i++)
	{
		res = res && (test[i]=='.' || word[i]==test[i]);
		if(!res)
			break;
	}
	return res;
}

int regex_check(const char *regex_string, const char *word)
{
	int res = 0;
	//
	tRegex *regex = regex_new(regex_string);
	if(!regex)
		return 0;
	res = regex_check_re(regex, word);
	regex_destroy(regex);
	return res;
}

int regex_check_re(tRegex *regex, const char *word)
{
	int res = 0;
	if(!regex)
		return 0;
	regex_prepare(regex);
	
	char *try=NULL;
	while(1)
	{
		try = regex_next_try(regex, strlen(word));
		if(!try)
			break;
		//printf("tentativas: %d\n", regex->tentativas);
		//printf("try: %s (%d ? %d)\n", try, strlen(try), strlen(word));
		//printf("total: %d\n",regex_total_simb_cond(regex));
		//
		res = regex_compare(try, word);
		//
		if(regex_log_func)
		{
			char msg[512], buff[512];
			strip_new_line(buff,try);
			sprintf(msg,"\tregex_log: trying to match: %s: %s\n", buff, (res ? "MATCH!" : "FAIL!"));
			regex_log_func(msg);
		}
		//
		free(try);
		//
		if(res)
		{
			if(regex_log_func)
			{
				char msg[512];
				sprintf(msg,"\tregex_log: Found a match after %d attempt(s). Stopping...\n",regex->tentativas);
				regex_log_func(msg);
			}
			break;
		}
	}
	return res;
}
