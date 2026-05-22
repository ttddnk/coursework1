#include <string.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <fcntl.h>

struct User {
    char ip[50];
    char nikname[100];
};

struct User users[100];
int userscol = 0;
char nowip[50];
char nownikname[100];

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

// Функция для установки неблокирующего режима для stdin
void set_nonblocking_input() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void* send_messages(void* arg) {
    int sock = *(int*)arg;
    char ip[50];
    char msg[256];
    struct sockaddr_in dest;
    
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8888);
    
    // Устанавливаем неблокирующий ввод
    set_nonblocking_input();
    
    while (1) {
        printf("\n[Введите IP и сообщение]: ");
        fflush(stdout);
        
        // Пытаемся прочитать ввод без блокировки
        int res = scanf("%s %[^\n]", ip, msg);
        
        if (res == 2) { // Успешно прочитали оба поля
            inet_pton(AF_INET, ip, &dest.sin_addr);
            char nikmsg[356];
            snprintf(nikmsg, sizeof(nikmsg), "[%s]: %s", nownikname, msg);
            sendto(sock, nikmsg, strlen(nikmsg), 0, (struct sockaddr*)&dest, sizeof(dest));
            printf("✓ Отправлено\n");
            fflush(stdout);
        } else if (res == 1) { // Прочитали только IP
            // Очищаем буфер ввода
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("Ошибка: нужно ввести IP и сообщение\n");
        } else if (res == EOF) {
            // Нет данных для ввода, просто ждем
            usleep(100000); // 100 мс
        }
        
        // Небольшая задержка для снижения нагрузки на CPU
        usleep(10000); // 10 мс
    }
    return NULL;
}

int main() {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    char buffer[1024];
    struct sockaddr_in addr;
    
    opruser();
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr*)&addr, sizeof(addr));
    
    pthread_t thread;
    pthread_create(&thread, NULL, send_messages, &s);
    
    while (1) {
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        
        // Устанавливаем таймаут для recvfrom
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100 мс
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(s, &readfds);
        
        int activity = select(s + 1, &readfds, NULL, NULL, &tv);
        
        if (activity > 0 && FD_ISSET(s, &readfds)) {
            recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &from_len);
            printf("\n[%s:%d]: %s\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), buffer);
            printf("[Введите IP и сообщение]: ");
            fflush(stdout);
        }
    }
    
    return 0;
}

