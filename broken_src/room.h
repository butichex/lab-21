#include "person.h"
#include <vector>

#ifndef IHW4__ROOM_H_
#define IHW4__ROOM_H_

struct Room {
    // Клиенты, проживающие в комнате
    std::vector<Person *> persons;
    // Вместимость комнаты
    int capacity;
    // Номер комнаты
    int room_number;

    Room() : capacity(0), room_number(0) {
    }

    Room(int capacity, int room_number) : capacity(capacity), room_number(room_number) {
    }

    // Функция, которая добавляет человека в список жильцов комнаты
    void addPerson(Person *person) {
        persons.push_back(person);
    }

    // Функция, которая выселяет человека из комнаты
    void removePerson(Person *person) {
        size_t pos = 0;
        for (; pos < persons.size(); ++pos) {
            if (persons[pos]->id == person->id) {
                break;
            }
        }

        if (pos >= persons.size()) {
            return;
        }

        persons.erase(persons.begin() + pos);
    }

    // Функция, проверяющая, свободна ли комната
    bool isFree() {
        return persons.size() != capacity;
    }

    // Функция, которая возвращает пол жильцов этой комнаты
    Gender getRoomGender() {
        return persons.empty() ? Gender::NONE : persons[0]->gender;
    }
};

#endif  // IHW4__ROOM_H_
