#define _GNU_SOURCE

#include <stdio.h> 
#include <string.h> //strlen, strcpy
#include <sys/socket.h> //socket, sendto, recvfrom(сокеты)
#include <netinet/in.h> //sockaddr_in, htons(сетевые адреса)
#include <arpa/inet.h> //inet_pton, inet_ntoa(для ip адресов)
#include <unistd.h> //тут просто системные вызовы
#include <pthread.h> //потокииии (кароче запуск фуекций одновременно)
#include <stdlib.h> //rand() рандомные числа
#include <time.h> //time(NULL) для разны чисел от времени
#include <ctype.h> //toupper() tolower() isalpha() lkz ,erdh
#include <net/if.h>
#include <errno.h>

// ANSI цвета для терминала
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

#define MAXnick 32 //макс длина ника
#define MAXtext 512 //макс длина текста сообщения
#define MAXusers 10 //макс число запоминаемых пользователей
#define PORT 8882
#define MULTICAST_GROUP "239.0.0.1"
#define OBNARUZ_INT 5

typedef enum _MessageType {
    MessageType_Hello,
    MessageType_HelloResponse,
    MessageType_Message,
    MessageType_PrivateMessage,
} MessageType;

typedef struct _MessageContent { //структура сообщ
    char nickname[MAXnick]; //ник
    char text[MAXtext]; //текст
} MessageContent;


typedef struct _HelloContent {
    char nickname[MAXnick];
} HelloContent;

typedef struct _PrivateMessageContent {
    char from[MAXnick];
    char to[MAXnick];
    char text[MAXtext];
} PrivateMessageContent;

struct Message { //типо полиморфизм и наследование как ооп
    MessageType type;
    union {
        MessageContent message;
        HelloContent hello;
        PrivateMessageContent privmsg;
    };
};

struct User { //хранение другого товарища
    char ip[16]; //айпи
    char nick[MAXnick]; //ник
    time_t last_seen;
    int online;
};



char mynick[MAXnick]; //наш ник
struct User users[MAXusers]; //все кого знаем
int usercount = 0; //сколько знаем
int sock;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER; //инкапсуляция типо скрываем данные 

const char *adjs[] = { //для создания ника части 
    "Shadow","Silent","Cyber","Neon","Dark","Void","Rust","Zero",
    "Alpha","Blind","Chaos","Deep","Evil","Frost","Grim","Harsh",
    "Iron","Jaded","Keen","Lone","Mad","Numb","Pale","Quick",
    "Rogue","Sly","Thin","Vile","Wild","Zoned",NULL
};

const char *nouns[] = {
    "Phantom","Cipher","Ghost","Byte","Kernel","Crash","Root","Null",
    "Agent","Blade","Crow","Drone","Echo","Fox","Grave","Hawk",
    "Imp","Jinx","Key","Lynx","Mist","Nerve","Owl","Pulse",
    "Quake","Raven","Snake","Thorn","Viper","Wolf","Xen","Zeal",NULL
};




char leet(char c) {
    switch (c) {
        case 'A': case 'a': return '4'; //A в 4
        case 'E': case 'e': return '3'; //E в 3
        case 'O': case 'o': return '0'; //O в 0
        case 'T': case 't': return '7'; //T в 7
        case 'S': case 's': return '5'; //S в 5
        default:  return c; //остальные оставили
    }
}


void gennick(char *buf, int size) { //генерация ников
    int ac = 0, nc = 0;
    while (adjs[ac]) ac++; //считаем прил
    while (nouns[nc]) nc++; //считаем сущ

    char base[100]; //склеиваем прид и сущ
    snprintf(base, sizeof(base), "%s%s", adjs[rand() % ac], nouns[rand() % nc]);
    char mod[100]; //замена разная
    int j = 0;
    for (int i = 0; base[i] && j < 99; i++) {
        char c = base[i];

        if (rand() % 100 < 47) c = leet(c); //47% зам на цифру ну это по приколу
        if (isalpha(c)) c = (rand() % 2) ? toupper(c) : tolower(c); //случайный регистр

        mod[j++] = c;
    }
    mod[j] = '\0';

    snprintf(buf, size, "%s_%d", mod, rand() % 1000); //добавляем число от 0 до 999 после
}




