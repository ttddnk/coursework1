#include <stdio.h> 
#include <string.h> 
#include <sys/socket.h> //socket, sendto, recvfrom(сокеты)
#include <netinet/in.h> //sockaddr_in, htons(сетевые адреса)
#include <arpa/inet.h> //inet_pton, inet_ntoa(для ip адресов)
#include <unistd.h> //тут просто системные вызовы
#include <pthread.h> //потокииии (кароче запуск фуекций одновременно)


// Что такое заголовок пакета и зачем он нужен и что такое тело пакета
// struct Message {
//     char name[100];
//     char message[1000];
// }

// struct Network {}

// network_init(...)
// network_send(net, ...)
// network_recv(net, ...) -> Message

void* send_messages(void* arg) //(это значит ф-я используется в потоке)
{
    //int sock = socket(AF_INET, SOCK_DGRAM, 0); //IPv4, UDP, по умолчанию
    int sock = *(int*)arg;//arg->указатель->int (номер сокета)
    char msg[256]; //тут просто храним текст
    char ip[50]; //а тут просто ip-адрес
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
    
    while (1) 
    {
        printf("укажите IP и сообщение ");
        fflush(stdout); //выводим принудителя это без буфферизации
        scanf("%s %[^\n]", ip, msg);//первое до пробела сохраняем в ip второе будет наше сообщение 
        inet_pton(AF_INET, ip, &dest.sin_addr);//IP в числовой формат и сохранение в 
        sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&dest, sizeof(dest)); //отправка сообщения
        printf("Sended\n");
        fflush(stdout);
    }
    return NULL;//хз цикл то бесконечный
}

int main() 
{
    int s = socket(AF_INET, SOCK_DGRAM, 0); //IPv4, UDP, по умолчанию
    char buffer[1024]; //1024 байта для хранения принятых данных
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
        socklen_t from_len = sizeof(from);
        recvfrom(s, buffer, sizeof(buffer), 0,  (struct sockaddr*)&from, &from_len);//получаем и запоминаем отправителя
        // printf("прием: %s\n", buffer);
        // Message msg = buffer;
        // msg.name
        printf("\n прием от %s:%d: %s\n", inet_ntoa(from.sin_addr), 
           ntohs(from.sin_port), 
           buffer);
        // strcpy(buffer, "передача");
        // sendto(s, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, sizeof(addr));
    }   
}
