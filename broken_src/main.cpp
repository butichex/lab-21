#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include "room.h"
#include "person.h"
#include "gender.h"

// Используются двоичный семафор и барьер
pthread_mutex_t mutex;
pthread_barrier_t barrier;

// Константы для количества комнат и максимального количества человек
const int kSingleRoomsAmount = 10;
const int kDoubleRoomsAmount = 15;
const int kMaxPersonsAmount = 150;

// Массив комнат
Room *rooms[kSingleRoomsAmount + kDoubleRoomsAmount];

// Пакет с данными, который будем передавать в качестве аргумента.
struct Data {
    // Указатель на человека
    Person *person_ptr;
    // Указатель на файл, в который следует выводить данные
    FILE *out;
    // Флаг, которые указывает, нужно ли выводить данные ещё и в консоль
    bool print_stdout;

    Data() : person_ptr(nullptr), out(nullptr), print_stdout(false) {
    }

    Data(Person *p, FILE *out, bool print_stdout)
        : person_ptr(p), out(out), print_stdout(print_stdout) {
        if (out == stdout) {
            this->print_stdout = false;
        }
    }
};

// Функция, которая ищет первую свободную комнату для человека заданного пола
Room *findRoom(Gender gender) {
    for (Room *room : rooms) {
        if (!room->isFree()) {
            continue;
        }

        if (room->capacity == 1 || room->getRoomGender() == gender ||
            room->getRoomGender() == Gender::NONE) {
            return room;
        }
    }

    return nullptr;
}

//
void *takeRoom(void *thread_parameter) {
    // Получаем аргументы
    Data *data = static_cast<Data *>(thread_parameter);
    // Указатель на клиента
    Person *person = data->person_ptr;
    // Указатель на файл, в который нужно выводить информацию
    FILE *out = data->out;

    // Чтобы избежать ситуации, когда некоторые потоки уже запустились, а другие только создаются,
    // ставим барьер

    // Поток спит случайное время
    usleep(rand() % 100);

    // В один момент времени только один поток может занимать комнату, поэтому
    // используем двоичный семафор

    // Ищем комнату, которую данный клиент может занять
    Room *room = findRoom(person->gender);

    // Если такой комнаты нет, то выводим справочной сообщение, снимаем блокировку с мьютекса
    // и выходим из функции
    if (!room) {
        if (data->print_stdout) {
            fprintf(stdout, "%s guest with ID %d couldn't find an available room and left.\n",
                    person->gender == Gender::MALE ? "Male" : "Female", person->id);
        }
        fprintf(out, "%s guest with ID %d couldn't find an available room and left.\n",
                person->gender == Gender::MALE ? "Male" : "Female", person->id);
        return nullptr;
    }

    // Иначе селим клиента в найденную комнату, выводим справочную информацию
    // и снимаем блокировку с мьютекса
    room->addPerson(person);
    if (data->print_stdout) {
        fprintf(stdout, "%s guest with ID %d takes room number %d.\n",
                person->gender == Gender::MALE ? "Male" : "Female", person->id, room->room_number);
    }
    fprintf(out, "%s guest with ID %d takes room number %d.\n",
            person->gender == Gender::MALE ? "Male" : "Female", person->id, room->room_number);

    // Клиент-поток "отдыхает" некоторое время в комнате
    usleep(rand() % 100 + 10);

    // Затем он собирается выселяться из комнаты. Чтобы не получилось ситуации,
    // когда один поток ещё не выселился, а другой пытается подселиться, ставим блокировку

    // Выселяем клиента из комнаты, выводим справочную информацию и снимаем блокировку
    room->removePerson(person);
    if (data->print_stdout) {
        fprintf(stdout, "%s guest with ID %d left room number %d.\n",
                person->gender == Gender::MALE ? "Male" : "Female", person->id, room->room_number);
    }
    fprintf(out, "%s guest with ID %d left room number %d.\n",
            person->gender == Gender::MALE ? "Male" : "Female", person->id, room->room_number);

    return nullptr;
}

// Функция, которая инициализирует комнаты
void initializeRooms() {
    // Добавляем одиночных комнат
    for (int i = 0; i < kSingleRoomsAmount; ++i) {
        rooms[i] = new Room(1, i + 1);
    }

    // Добавляем комнаты на 2 места
    for (int i = 0; i < kDoubleRoomsAmount; ++i) {
        rooms[kSingleRoomsAmount + i] = new Room(2, kSingleRoomsAmount + i + 1);
    }
}

// Функция для считывания входных данных
int readData(FILE *in, int *male, int *female) {
    // Если указатель на поток для чтения == nullptr, выходим с ошибкой
    if (!in) {
        return 1;
    }

    // Считываем количество мужчин и женщин
    int result = fscanf(in, "%d %d", male, female);

    // Если было считано не 2 значения, выходим с ошибкой
    if (result != 2) {
        return 2;
    }

    // Иначе всё ок, код ошибки = 0
    return 0;
}

// Функция, генерирующая случайные входные данные
void generateRandomData(int *male, int *female) {
    *male = 1 + rand() % (kMaxPersonsAmount / 2 - 1);
    *female = 1 + rand() % (kMaxPersonsAmount / 2 - 1);
}

