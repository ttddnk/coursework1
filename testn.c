#include <stdio.h> 
#include <string.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

struct User {
    char ip[50];
    char nikname[100];
};

struct User users[100];
int userscol = 0;
char nowip[50];
char nownikname[100];
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER; // Мьютекс для синхронизации вывода

char* getnikname(char* ip) {
    for (int i = 0; i < userscol; i++) {
        if (strcmp(users[i].ip, ip) == 0) {
            return users[i].nikname;
        }
    }
    return NULL;
}

void adduser(char* ip, char* nikname) {
    strcpy(users[userscol].ip, ip);
    strcpy(users[userscol].nikname, nikname);
    userscol++;
}

void opruser() {
    char ip[50];
    char nikname[100];
    char* sushnikname;
    
    printf("Введите ваш ip: ");
    fflush(stdout);
    scanf("%s", ip);
    
    sushnikname = getnikname(ip);
    
    if (sushnikname != NULL) {
        printf("Ваш никнейм: %s\n", sushnikname);
        strcpy(nownikname, sushnikname);
        strcpy(nowip, ip);
    } else {
        printf("Введите ваш никнейм: ");
        fflush(stdout);
        scanf("%s", nikname);
        adduser(ip, nikname);
        strcpy(nownikname, nikname);
        strcpy(nowip, ip);
    }
}

void* send_messages(void* arg) {
    int sock = *(int*)arg;
    char ip[50];
    char msg[256];
    struct sockaddr_in dest;
    
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8888);
    
    while (1) {
        pthread_mutex_lock(&print_mutex);
        printf("\n[Введите IP и сообщение]: ");
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
        
        if (scanf("%s %[^\n]", ip, msg) == 2) {
            inet_pton(AF_INET, ip, &dest.sin_addr);
            char nikmsg[356];
            snprintf(nikmsg, sizeof(nikmsg), "[%s]: %s", nownikname, msg);
            sendto(sock, nikmsg, strlen(nikmsg), 0, (struct sockaddr*)&dest, sizeof(dest));
            
            pthread_mutex_lock(&print_mutex);
            printf("✓ Отправлено\n");
            fflush(stdout);
            pthread_mutex_unlock(&print_mutex);
        } else {
            // Очищаем буфер при ошибке
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            pthread_mutex_lock(&print_mutex);
            printf("Ошибка: нужно ввести IP и сообщение\n");
            fflush(stdout);
            pthread_mutex_unlock(&print_mutex);
        }
    }
    return NULL;
}

void* receive_messages(void* arg) {
    int sock = *(int*)arg;
    char buffer[1024];
    
    while (1) {
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        
        recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &from_len);
        
        pthread_mutex_lock(&print_mutex);
        printf("\n[%s:%d]: %s\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), buffer);
        printf("[Введите IP и сообщение]: ");
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
    }
    return NULL;
}

int main() {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    
    opruser();
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr*)&addr, sizeof(addr));
    
    pthread_t send_thread, recv_thread;
    pthread_create(&send_thread, NULL, send_messages, &s);
    pthread_create(&recv_thread, NULL, receive_messages, &s);
    
    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);
    
    return 0;
}