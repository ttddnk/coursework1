#include <stdio.h> 
#include <string.h> //strlen, strcpy
#include <sys/socket.h> //socket, sendto, recvfrom(сокеты)
#include <netinet/in.h> //sockaddr_in, htons(сетевые адреса)
#include <arpa/inet.h> //inet_pton, inet_ntoa(для ip адресов)
#include <unistd.h> //тут просто системные вызовы
#include <pthread.h> //потокииии (кароче запуск фуекций одновременно)
#include <stdlib.h> //rand() рандомные числа
#include <time.h> //time(NULL) для разны чисел от времени
#include <ctype.h> //toupper() tolower() isalpha() lkz ,erdx

#define MAXnick 32 //макс длина ника
#define MAXtext 512 //макс длина текста сообщения
#define MAXusers 100 //макс число запоминаемых пользователей

struct Message { //структура сообщ
    char nickname[MAXnick]; //ник
    char text[MAXtext]; //айпи
};

struct User { //хранение
    char ip[16]; //айпи
    char nick[MAXnick]; //ник
};

char mynick[MAXnick]; //наш ник
struct User users[MAXusers]; //все кого знаем
int usercount = 0; //сколько знаем

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
        if (rand() % 100 < 47) c = leet(c); //47% зам на цифру
        if (isalpha(c)) c = (rand() % 2) ? toupper(c) : tolower(c); //случайный регистр
        mod[j++] = c;
    }
    mod[j] = '\0';

    snprintf(buf, size, "%s_%d", mod, rand() % 1000); //добавляем число от 0 до 999 после
}

char* findnick(char *ip) { //ищем ник по айпи, не знаем - сам айпи
    for (int i = 0; i < usercount; i++) {
        if (strcmp(users[i].ip, ip) == 0)
            return users[i].nick;
    }
    return ip;
}

void saveuser(char *ip, char *nick) { //запоминание айпи+ника
    for (int i = 0; i < usercount; i++) { //проверяем есть ли и обновляем
        if (strcmp(users[i].ip, ip) == 0) {
            strncpy(users[i].nick, nick, MAXnick - 1);
            return;
        }
    }

    if (usercount < MAXusers) { //если нет то добавляем
        strncpy(users[usercount].ip, ip, 15);
        strncpy(users[usercount].nick, nick, MAXnick - 1);
        usercount++;
    }
}

void* send_messages(void* arg) //(это значит ф-я используется в потоке)
{
    int sock = *(int*)arg;//arg->указатель->int (номер сокета)
    char input[512]; //теперь input[512] вся строка //msg[256]; //тут просто храним текст
    char ip[50]; //а тут просто ip-адрес
    char text[MAXtext]; //отдельно текст
    struct sockaddr_in dest; //вот это уже структура адреса получателя

    //struct sockaddr_in 
    //{
    //    sa_family_t    sin_family;   // AF_INET (IPv4)
    //    in_port_t      sin_port;     // порт (8888)
    //    struct in_addr sin_addr;     // IP-адрес - ЭТО
    //    char           sin_zero[8];  // для выравнивания
    //};

    //dest.sin_family = AF_INET; семейство адресов
    //dest.sin_port = htons(8888); поле порта
    //dest.sin_addr; поле IP-адреса (in_addr)
    //dest.sin_addr.s_addr; внутри sin_addr (само число IP)

    dest.sin_family = AF_INET; //также IPv4
    dest.sin_port = htons(8882); //ставим порт
    
    printf("укажите свой IP: "); //спрашиваем айпи и даём ник
    fflush(stdout);
    scanf("%49s", ip);
    getchar(); //съедаем Enter после scanf
    printf("ваш ник: %s\n", mynick);

    while (1) 
    {
        printf("user и сообщение ");
        fflush(stdout); //выводим принудителя это без буфферизации
        if (!fgets(input, sizeof(input), stdin)) continue;        //f (!fgets(input, sizeof(input), stdin)) continue;//scanf("%s %[^\n]", ip, msg);//первое до пробела сохраняем в ip второе будет наше сообщение 
        
        sscanf(input, "%49s %[^\n]", ip, text); //разбиваем на айпи и текст

        struct Message msg; //собираем ник+текст
        strncpy(msg.nickname, mynick, MAXnick - 1);
        strncpy(msg.text, text, MAXtext - 1);
        
        inet_pton(AF_INET, ip, &dest.sin_addr); //айпи в число и сохранение
        sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&dest, sizeof(dest)); //шлём
    }
    return NULL;//хз цикл то бесконечный
}

int main() 
{
    srand(time(NULL) ^ getpid()); //генератор случайных чисел запускаяем
    gennick(mynick, sizeof(mynick)); //генерируем сбе ник
    
    int s = socket(AF_INET, SOCK_DGRAM, 0); //IPv4, UDP, по умолчанию
    struct Message msg; //char buffer[1024]; //1024 байта для хранения принятых данных
    struct sockaddr_in addr; //адрес IP и порт

    // struct in_addr 
    // {
    //     uint32_t s_addr;  // IP-адрес в виде числа (например 0x0100007f для 127.0.0.1)
    // };

    addr.sin_family = AF_INET; //адрес будет IPv4
    addr.sin_port = htons(8882); //порт 8882(сетевой порядок байт)
    addr.sin_addr.s_addr = INADDR_ANY;//слушаем все сетевые интерфейсы
    bind(s, (struct sockaddr*)&addr, sizeof(addr));//теперь сокет слушает порт 8888
    
    pthread_t thread; //идентификатор потока 
    pthread_create(&thread, NULL, send_messages, &s); //новый поток
    //&thread - ID потока, адтрибуты стандарт, ф-я робит в новом потоке

    while (1) 
    {
        struct sockaddr_in from;  
        socklen_t len = sizeof(from);
        recvfrom(s, &msg, sizeof(msg), 0,  (struct sockaddr*)&from, &len);//получаем и запоминаем отправителя
        
        char ip_str[16];
        strncpy(ip_str, inet_ntoa(from.sin_addr), 15); //получаем айпи отправ строкой

        saveuser(ip_str, msg.nickname); //запоминаем

        printf("%s: %s\n", msg.nickname, msg.text); //показываем только ник и текст

        // strcpy(buffer, "передача");
        // sendto(s, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, sizeof(addr));
    }   
}