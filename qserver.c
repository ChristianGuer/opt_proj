#include <stdio.h>
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
#include "passivesock.c"
#include "qserver.h"
#include "qfuncs.h"
#include <bits/pthreadtypes.h>
// coppy new .h file
//make array of struct that hold a players name and score
// in quiz mutex lock, read the first message from the ADMIN and set max clients 

#define	BUFSIZE			4096
#define SHORTSIZE	128
#define MAX_SOCKETS	1024

sem_t sem_spots_available;
int groupSize = 0, clients = 0;
int initialized = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
player_t *players_list[MAX_SOCKETS]; 
ques_t *questions[MAXQ];

int passivesock( char *service, char *protocol, int qlen, int *rport );
int start_quiz(int ssock);
void showScores(int ssock);
void exit_player(int ssock);


void exit_player(int ssock){
    pthread_mutex_lock(&lock);
    clients--;
    printf("player:%d remain:%d.\n", ssock, clients);
    if(clients == 0){
        initialized = 0;
        for(int i = 0; i < MAXQ; i++){
            if(questions[i] != NULL){
                questions[i]->respondents = 0;
                memset(questions[i]->winner, 0, sizeof(questions[i]->winner));
            }
        }
        pthread_mutex_unlock(&lock);
        sem_destroy(&sem_spots_available);
    }else{
        pthread_mutex_unlock(&lock);
    }
    if(players_list[ssock] != NULL){
        free(players_list[ssock]);
        players_list[ssock] = NULL;
    }
    close(ssock);
    //free player info , reset respondnets, and winner
}


