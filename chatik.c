#define _GNU_SOURCE
//все не эелементарное не просто
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
#include <net/if.h> //чтоб мультикаст знал на какой интерфейс отправлять
#include <errno.h> //удобно видеть где в фуекции ошибка

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
    MessageType_Hello, //первый элемент энам 0 автоматически так(константа)
    MessageType_HelloResponse, //1
    MessageType_Message, //2
    MessageType_PrivateMessage, //3
} MessageType; //и это будет новый тип

typedef struct _MessageContent { //структура сообщ (как контейнер для данных)
    char nickname[MAXnick]; //ник тут просто массивы
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
    MessageType type; //вот тут храниит тип
    union //объединяем так что разные поля используют одну память
    {
        MessageContent message;
        HelloContent hello;              //олин из трех  в зависимости от тайп
        PrivateMessageContent privmsg;
    };
};

struct User { //хранение другого товарища
    char ip[16]; //айпи
    char nick[MAXnick]; //ник
    time_t last_seen;
    int online;
};



char mynick[MAXnick]; //наш ник ГЛОБАЛЬНО чтобы все функциии видели
struct User users[MAXusers]; //все кого знаем (10) наших пользователец просто массив
int usercount = 0; //сколько знаем
int sock;  //номер сокета
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER; //инкапсуляция типо скрываем данные, мьютекс для защиты массивва

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




char leet(char c) { //приняли символ вернули символ
    switch (c) { //это чтобы без if else...if else... провека с 
        case 'A': case 'a': return '4'; //A в 4 ну к прмеру А похожа на 4:)
        case 'E': case 'e': return '3'; //E в 3
        case 'O': case 'o': return '0'; //O в 0
        case 'T': case 't': return '7'; //T в 7
        case 'S': case 's': return '5'; //S в 5
        default:  return c; //остальные оставили если ничего не подошло
    }
}


void gennick(char *buf, int size) { //генерация ников воид чтобы ничего не венула а указатель на буфер куда записать
    int ac = 0, nc = 0;
    while (adjs[ac]) ac++; //считаем прил (идем до конца)
    while (nouns[nc]) nc++; //считаем сущ

    char base[100]; //склеиваем прид и сущ
    snprintf(base, sizeof(base), "%s%s", adjs[rand() % ac], nouns[rand() % nc]); //выбираем случайное
    char mod[100]; //замена разная
    int j = 0; 
    for (int i = 0; base[i] && j < 99; i++) { //просто цикл по каждому символу
        char c = base[i];//взяли текущ

        if (rand() % 100 < 47) c = leet(c); //47% зам на цифру ну это по приколу
        if (isalpha(c)) c = (rand() % 2) ? toupper(c) : tolower(c); //случайный регистр (не люблю тернарные операторы)

        mod[j++] = c;
    }
    mod[j] = '\0'; //строка обязана заканчивася 0! это фишка си

    snprintf(buf, size, "%s_%d", mod, rand() % 1000); //добавляем число от 0 до 999 после
}




//ищем ник по айпи, не знаем тогда сам айпи
char* findnick(char *ip) {for (int i = 0; i < usercount; i++) {if (strcmp(users[i].ip, ip) == 0) return users[i].nick;} 
return ip; //ник не нашли тогда вернем айпи как строку
} //фуекция вернет указатель на сторку и пример айпи, потом проход по всем известным пользователям 



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
//кароче мы захватываем мьютекст чтобы другой поток не изменил массив одноверенно
    for (int i = 0; i < usercount; i++) { //проверяем есть ли и обновляем(ищем такого же по нику)
        if (strcmp(users[i].nick, nick) == 0) {
            strcpy(users[i].ip, ip);  //обнова для айпи и ника (с ограничением так круче и безопаснее)
            strncpy(users[i].nick, nick, MAXnick - 1);
            users[i].last_seen = time(NULL);  //время обновили
            users[i].online = 1; //поставили что онлайн
            pthread_mutex_unlock(&users_mutex); //свобода мьюексу 
            return;
        }
    }


    if (usercount < MAXusers) //если нет то добавляем (это если место не нашли то надо бы добавить)
    { 
        strncpy(users[usercount].ip, ip, 15);

        strncpy(users[usercount].nick, nick, MAXnick - 1);
        users[usercount].last_seen = time(NULL);
        users[usercount].online = 1;
        usercount++;

        printf("\n%s[%s]%s %s%s%s %s\n",BOLD, "НОВЫЙ",RESET, colornick(nick),nick, RESET, ip);
    }
    pthread_mutex_unlock(&users_mutex); //сообщаем что есть новичок
}