//ищем ник по айпи, не знаем тогда сам айпи
char* findnick(char *ip) {for (int i = 0; i < usercount; i++) {if (strcmp(users[i].ip, ip) == 0) return users[i].nick;} 
return ip;
}



char* colornick(char *nick) {
    int sum = 0;
    for (int i = 0; nick[i]; i++) {sum += nick[i];}

    switch (sum % 6) {
        case 0: return RED;
        case 1: return GREEN;
        case 2: return YELLOW;
        case 3: return BLUE;
        case 4: return MAGENTA;
        default: return CYAN;
    }
}

void saveuser(char *ip, char *nick) { //запоминание айпи+ника
    pthread_mutex_lock(&users_mutex); //мьютекс защищает глобальный массив чтоыб из разных потоков не видели

    for (int i = 0; i < usercount; i++) { //проверяем есть ли и обновляем
        if (strcmp(users[i].nick, nick) == 0) {
            strcpy(users[i].ip, ip);
            strncpy(users[i].nick, nick, MAXnick - 1);
            users[i].last_seen = time(NULL); 
            users[i].online = 1; 
            pthread_mutex_unlock(&users_mutex);
            return;
        }
    }


    if (usercount < MAXusers) //если нет то добавляем
    { 
        strncpy(users[usercount].ip, ip, 15);

        strncpy(users[usercount].nick, nick, MAXnick - 1);
        users[usercount].last_seen = time(NULL);
        users[usercount].online = 1;
        usercount++;

        printf("\n%s[%s]%s %s%s%s %s\n",BOLD, "НОВЫЙ",RESET, colornick(nick),nick, RESET, ip);
    }
    pthread_mutex_unlock(&users_mutex);
}

void multicast_send(struct Message *msg) {
    struct sockaddr_in group_addr;
    group_addr.sin_family = AF_INET;
    group_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, MULTICAST_GROUP, &group_addr.sin_addr);
    sendto(sock, msg, sizeof(*msg), 0, (struct sockaddr*)&group_addr, sizeof(group_addr));
}




void setup_multicast() {
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    inet_pton(AF_INET, MULTICAST_GROUP, &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY;
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));




    int ttl = 2;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    int loop = 1;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
}

void* send_messages(void* arg) //(это значит ф-я используется в потоке)
{
    int sock = *(int*)arg;//arg->указатель->int (номер сокета)
    char input[512]; //теперь input[512] вся строка //msg[256]; //тут просто храним текст
    char ip[50]; //а тут просто ip-адрес
    char text[MAXtext]; //отдельно текст
    struct sockaddr_in dest; //вот это уже структура адреса получателя

    dest.sin_family = AF_INET; //также IPv4
    dest.sin_port = htons(PORT); //ставим порт



    

    while (1) 
    {
        printf("%s[%s%s%s%s]%s ",BOLD, colornick(mynick),mynick, RESET, BOLD, RESET);

        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) continue;
        
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "list") == 0) 
        {
            pthread_mutex_lock(&users_mutex);

            printf("\nпользователи в сети: \n");

            for (int i = 0; i < usercount; i++) {
                if (users[i].online) {printf("  %s%s%s \n", colornick(users[i].nick),users[i].nick,RESET);
                }}
            pthread_mutex_unlock(&users_mutex);
            continue;
        }
        
        char *space = strchr(input, ' ');
        if (!space) {printf("формат: ник сообщение\n"); continue;}
        


        *space = 0;
        strcpy(ip, input);
        strcpy(text, space + 1);
        
        pthread_mutex_lock(&users_mutex);
        char target_ip[16] = {0};
        for (int i = 0; i < usercount; i++) {
            if (strcmp(users[i].nick, ip) == 0) {
                strcpy(target_ip, users[i].ip);
                break;
            }
        }
        pthread_mutex_unlock(&users_mutex);
        
        if (target_ip[0] == 0) {
            printf("пользователь %s не найден\n", ip);
            continue;
        }
        
        struct Message msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = MessageType_PrivateMessage;
        strcpy(msg.privmsg.from, mynick);
        strcpy(msg.privmsg.to, ip);
        strcpy(msg.privmsg.text, text);
        
        inet_pton(AF_INET, target_ip, &dest.sin_addr);
        sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&dest, sizeof(dest));
        
    }


    return NULL;
}