int main(int argc, char *argv[]) {
    // Переменная для хранения кода опции
    int opt;
    // Переменная для хранения указателя на поток для чтения данных
    FILE *input = stdin;
    // Переменная для хранения указателя на поток для вывода данных
    FILE *output = stdout;

    // Количество мужчин
    int men_count = 0;
    // Количество женщин
    int women_count = 0;

    // Флаг, который показывает, были ли считаны данные из командной строки
    int console_flag = 0;
    // Флаг, который указывает на то, следует ли случайно генерировать входные данные
    int random_flag = 0;
    // Флаг, который указывает на то, следует ли выводить информацию и на экран, и в файл
    int double_print_flag = 0;
    // Семя рандома
    int seed = 42;

    while ((opt = getopt(argc, argv, "rds:i:o:m:f:")) != -1) {
        switch (opt) {
            // Флаг двойного вывода
            case 'd':
                double_print_flag = 1;
                break;
            // Генерация случайного набора
            case 'r':
                random_flag = 1;
                break;
            // Указание входного файла
            case 'i':
                input = fopen(optarg, "r");
                break;
            // Указание выходного файла
            case 'o':
                output = fopen(optarg, "w");
                break;
            // Количество клиентов мужского пола
            case 'm':
                men_count = atoi(optarg);
                console_flag = 1;
                break;
            // Количество клиентов женского пола
            case 'f':
                women_count = atoi(optarg);
                console_flag = 1;
                break;
            // seed для рандома
            case 's':
                seed = atoi(optarg);
                break;
            // На случай ошибки
            case '?':
                return 0;
        }
    }

    // Устанавливаем семя рандома
    srand(seed);

    // Если был запрошен двойной вывод, но не указан выходной файл, сообщаем об ошибке
    if (double_print_flag != 0 && output == stdout) {
        printf("You must specify the path to the output file via the -o option:\n");
        return 0;
    }

    // Выводим приглашение ко вводу в консоль
    if (input == stdin && !random_flag && !console_flag) {
        printf("Enter two non-negative integers separated by a space <men_count> <women_count>:\n");
    }

    // Переменная для хранения кода, возвращаемого функцией для чтения
    int status_code = 0;

    // Генерируем данные случайно, если установлен соответствующий флаг, иначе считываем с
    // файла/консоли
    if (random_flag) {
        generateRandomData(&men_count, &women_count);
        printf("Randomly generated data: men_count=%d, women_count=%d\n", men_count, women_count);
    } else if (!console_flag) {
        status_code = readData(input, &men_count, &women_count);
    }

    // Если код возврата == 1, то не удалось получить доступ к потоку со входными данными
    // Если код возврата == 2, данные введены в неверном формате
    if (status_code == 1) {
        printf("Cannot access stream with input.\n");
        return 0;
    } else if (status_code == 2) {
        printf(
            "The input data is in the wrong format."
            " Make sure the input consists of two integers separated by a space.\n");
        return 0;
    } else if (men_count + women_count > kMaxPersonsAmount || men_count < 0 || women_count < 0) {
        // Выводим в случае, если входные данные не соответствуют диапазону
        printf(
            "The number of women and men in total cannot be more than 150."
            " Also both values must be non-negative\n");
        return 0;
    } else if (!output) {
        // Выводим в случае, если не удалось получить доступ к потоку для вывода данных
        printf("Cannot access output stream.\n");
        return 0;
    }

    // Суммарное количество клиентов
    int persons_number = men_count + women_count;
    // Массив для хранения пакетов, передаваемых потоку
    Data *data = new Data[persons_number];
    // Инициализируем комнаты гостиницы, барьер и мьютекс
    initializeRooms();
    pthread_mutex_init(&mutex, nullptr);
    pthread_barrier_init(&barrier, nullptr, persons_number);

    // Создаем пакеты для потоков: добавляем в них указатель на клиента,
    // а также указатель на поток для вывода данных
    for (int i = 0; i < men_count; ++i) {
        data[i] = Data(new Person(i, Gender::MALE), output, double_print_flag);
    }
    for (int i = 0; i < women_count; ++i) {
        data[men_count + i] =
            Data(new Person(men_count + i, Gender::FEMALE), output, double_print_flag);
    }

    // Создаем массив для хранения потоков
    pthread_t *threads = new pthread_t[persons_number];
    for (int i = 0; i < persons_number; ++i) {
        // Запускаем потоки для выполнения задачи takeRoom,
        // в качестве аргумента передаем соответствующий пакет
        pthread_create(threads + i, nullptr, takeRoom, static_cast<void *>(data + i));
    }

    // Ожидаем завершения каждого из потоков
    for (int i = 0; i < persons_number; ++i) {
        pthread_join(threads[i], nullptr);
    }
    delete[] threads;

    // Удаляем комнаты
    for (Room *ptr : rooms) {
        delete ptr;
    }

    // Удаляем информацию о клиентах
    for (int j = 0; j < persons_number; ++j) {
        delete data[j].person_ptr;
    }
    delete[] data;

    // Удаляем синхропримитивы
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);

    return 0;
}