void * quiz( void *s)
{
    //new threaded socket to client now executing quiz initialization
    char buf[BUFSIZE];
    player_t *new_player = malloc(sizeof(player_t));
    // ssock wont be changed by another thread bc it has been malloc'd mem 
    int ssock = *(int*) s;
    int cc;
    free(s);
    pthread_mutex_lock(&lock);
    if (initialized == 0)
    {  
        initialized = -1;
        pthread_mutex_unlock(&lock);

        if(write(ssock, WADMIN, strlen(WADMIN)) <= 0){
            printf("Admin disconnected before setup.\n");
            close(ssock);
            free(new_player);
            pthread_exit(NULL);
        }
        cc = read(ssock, buf, BUFSIZE - 1);

        if (cc <= 0) {
            printf("Admin disconnected before setup.\n");
            close(ssock);
            free(new_player);
            pthread_exit(NULL);
        }

        buf[cc] = '\0';

        char *token = strtok(buf, "|");
            
        if (token && strcmp(token, "GROUP") == 0) {
                // 1. Get the name
            token = strtok(NULL, "|");
            if (token != NULL) {
                strncpy(new_player->name, token, NAMELEN - 1);
                new_player->name[NAMELEN-1] = '\0';  // Always safe-terminate string
            }
            token = strtok(NULL, "\r\n"); // read until CRLF
            if (token != NULL) {
                groupSize = atoi(token);
            }
            
            new_player->score = 0;
            new_player->answered = 1;
            new_player->socket = ssock;
            write(ssock, WAIT, strlen(WAIT));
        } else {
            printf("Invalid ADMIN message received.\n");
            close(ssock);
            free(new_player);
            pthread_exit(NULL);
        }


        pthread_mutex_lock(&lock);
        initialized = 1;
        sem_init(&sem_spots_available, 0, groupSize);
        sem_wait(&sem_spots_available); // remove from the semaphore
        pthread_mutex_unlock(&lock);
        //exit and return to normal

    } else if (initialized == -1) {
        //TOO EARLY CLIENT WIlL BE DISCONNECTED
        pthread_mutex_unlock(&lock);
        printf("Client %d disconnected.\n", ssock);
        write(ssock, FULL, strlen(FULL));
        close(ssock);
        free(new_player);
        pthread_exit(NULL);
    }else{
        //ADMIN CONNECTED SUCCES BUT CHECK IF GROUP IS FULL
        if(sem_trywait(&sem_spots_available) == -1 ||initialized == 2){
            //Too many CLIENTS
            pthread_mutex_unlock(&lock);
            printf("Client %d disconnected bc TOO full.\n", ssock);
            write(ssock, FULL, strlen(FULL));
            close(ssock);
            free(new_player);
            pthread_exit(NULL);
            //maybe
        }else{
            //NOT FULL, MAKE NEW PLAYER
            //sem_post(&sem_clients);
            pthread_mutex_unlock(&lock);
            if(write(ssock, WJOIN, strlen(WJOIN)) <= 0){
                printf("Player disconnected before setup.\n");
                sem_post(&sem_spots_available); // adds back to the semaphore
                close(ssock);
                free(new_player);
                pthread_exit(NULL);
            }

            cc = read(ssock, buf, BUFSIZE - 1);
            if (cc <= 0) {
                printf("Player disconnected before setup.\n");
                sem_post(&sem_spots_available); // adds back to the semaphore
                close(ssock);
                free(new_player);
                pthread_exit(NULL);
            }

            buf[cc] = '\0';
            char *token = strtok(buf, "|");
                
            if (token && strcmp(token, "JOIN") == 0) {
                    // 1. Get the name
                token = strtok(NULL, "\r\n");
                if (token != NULL) {
                    strncpy(new_player->name, token, NAMELEN - 1);
                    new_player->name[NAMELEN-1] = '\0';  // Always safe-terminate string
                }else{
                    printf("Invalid JOIN message received.\n");
                    sem_post(&sem_spots_available);
                    close(ssock);
                    free(new_player);
                    pthread_exit(NULL);
                }
            }
            new_player->score = 0;
            new_player->answered = 1;
            new_player->socket = ssock;
            if(write(ssock, WAIT, strlen(WAIT)) <= 0){
                printf("Player disconnected before setup.\n");
                sem_post(&sem_spots_available); // adds back to the semaphore
                close(ssock);
                free(new_player);
                pthread_exit(NULL);
            }
        }
        
    }
    //Succesfully created a player now adding him to list
    pthread_mutex_lock(&lock);
    //maybe move this up to make sure the player is not null
    players_list[ssock] = new_player;
    clients++;
    if(clients >= groupSize){
        initialized = 2;
        printf("starting players: %d\n", clients);
        pthread_cond_broadcast(&start_cond); // adds back to the semaphore

    }
    while(clients < groupSize){
        printf("groupsize: %d\n", groupSize);
        pthread_cond_wait(&start_cond, &lock);
    }
    pthread_mutex_unlock(&lock);
    if(start_quiz(ssock) <= 0){
        //sem_wait(&sem_clients); // remove from the semaphore
        printf("Exiting Client Early %d\n", ssock);
        exit_player(ssock);
        pthread_mutex_lock(&lock);
        pthread_cond_broadcast(&start_cond);
        printf("Client %d broadcasets when they left.  clients=%d  responds:%d\n", ssock, clients, questions[1]->respondents);
        pthread_mutex_unlock(&lock);
    }else{
        showScores(ssock);
        //cond wall
        sleep(2);
        exit_player(ssock);
    }
    pthread_exit(NULL);

    //exit and return to normal
}
/*
*/
int
main( int argc, char *argv[] )
{
    char            *fileName;
	char			*service;
	struct sockaddr_in	fsin;
	int			alen = sizeof(fsin);
	int			msock;
	int			ssock;
	int			rport = 0;
    //make num clients semaphore
	switch (argc) 
	{
		case	1:
			// No args? let the OS choose a port and tell the user

			fprintf( stderr, "usage: server [file] [port]\n" );
			exit(-1);
			break;
		case	2:
			// User provides a port? then use it
            fileName = argv[1];
            rport = 1;
            break;
        case	3:
            fileName = argv[1];
			service = argv[2];
			break;
		default:
			fprintf( stderr, "usage: server [port]\n" );
			exit(-1);
	}

	// Call the function that sets main socket to listen for connections
	msock = passivesock( service, "tcp", QLEN, &rport );

	if (rport)
	{
		// Tell the user the selected port number
		printf( "server: port %d\n", rport );	
		fflush( stdout );
	}
    read_questions(fileName, questions);

	//while not max clients accepting stage. then move to questions
        for (;;)
        {
            int	ssock;
            pthread_t	thr;

            ssock = accept( msock, (struct sockaddr *)&fsin, &alen );
            if (ssock < 0)
            {
                fprintf( stderr, "accept: %s\n", strerror(errno) );
                break;
            }
            int *psock = malloc(sizeof(int));
            *psock = ssock;

            printf( "A client has arrived for Quizzing - serving on fd %d.\n", ssock );
            fflush( stdout );

            //creates new thread and sends it the socket to echo function
            pthread_create( &thr, NULL, quiz, psock);

        }
    
	pthread_exit(NULL);
}


