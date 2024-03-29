/**  
* svr_c.c - C client forcommunicate with server 
* @authors  
*   Carlos Ferreira     11-10323
*   Carlos Martínez     11-10584
*   Dayana Rodrigues    10-10615
*/

#include<stdio.h>       /** printf */
#include<string.h>      /** strlen */
#include<sys/socket.h>  /** socket*/
#include<arpa/inet.h>   /** inet_addr */
#include<unistd.h>
#include<stdlib.h>
#include <time.h>
 
/**  
    * Main function with the logic of the client. 
    * @return the code of exit.  
*/

int main(int argc , char *argv[])
{
    /** verifying the command line */
	if (argc != 7) {
        fprintf(stderr,"Uso: %s -d <nombre_modulo_central> -p <puerto_svr_s> -l <puerto_local>\n", argv[0]);
        exit(1);
    }

    int sock, client_sock, bytes_recv;          /** integers for sockt ID and number of received bytes */
    struct sockaddr_in server;                  /** struct for server info */
    char message[1000] , server_reply[2000];    /** variables for messages */
    char number[12];                            
    char localportstr[12];                      /** local port of client */

    char *hostname;  /** hostname of client */

    char *flag_hostname = strdup("-d");     /** flag for hostname */
    char *flag_localport = strdup("-l");    /** flag for local port */
    char *flag_serverport = strdup("-p");   /** flag for server port */

    int localport;  /** local port */
    int serverport; /** server port*/

    int n = 1;
    int k = 2;
    int server_mode = 0;
    int i;

    /** readinng the params from comand line */
    for(i = 0 ; i < 3 ; i++){
        if (strcmp(argv[n],flag_hostname) == 0){
          hostname = strdup(argv[k]);
        } else if (strcmp(argv[n],flag_localport) == 0){
          localport = strtol(argv[k],0,10);
        } else if (strcmp(argv[n],flag_serverport) == 0){
          serverport = strtol(argv[k],0,10);
        } else {
          printf("ERROR: Argumento %s no reconocido.\n", argv[n]);
          exit(1);
        }
        n = n + 2;
        k = k + 2;
    }

    FILE *fp;   /** file of client connections */
    i = 0;      

    /** open the file with the num of previous connections with server */
    fp = fopen("reg.txt" , "r");
    if (fp != NULL){
        fscanf (fp, "%d", &i);
    }

    /** create socket */
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    /** prepare the sockaddr_in structure */ 
    server.sin_addr.s_addr = inet_addr(hostname);
    server.sin_family = AF_INET;

    /** if the client was connected with server before, the client act like server */
    if (i > 0) {

        server_mode = 42;   /** node for server */

        /** variables for connection with server */
        server.sin_port = htons( localport );
        int c , *new_sock;
        struct sockaddr_in client;

        /** binding */
        if( bind(sock,(struct sockaddr *)&server , sizeof(server)) < 0) 
        {
            perror("bind failed. Error");
            return 1;
        }
        puts("bind done");
         
        /** binding */
        listen(sock , 3);
         
        /** accept incoming connection fron server */
        puts("Waiting for incoming connections...");
        c = sizeof(struct sockaddr_in);
        client_sock = accept(sock, (struct sockaddr *)&client, (socklen_t*)&c);
        puts("Connected again");

    /** if the client is connected to server for first time */
    } else {
        /** set server port */
        server.sin_port = htons( serverport );

        /** Connect to remote server */    
        if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
        {
            perror("connect failed. Error");
            return 1;
        }
        /** send to te server the client local port */    
        sprintf(localportstr, "%d", localport);
        write(sock, localportstr, strlen(localportstr));
        puts("Connected");
    }

    /** open the client connections file */
    sprintf(number, "%d", i+1);
    fp = fopen("reg.txt" , "w");

    /** update the nun of connections in file */
    fwrite(number , sizeof(char) , strlen(number) , fp );

    /** close connection file */
    fclose(fp);

    /** keep communicating with server */    
    while(1)
    {
        /** read message from console */ 
        printf("Enter message : ");
        scanf ("%[^\n]%*c", message);
        
        /** variables of connection time */  
        time_t rawtime;
        struct tm * timeinfo;
        
        /** connection info */  
        char output[2000];
        
        /** setting conection time*/ 
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        
        /** building the connection */  
        sprintf(output,"%d:%d:%d,%d-%d-%d %s",timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, message);
        
        printf("Sending message %s\n", output);
        
        /** send some data to server */  
        if(server_mode){
            if( write(client_sock , output , strlen(output)) < 0)
            {
                perror("Send failed");
            }
        } else {
            if( write(sock , output , strlen(output)) < 0)
            {
                perror("Send failed");
                return 1;
            }
        }
    }
    
    /** close socket */  
    close(sock);
    
    return 0;
}