void multicast_send(struct Message *msg) { //это боль и отправка всем в группе
    struct sockaddr_in group_addr;  //просто структура для адреса
    group_addr.sin_family = AF_INET;
    group_addr.sin_port = htons(PORT); //преобразуем в стетевой порядок байт(кароче х.х.х.х в число)
    inet_pton(AF_INET, MULTICAST_GROUP, &group_addr.sin_addr);
    sendto(sock, msg, sizeof(*msg), 0, (struct sockaddr*)&group_addr, sizeof(group_addr)); 
    //привели адрес к нужному типу и оправили по UDP 
}




void setup_multicast() { //тут настройка сокета для приема мультикаста
    int reuse = 1; //разрешим переиспользовать чтобы несколько программ слуышали один порт
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq)); //обновляем структуру, заполняем байты нулями
    inet_pton(AF_INET, MULTICAST_GROUP, &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = INADDR_ANY; //любой интерфецс
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)); //подписываем на мультикаст группу

    struct sockaddr_in addr; //привязываем сокет к порту на всех интерфейсах 
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));




    int ttl = 2; //это время жизни пакета (далеко не убежит) ну максимум два роутера
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
// мне надо для тестирования 1 чтобы самой себе отправлять
    int loop = 1; 
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
}

void* send_messages(void* arg) //(это значит ф-я используется в потоке) универсальный указатель....
{
    int sock = *(int*)arg;//arg->указатель->int (номер сокета) превращаем void* в int* и берём значение
    char input[512]; //теперь input[512] вся строка //msg[256]; //тут просто храним текст (для ввода с клавы)
    char target_nick[MAXnick];
    char text[MAXtext]; //отдельно текст
    struct sockaddr_in dest; //вот это уже структура адреса получателя

    dest.sin_family = AF_INET; //также IPv4
    dest.sin_port = htons(PORT); //ставим порт 8882



    

    while (1) 
    {
        printf("%s[%s%s%s%s]%s ",BOLD, colornick(mynick),mynick, RESET, BOLD, RESET); //вот оно пошло пошло

        fflush(stdout); //принудительно выводим!
        if (!fgets(input, sizeof(input), stdin)) continue; //читаем строку, если ошибка !fgets пропускаем

        
        input[strcspn(input, "\n")] = 0; //strcspn ищет позицию \n и заменяет на 0
        
        if (strcmp(input, "list") == 0) 
        {
            pthread_mutex_lock(&users_mutex);

            printf("\nпользователи в сети: \n");

            for (int i = 0; i < usercount; i++) { //выводим список
                if (users[i].online) {printf("  %s%s%s (%s)\n", colornick(users[i].nick),users[i].nick,RESET, users[i].ip);
                }}
            pthread_mutex_unlock(&users_mutex); //перебираем пользователей
            continue; //мьютекс совободили и гоу дальше
        }
        
        char *space = strchr(input, ' '); //ну элементарно ищем пробел
        if (!space) {printf("формат: ник сообщение\n"); continue;}
        


        *space = 0; //0 вместо проблела нам же надо разделить строку
        strcpy(target_nick, input);
        strcpy(text, space + 1);
        
        pthread_mutex_lock(&users_mutex);
        char target_ip[16] = {0}; //ищем айпишник получателя

        for (int i = 0; i < usercount; i++) { //перебор по никам
            if (strcmp(users[i].nick, target_nick) == 0) {
                strcpy(target_ip, users[i].ip);
                break;
            }
        }
        pthread_mutex_unlock(&users_mutex);
        
        if (target_ip[0] == 0) {
            printf("пользователь %s не найден\n", target_nick);
            continue;
        }
        
        struct Message msg;//обнуляем структуру а то паямть будет замусорена
        memset(&msg, 0, sizeof(msg));

        msg.type = MessageType_PrivateMessage; //заполняем сообщение
        strcpy(msg.privmsg.from, mynick);
        strcpy(msg.privmsg.to, target_nick);
        strcpy(msg.privmsg.text, text);
        
        memset(&dest, 0, sizeof(dest));//готовим адрес получателя
        dest.sin_family = AF_INET;
        dest.sin_port = htons(PORT);
        inet_pton(AF_INET, target_ip, &dest.sin_addr);
        
        printf("[уходит] %s -> %s (%s): %s\n", mynick, target_nick, target_ip, text); //это мои логи потом уберем когда заработает
        
        sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&dest, sizeof(dest)); //отправляет личное сообщение напрямую 
        
    }


    return NULL;
}





