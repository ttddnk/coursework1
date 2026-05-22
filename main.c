#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h> //потокиии (слушаем и говорим)
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888


void* receive_messages() 
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    char buffer[1024];
    while(1) 
    {
        struct sockaddr_in sender;
        int len = sizeof(sender);
        int n = recvfrom(sock, buffer, 1024, 0, (struct sockaddr*)&sender, &len);
        buffer[n] = 0;
        
        char ip[20];
        inet_ntop(AF_INET, &sender.sin_addr, ip, sizeof(ip));
        
        printf("\n[%s] говорит: %s\n", ip, buffer);
        printf("> ");
        fflush(stdout);
    }
}

int main() {
    printf("чатик запущен!\n");
    
    
    pthread_t thread; //поток для приема сообщений 
    pthread_create(&thread, NULL, receive_messages, NULL);
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0); //сокет для отправки
    
    int broadcast = 1; //широковещательная рассылка
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    
    struct sockaddr_in dest; //адрес всем для всей сети
    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT);
    dest.sin_addr.s_addr = inet_addr("255.255.255.255");
    
    char message[1024];
    while(1) {
        printf("> ");
        fgets(message, sizeof(message), stdin);
        
        message[strlen(message)-1] = 0; // убираем перевод строки, который добавляет fgets

        sendto(sock, message, strlen(message), 0, //отправка
               (struct sockaddr*)&dest, sizeof(dest));
    }
    
    return 0;
}