int main() 
{
    srand(time(NULL) ^ getpid()); //генератор случайных чисел запускаяем
    gennick(mynick, sizeof(mynick)); //генерируем себе ник
    
    sock = socket(AF_INET, SOCK_DGRAM, 0); //IPv4, UDP, по умолчанию
    setup_multicast();
//тут лажа?
    struct timeval tv; //мб сработает я не знаю уже
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    system("clear");
    system("figlet -f slant 'Chatik'");
    system("echo ''");



    printf("ваш ник: %s%s%s\n", colornick(mynick), mynick, RESET);
    printf("порт: %d\n", PORT);
    printf("мультикаст группа: %s:%d\n", MULTICAST_GROUP, PORT);
    printf("list - покажет пользователей в сети\n");
    
    pthread_t send_thread;
    pthread_create(&send_thread, NULL, send_messages, &sock);
    
    struct Message msg;
    struct sockaddr_in from;
    socklen_t len = sizeof(from);
    time_t last_hello = 0;
    
    struct Message hello_msg;
    memset(&hello_msg, 0, sizeof(hello_msg));
    hello_msg.type = MessageType_Hello;
    strcpy(hello_msg.hello.nickname, mynick);
    multicast_send(&hello_msg);



    while (1) 
    {
        if (time(NULL) - last_hello >= OBNARUZ_INT) {

        struct Message hello_msg;
        memset(&hello_msg, 0, sizeof(hello_msg));
        hello_msg.type = MessageType_Hello;

        strcpy(hello_msg.hello.nickname, mynick);
            
            multicast_send(&hello_msg);
            last_hello = time(NULL);
            
        pthread_mutex_lock(&users_mutex);
        time_t now = time(NULL);

            for (int i = 0; i < usercount; i++) {
                if (users[i].online && (now - users[i].last_seen) > 30) 

                {users[i].online = 0;
                    // printf("\n%s%s%s (%s) вышел\n",colornick(users[i].nick),users[i].nick, RESET, users[i].ip);
                    }
            }

        pthread_mutex_unlock(&users_mutex);

        }
        
        memset(&msg, 0, sizeof(msg));
        int n = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&from, &len);
        
        if (n > 0) {
        char ip_str[16];
        strcpy(ip_str, inet_ntoa(from.sin_addr));
        
        if (msg.type == MessageType_Hello) 
        { 
            struct Message response; memset(&response, 0, sizeof(response)); response.type = MessageType_HelloResponse;
            strcpy(response.hello.nickname, mynick); 
            sendto(sock, &response, sizeof(response), 0, (struct sockaddr*)&from, sizeof(from));
            saveuser(ip_str, msg.hello.nickname);

            // printf("\n%s[+]%s %s%s%s (%s) в сети\n"  ,BOLD, RESET, 
            //     colornick(msg.hello.nickname), msg.hello.nickname,RESET, ip_str);
        }



        else if (msg.type == MessageType_HelloResponse) {saveuser(ip_str, msg.hello.nickname);}
        else if (msg.type == MessageType_Message) 
        {

        saveuser(ip_str, msg.message.nickname);
        printf("\n%s%s%s%s: %s%s\n", 
            colornick(msg.message.nickname),
            msg.message.nickname, 
            RESET,BOLD,msg.message.text, RESET);
        }

        else if (msg.type == MessageType_PrivateMessage) {
            if (strcmp(msg.privmsg.to, mynick) == 0) 
            {
                saveuser(ip_str, msg.privmsg.from);

                printf("\n%s[личное] %s%s%s%s: %s%s\n", BOLD,
                    colornick(msg.privmsg.from), 
                    msg.privmsg.from, RESET,BOLD, msg.privmsg.text, RESET);
            }
        }
        // printf("[бяка] ");
        // printf("%s[%s%s%s%s]%s ", BOLD,colornick(mynick), mynick, RESET,BOLD,RESET);
        fflush(stdout);
        }
    }
}