int start_quiz(int ssock) {
    printf("Starting quiz for client %d.\n\n", ssock);
    char buf[BBUF];
    char answer[LBUF];
    char winner_msg[LBUF + 6];

    for (int i = 0; i < MAXQ; i++) {
        if (questions[i] == NULL) break;

        memset(winner_msg, 0, sizeof(winner_msg));
        memset(answer, 0, sizeof(answer));

        int size = strlen(questions[i]->qtext);
        int cc = snprintf(buf, sizeof(buf), "QUES|%d|%s", size, questions[i]->qtext);
        if (cc < 0 || cc >= sizeof(buf)) {
            printf("Error formatting question.\n");
            close(ssock);
            return -1;
        }

        if (write(ssock, buf, cc) <= 0) return -1;

        memset(buf, 0, sizeof(buf));
        cc = read(ssock, answer, LBUF - 1);
        if (cc <= 0) {
            printf("Client %d disconnected during question.\n", ssock);
            pthread_mutex_lock(&lock);
            pthread_cond_broadcast(&start_cond);  // Wake up others
            pthread_mutex_unlock(&lock);
            return -2;  // Don't count respondent
        }

        answer[cc] = '\0';

        pthread_mutex_lock(&lock);
        questions[i]->respondents++;
        printf("Responses: %d\n", questions[i]->respondents);

        if (strncmp(answer, "ANS|", 4) == 0) {
            char *token = strtok(answer, "|");
            token = strtok(NULL, "|");
            remove_newline(token, strlen(token));

            if (token && strncmp(token, "NOANS", 5) != 0) {
                if (strncmp(token, questions[i]->answer, strlen(questions[i]->answer)) == 0) {
                    printf("Player %d answered correctly.\n", ssock);
                    players_list[ssock]->score++;
                    if (questions[i]->winner[0] == '\0') {
                        // First correct answer
                        players_list[ssock]->score++;
                        strncat(questions[i]->winner, players_list[ssock]->name, LBUF - 1);
                    }
                } else {
                    players_list[ssock]->score--;
                }
            } else {
                printf("Player %d did not answer.\n", ssock);
            }
        } else {
            printf("Player %d gave invalid answer.\n", ssock);
        }

        // Always broadcast to ensure no thread gets stuck
        pthread_cond_broadcast(&start_cond);

        // Wait for all live clients to respond
        while (questions[i]->respondents < clients) {
            printf("sock:%d waiting, respondents: %d, clients: %d\n",
                   ssock, questions[i]->respondents, clients);
            pthread_cond_wait(&start_cond, &lock);
        }

        pthread_mutex_unlock(&lock);

        snprintf(winner_msg, sizeof(winner_msg), "WIN|%s%s", questions[i]->winner, CRLF);
        if (write(ssock, winner_msg, strlen(winner_msg)) <= 0) {
            printf("Client %d quit before receiving WIN.\n", ssock);
            return -1;
        }
    }

    return 1;
}

int compare_score(const void *a, const void *b) {
    player_t *playerA = *(player_t **)a;
    player_t *playerB = *(player_t **)b;
    return (playerB->score - playerA->score);
}

void showScores(int ssock){
    player_t *players_scores_sorted[groupSize];
    int active_players = 0;
    for(int i = 0; i < MAX_SOCKETS; i++){
        if(players_list[i] != NULL){
            players_scores_sorted[active_players] = players_list[i];
            printf("Player %d: %s, Score: %d\n", players_list[i]->socket, players_list[i]->name, players_list[i]->score);
            active_players++;
        }
    }
    qsort(players_scores_sorted, active_players, sizeof(player_t*), compare_score);
    char score_msg[BBUF];
    int bytes = 0;
    bytes += snprintf(score_msg, sizeof(score_msg) - bytes, "RESULTS");
    for(int i = 0; i < active_players; i++){
        bytes += snprintf(score_msg + bytes, sizeof(score_msg) - bytes, "|%s|%d", players_scores_sorted[i]->name, players_scores_sorted[i]->score);
    }
    strncat(score_msg, CRLF, sizeof(score_msg) - strlen(score_msg) - 1);
    if(write(ssock, score_msg, strlen(score_msg)) <= 0)
        return;
    printf("Scores sent to client %d: %s\n", ssock, score_msg);
}
//how do i know when a client discconnects is it with crtrl c or z but will the server pick itup from reads and write returning 0
// if no one gets it right do u want (NULL) to stay

/* todo:

Error: sometimes it displays next question for both somethimes it doesnt. 
I think when it happens its only to the window that asnwered second
*/

//comebacl and make sure each thread wipes its memory