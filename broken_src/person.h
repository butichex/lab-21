#include "gender.h"

#ifndef IHW4__PERSON_H_
#define IHW4__PERSON_H_

struct Person {
    // Пол клиента
    Gender gender;
    // Уникальный идентификатор клиента
    int id;
    Person(int id, Gender gender) : id(id), gender(gender) {
    }
};
#endif  // IHW4__PERSON_H_