int main() 
{
    srand(time(NULL) ^ getpid()); //генератор случайных чисел запускаяем
    gennick(mynick, sizeof(mynick)); //генерируем себе ник
    
    sock = socket(AF_INET, SOCK_DGRAM, 0); //IPv4, UDP, по умолчанию кароче UDP сокет
    setup_multicast(); //настраиваем мультикаст:(
//тут лажа?
    struct timeval tv; //мб сработает я не знаю уже
    tv.tv_sec = 0;
    tv.tv_usec = 500000; //не боимся это 0,5 секунды чтобы recvfrom не ждал вечно
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    system("clear");
    system("figlet -f slant 'Chatik'");
    system("echo ''");



    printf("ваш ник: %s%s%s\n", colornick(mynick), mynick, RESET);
    printf("порт: %d\n", PORT);
    printf("мультикаст группа: %s:%d\n", MULTICAST_GROUP, PORT);
    printf("list - покажет пользователей в сети\n");
    
    pthread_t send_thread; //запусккаем поток для отправки
    pthread_create(&send_thread, NULL, send_messages, &sock);
    
    struct Message msg;
    struct sockaddr_in from;
    socklen_t len = sizeof(from);
    time_t last_hello = 0;//переменные для приема

// сразу хеллоу при запуске
    
    struct Message hello_msg;
    memset(&hello_msg, 0, sizeof(hello_msg));
    hello_msg.type = MessageType_Hello;
    strcpy(hello_msg.hello.nickname, mynick);
    multicast_send(&hello_msg);



    while (1) 
    {
        if (time(NULL) - last_hello >= OBNARUZ_INT) { //если прошло 5 секунд отпарляем хеллоу

        struct Message hello_msg;
        memset(&hello_msg, 0, sizeof(hello_msg));
        hello_msg.type = MessageType_Hello;

        strcpy(hello_msg.hello.nickname, mynick);
            
            multicast_send(&hello_msg);
            last_hello = time(NULL);
            
        pthread_mutex_lock(&users_mutex);
        time_t now = time(NULL);

            for (int i = 0; i < usercount; i++) {
                if (users[i].online && (now - users[i].last_seen) > 30) //если 30 секунд молчит то офлайн

                {users[i].online = 0;
                    // printf("\n%s%s%s (%s) вышел\n",colornick(users[i].nick),users[i].nick, RESET, users[i].ip);
                    }
            }

        pthread_mutex_unlock(&users_mutex);

        }
        
        memset(&msg, 0, sizeof(msg));
        int n = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&from, &len); //ждем 0,5 сек
        
        if (n > 0) { //если что-то получили
        char ip_str[16];
        strcpy(ip_str, inet_ntoa(from.sin_addr)); //то айпи превращаем в строку
        
        if (msg.type == MessageType_Hello) //если хелоу пришел отвечаем на него
        { 
            struct Message response; memset(&response, 0, sizeof(response)); response.type = MessageType_HelloResponse;
            strcpy(response.hello.nickname, mynick); 
            sendto(sock, &response, sizeof(response), 0, (struct sockaddr*)&from, sizeof(from));
            saveuser(ip_str, msg.hello.nickname);

            // printf("\n%s[+]%s %s%s%s (%s) в сети\n"  ,BOLD, RESET, 
            //     colornick(msg.hello.nickname), msg.hello.nickname,RESET, ip_str);
        }

//публичное пока не продумано

        else if (msg.type == MessageType_HelloResponse) {saveuser(ip_str, msg.hello.nickname);} //сохраняем пользователя
        else if (msg.type == MessageType_Message) 
        {

        saveuser(ip_str, msg.message.nickname); 
        printf("\n%s%s%s%s: %s%s\n", 
            colornick(msg.message.nickname),
            msg.message.nickname, 
            RESET,BOLD,msg.message.text, RESET);
        }

        else if (msg.type == MessageType_PrivateMessage) {
        printf("[пришло] от=%s для=%s: %s\n", msg.privmsg.from, msg.privmsg.to, msg.privmsg.text);
            
        if (strcmp(msg.privmsg.to, mynick) == 0)  //если наше сообщение ТО ПОКАЗАТЬ УЖЕ
        {
            saveuser(ip_str, msg.privmsg.from);
            printf("\n%s[личное] %s%s%s: %s%s\n", BOLD, colornick(msg.privmsg.from), msg.privmsg.from, RESET, msg.privmsg.text, RESET);
            fflush(stdout);
        }
        }
    }
}
}