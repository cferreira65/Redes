/**  
* svr_s.c - C socket server what handles multiple clients using threads  
* @authors  
*   Carlos Ferreira     11-10323
*   Carlos Martínez     11-10584
*   Dayana Rodrigues    10-10615
*/

#include<stdio.h>
#include<string.h>      /** strlen */
#include<stdlib.h>      /** strlen */
#include<sys/socket.h>
#include<arpa/inet.h>   /** inet_addr */
#include<unistd.h>      /** write */
#include<pthread.h>     /** for threading, link with lpthread */
#include <string.h>

char *file; /** log file name of the server */
FILE *fp;   /** log file */

void *connection_handler(void *);   /** The thread function. */
int patternFinder(char *m);         /** Function to find patterns */
int send_mail(char *m);

/**  
    * Struct for params of each thread. 
*/

struct ParamsThread
{
    char *ip;       /** Struct value for the IP address */
    int *sock;      /** Struct value for the thread ID */
};

/**  
    * Main function with the logic of the server. 
    * @return the code of exit.  
*/

int main(int argc , char *argv[])
{
    /** verifying the command line */
    if (argc != 5) {
        fprintf(stderr,"Uso: %s -l <puerto_srv_c> -b <archivo_bitácora>\n", argv[0]);
        exit(1);
    }

    char *flag_port;                /** flag port */
    char *flag_log_file;            /** flag for log file */
    file = argv[4];                 /** name of log file */
    flag_port = strdup("-l");       
    flag_log_file = strdup("-b");   

    int n, k;               /** aux variables for reading command line params */
    long port;              /** port */
    char *log_file_name;    /** name of log file */

    n = 1;
    k = 2;

    int i;

    /** readinng the params from comand line */
    for(i = 0 ; i < 2 ; i++){
        if (strcmp(argv[n],flag_port) == 0){
          port = strtol(argv[k],0,10);
        } else if (strcmp(argv[n],flag_log_file) == 0){
          log_file_name = strdup(argv[k]);
        } else {
          printf("ERROR: Argumento %s no reconocido.\n", argv[n]);
          exit(1);
        }
        n = n + 2;
        k = k + 2;
    }

    int socket_desc , client_sock , c , *new_sock;  /** sockets variables */
    struct sockaddr_in server , client;             /** struct for client and server info */


    
    socket_desc = socket(AF_INET , SOCK_STREAM , 0); /** create socket */
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    /** prepare the sockaddr_in structure */ 
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );
     
    
    /** binding */ 
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) 
    {
        
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    /** listen */
    listen(socket_desc , 3);
     
     /** accept and incoming connection */
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    
    /** While connection is accepted create a thread and assign the socket for beging connection */
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
        pthread_t sniffer_thread;                   /** thread */
        new_sock = malloc(1);                       /** socket */
        *new_sock = client_sock;                    /** listen */
        struct ParamsThread params;                 /** struct of thread params */
        params.ip = inet_ntoa(client.sin_addr);     /** IP address */
        params.sock = new_sock;                     /** socket ID */
         

        /** creating thread */
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , &params) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        puts("Handler assigned");
    }
    
    /** accept failed */
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
/**
 *  This will handle connection for each client.
 *  @params socket_desc: socket descriptor.
 */

