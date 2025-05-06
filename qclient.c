#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define ADMIN		"QS|ADMIN\r\n"
#define JOIN		"QS|JOIN\r\n"
#define FULL		"QS|FULL\r\n"
#define WAIT		"WAIT\r\n"
#define CRLF            "\r\n"
#define BUFSIZE		4096
#define SHORTSIZE	128

int	gotqinbuf = 0;

int connectsock( char *host, char *service, char *protocol );

void unexpected_message( char *buf )
{
	printf( "The server sent an unexpected message %s.\n", buf );
	exit(-1);
}

void process_results( int sock, char *buf )
{
	char *t1, *t2, *t3;
	printf( "The final scores are: \n" );
	for (;;)
	{
		t2 = strtok( NULL, "|\r\n" );
		t3 = strtok( NULL, "|\r\n" );
		if ( t2 == NULL || strlen(t2) == 0 )
			break;
		printf( "Player - %s: %s\n", t2, t3 );
	}
	exit(0);
}

void process_a_question( int sock, char *buf )
{
	char *t1, *t2, *t3;
	int len;
	t1 = strtok( buf, "|" );
	if ( strcmp(t1, "QUES") != 0 )
	{
		//printf( "This is t1: *%s*\n", t1 );
		if ( strcmp(t1, "RESULTS") == 0 )
			process_results(sock, buf);
		else	
			unexpected_message("Should be a question.");
	}
	t2 = strtok( NULL, "|" );
	t3 = strtok( NULL, "|" );
	len = atoi(t2);

	// If less than, I should actually keep reading
	if (strlen(t3) != len)
		unexpected_message("The text size does not match.");
	printf( "Please answer this question: \n%s ", t3 );
}

void read_from_server( int sock, char *buf )
{
	int cc;

	if ( (cc = read( sock, buf, BUFSIZE )) <= 0 )
        {
        	printf( "The server has gone.\n" );
		exit(-1);
        }
	buf[cc] = '\0';
	//printf( "DEBUG: Server sent: %s", buf );
}

void write_to_server( int sock, char *buf, int len )
{
	// Send to the server
	if ( write( sock, buf, len ) < 0 )
	{
		fprintf( stderr, "client write: %s\n", strerror(errno) );
		exit( -1 );
	}	
}

void process_a_winner( int sock, char *buf )
{
	char *t1, *t2, *t3 = NULL;
	int i;

	//printf( "processing the winner message %s\n", buf );
	
	if ( strncmp(buf, "WIN", 3) != 0 )
		unexpected_message("Should be a winner announcement.");
	if ( buf[4] == '\r' || buf[4] == '\n' )
	{
		printf( "No one got the correct answer.\n" );
		t3 = buf+4;
	}
	else
	{
		t1 = strtok( buf, "|" );
		t2 = strtok( NULL, "\r\n" );
		t3 = strtok( NULL, "" );
		printf( "%s got the correct answer the fastest.\n", t2 );
	}
	if (t3 != NULL && strlen(t3) > 2)
	{
		//printf("This is t3: *%s*\n", t3 );
		while ( t3[0] == '\n' || t3[0] == '\r' )
			t3++;
		for ( i = 0; t3[i] != '\0'; i++ )
			buf[i] = t3[i];
		buf[i] = '\0';
		gotqinbuf = 1;
	}
}

int remove_newline( char *buf, int len )
{
        while ( buf[len-1] == '\n' || buf[len-1] == '\r' )
                len --;
        buf[len] = '\0';
        return len;
}


/*
**	Client
*/
int
main( int argc, char *argv[] )
{
	char		buf[BUFSIZE];
	char		name[SHORTSIZE];
	char		groupSize[SHORTSIZE];
	char		*service;		
	char		*host = "localhost";
	int		cc;
	int		csock;
	int		i, j;
	
	switch( argc ) 
	{
		case    2:
			service = argv[1];
			break;
		case    3:
			host = argv[1];
			service = argv[2];
			break;
		default:
			fprintf( stderr, "usage: client [host] port\n" );
			exit(-1);
	}

	//	Create the socket to the server
	if ( ( csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}

	printf( "Welcome to the quiz/game server\n" );
	fflush( stdout );

	read_from_server( csock, buf ); //////

	if ( strncmp(buf, ADMIN, strlen(ADMIN)) == 0 )
	{
		printf( "You are the group admin. What's your name?  " );
		fgets( name, SHORTSIZE, stdin );
		remove_newline(name, strlen(name));

		printf( "How big do you want your group to be?  " );
		fgets( groupSize, SHORTSIZE, stdin );
		remove_newline(groupSize, strlen(groupSize));

		sprintf( buf, "GROUP|%s|%s%s", name, groupSize, CRLF );
	}
	else if ( strncmp(buf, JOIN, strlen(JOIN)) == 0 )
	{
		printf( "You are joining a group. What's your name?  " );
		fgets( name, SHORTSIZE, stdin );
		remove_newline(name, strlen(name));

		sprintf( buf, "JOIN|%s%s", name, CRLF );
	}
	else if ( strncmp(buf, FULL, strlen(FULL)) == 0 )
	{
		printf( "Sorry but the group is full." );
		close(csock);
		exit(0);
	}
	else
		unexpected_message(buf);

	// Send to the server
	write_to_server( csock, buf, strlen(buf) );

	// The server will say WAIT but maybe also start the quiz
	read_from_server( csock, buf );
	if ( strncmp(buf, WAIT, strlen(WAIT)) == 0 )
	{
		printf( "Please wait for your activity to start.\n" );
		// In case the quiz started already
		if (strlen(buf) > strlen(WAIT))
		{
			for ( i = 0, j = strlen(WAIT); buf[j] != '\0'; i++, j++ )
				buf[i] = buf[j];
			buf[i] = '\0';
			//printf( "HELLO %s", buf );
			gotqinbuf = 1;
		}
	}
	else
		unexpected_message(buf);

	// Taking the quiz
	for (;;)
	{
		if (!gotqinbuf)
		{
			read_from_server( csock, buf );
		}
		gotqinbuf = 0;

		process_a_question(csock, buf);
		// get the answer
		fgets( name, SHORTSIZE, stdin );
		remove_newline(name, strlen(name));
		sprintf( buf, "ANS|%s%s", name, CRLF );
		// Send to the server
		write_to_server( csock, buf, strlen(buf) );

		// Get the result
		read_from_server( csock, buf );
		process_a_winner( csock, buf );
	}
	close( csock );

}

	



	


