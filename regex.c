#include "regex.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

tRegex *process_regex(const char *regex_string)
{
	tRegex *regex = malloc(sizeof(tRegex));
	regex->incond=0;
	regex->cond_count=0;
	regex->cond_list=NULL;
	regex->base=malloc(1);
	regex->base[0]='\0';
	//
	regex->case_list = NULL;
	regex->tentativas=0;
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
	regex->case_list = malloc(sizeof(int)*regex->incond);
	for(i=0;i<regex->cond_count;i++)
	{
		if(regex->cond_list[i]->operador=='+')
			regex->case_list[i]=1;
		else
			regex->case_list[i]=0;
	}
	return regex;
}

int total_simb_cond(tRegex *regex)
{
	int total_simbolos = 0, i;
	for(i=0;i<regex->cond_count;i++)
	{
		total_simbolos += ( regex->case_list[i]*regex->cond_list[i]->simbolos_count );
	}
	return total_simbolos;
}

void increment_cond(tRegex *regex, int dist)
{
	int i=0;
	while(1)
	{
		regex->case_list[i]++;
		if(total_simb_cond(regex)==dist)
			break;
		while(total_simb_cond(regex)>dist)
		{
			regex->case_list[i]=( regex->cond_list[i]->operador=='+' ? 1 : 0 );
			i++;
			regex->case_list[i]++;	
			if(i+1>=regex->cond_count)
				return;
		}
		i=0;
	}	
}

char *next_try(tRegex *regex, int match_size)
{
	int i;
	int cond_index=0;
	int dist = match_size - regex->incond;
	int total_simbolos = total_simb_cond(regex);
	//
	char *test = NULL;
	int size=0;
	//
	if(dist < total_simbolos)
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
			increment_cond(regex, dist);
			total_simbolos = total_simb_cond(regex);
		}
		test = (char *)malloc(regex->incond+total_simbolos);
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
		increment_cond(regex, dist);
	}

	regex->tentativas++;
	return test;
}

void regex_destroy(tRegex *regex)
{
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

int compare(const char *test, const char *word)
{
	int i = 0, res = 1;
	for(i=0;i<strlen(word);i++)
	{
		res = res && (word[i]==test[i] || test[i]=='.');
	}
	return res;
}

int check(const char *regex_string, const char *word)
{
	int res = 0;
	//
	tRegex *regex = process_regex(regex_string);
	char *try = next_try(regex, strlen(word));
	while(try)
	{
		res = compare(try, word);
		free(try);
		if(res)
			break;
		try = next_try(regex, strlen(word));
	}
	regex_destroy(regex);
	return res;
}