void *connection_handler(void *socket_desc)
{
    
    /** get the socket descriptor */
    struct ParamsThread *params = socket_desc;
    int socke = *(int*)params->sock;

    /** variables for messages transmition */
    int read_size;
    char *message , client_message[2000];
    char * pch;
    
    int client_port;
    struct sockaddr_in client;

    printf("New ATM with id %lu connected!\n", pthread_self());

    /** receive a message from client with local port*/
    if ((read_size = recv(socke , client_message , 12 - 1 , 0)) > 0){
        client_message[read_size] = '\0';
        client_port = strtol(client_message, NULL, 10);
    }
    
    time_t currenttime = clock();
    time_t newtime;
    /** read messages from the client and reply */
    while( (read_size = recv(socke , client_message , 2000 - 1 , 0)) >= 0)
    {


        char res[500];

        
        if (read_size == 0){
        /** handle for lost connection */

            /*
                CLIENT DISCONNECTED
                There will be reconnection attemps every few seconds
            */

            /** close the socket */
            close(socke);                               
            /** create new socket */
            socke = socket(AF_INET , SOCK_STREAM , 0);  

            if (socke == -1)
            {
                printf("Could not create socket");
            }

            printf("Connection to %lu lost.\n", pthread_self());
            printf("Trying to reconnect...\n");

            /** init descriptor for communicate with client */
            time_t newtime = time(0);
            client.sin_addr.s_addr = inet_addr(params->ip);
            client.sin_family = AF_INET;
            client.sin_port = htons(client_port);

            /** try to reconnect with client */
            while (connect(socke, (struct sockaddr *)&client , sizeof(client)) < 0)
            {
                perror("Connect failed. Error");
                sleep(5);
                /** send alert if can't communicate for 5 min (300 sec)*/
                currenttime  = time(0);
                if(currenttime - newtime > 300)
                {
                    printf("Sending Communication Offline alert.\n");
                    /* sends mail notifying the communication offline */
                    send_mail(strdup("Communication Offline"));

                    newtime = time(0);
                
                    time_t rawtime;
                    struct tm * timeinfo;
                    time ( &rawtime );

                    timeinfo = localtime ( &rawtime );

                    char date[30];

                    /** prepare data and update the log file */
                    sprintf(date,"%d:%d:%d,%d-%d-%d",timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);

                    sprintf(res,"(%lu,%s,%s,%d,%s)\n",pthread_self(),date,params->ip,1,strdup("Communication Offline"));
                    
                    fp = fopen( file , "a" );                       /** open or create the log for write */
                    fwrite(res , sizeof(char) , strlen(res) , fp ); /** write */
                    fclose(fp);                                     /** close the log file */
                }
                
            }
            /** continue with the communication */
            printf("Reconnected to %lu\n", pthread_self());
            continue;    
        }

        /** send message back to client */
        client_message[read_size] = '\0';
        printf("Message from %lu: %s\n", pthread_self(), client_message );        
        fp = fopen( file , "a" ); /** open or create the log for write */

        /** save the message from client */
        char *m1 = strtok(client_message," ");
        char *m2 = strtok(NULL,"\n");
        int pattern;

        /** check patterns in message */
        if((pattern = patternFinder(m2)) > 0){
            /** handler for recognized pattern  */
            printf("Pattern recognized: %s\n", m2);
        } else {
            /** no patters found. */
            printf("Not a pattern.\n");
        }

        /** getting the id thread  */
        pch = strtok (client_message,"\n");
        if (pch != NULL)
        {
            sprintf(res,"(%lu,%s,%s,%d,%s)\n",pthread_self(),m1,params->ip,pattern,m2);
        }

        /** writing on log file */
        fwrite(res , sizeof(char) , strlen(res) , fp );

        /** close the log file */
        fclose(fp);

    }
    

    /** failed */
    if(read_size == -1)
    {
        perror("recv failed");
    }
         
    /** free the socket pointer */
    free(params->sock);
     
    return 0;
}

/** 
*   Return the code if a pattern was found and -1 if not
*   @params m message from client
*/

int patternFinder(char *m){
    int code = -1;
    if (strcmp(m, "Communication Offline") == 0){
        code = 1;
    } else if (strcmp(m, "Communication error") == 0){
        code = 2;
    } else if (strcmp(m, "Low Cash alert") == 0){
        code = 3;
    } else if (strcmp(m, "Running Out of notes in cassette") == 0){
        code = 4;
    } else if (strcmp(m, "empty") == 0){
        code = 5;
    } else if (strcmp(m, "Service mode entered") == 0){
        code = 6;
    } else if (strcmp(m, "Service mode left") == 0){
        code = 7;
    } else if (strcmp(m, "device did not answer as expected") == 0){
        code = 8;
    } else if (strcmp(m, "The protocol was cancelled") == 0){
        code = 9;
    } else if (strcmp(m, "Low Paper warning") == 0){
        code = 10;
    } else if (strcmp(m, "Printer Error") == 0){
        code = 11;
    } else if (strcmp(m, "Paper-out condition") == 0){
        code = 12;
    } else {
        code = -1;
    }

    if (code > 0) {
        send_mail(m);
    }
    return code;
}

/** 
*   Send alarm message vie email
*   @params m message from client wich contains a pattern
*/

int send_mail(char *m){

    char cmd[100];                      /** variable for command line */
    char to[] = "supervisor@atm.com";   /** email id of the recepient */
    char tempFile[100] = "temp";                 /** name of temp file */

    FILE *fp = fopen(tempFile,"w");     /** open it for writing */
    fprintf(fp,"%s\n", m);              /** write body to it */
    fclose(fp);                         /** close it */

    /** prepare command line */
    sprintf(cmd,"mail -s ALERT  %s < %s",to,tempFile);
    /** execute it */
    system(cmd);    

    return 0;
}

