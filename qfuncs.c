#include "qserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int remove_newline( char *buf, int len )
{
	while ( buf[len-1] == '\n' || buf[len-1] == '\r' )
		len --;	
	buf[len] = '\0';
	return len;
}


void reset_questions( ques_t *q[] )
{
	int i = 0;
	
	while ( q[i] != NULL )
	{
		q[i]->respondents = 0;
		strcpy( q[i]->winner, "" );
		i++;
	}
}


void read_questions( char *qfile, ques_t *q[] )
{
	char	qbuf[BBUF];
	FILE 	*fp;
	int	pos, nq, ans, len, qn;

	if ( (fp = fopen( qfile, "r" )) == NULL )
	{
		printf( "Cannot open question file %s.\n", qfile );
		exit(-1);
	}

	qn = 0;
	nq = 1;		// new question
	ans = 0;
	while ( fgets( qbuf, BBUF, fp ) != NULL )
	{
		if ( nq )
		{
			q[qn] = (ques_t *) malloc( sizeof(ques_t) );
			pos = 0;
			nq = 0;
		}
		len = strlen(qbuf);
		// printf( "The string is *%s* %d.\n", qbuf, len );
		if ( ans == 0 )
		{
			if ( len == 1)
				ans = 1;
			else
			{
				strcpy( q[qn]->qtext+pos, qbuf );
				pos += strlen(qbuf);
			}
		}
		else
		{
			if ( len == 1 )
			{
				ans = 0;
				nq = 1;
				qn++;
			}
			else
			{
				remove_newline( qbuf, strlen(qbuf) );
				strcpy( q[qn]->answer, qbuf );
			}
		}
	}	
	printf( "The number of questions is %d.\n", qn );
	q[qn] = NULL;

}

