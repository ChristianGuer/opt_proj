#ifndef QSERVER_H
#define QSERVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>


#define	QLEN			5
#define	BBUF			2304
#define	LBUF			64
#define NAMELEN		128
#define MAXQ			128

typedef struct question
{
	char	qtext[BBUF];
	char	answer[LBUF];
	char	winner[LBUF];
	int	respondents;
} ques_t;

typedef struct player{
	char name[NAMELEN];
	int score;
	int socket;
	int answered;
}player_t;

int start_quiz(int sock);

// Server messages
#define WADMIN	"QS|ADMIN\r\n"
#define WAIT	"WAIT\r\n"
#define WJOIN	"QS|JOIN\r\n"
#define FULL	"QS|FULL\r\n"

// Client messages
#define JOIN	"JOIN"
#define GROUP	"GROUP"
#define CRLF            "\r\n"


#endif