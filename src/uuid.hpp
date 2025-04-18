//
// Created by cory on 4/11/25.
//

#ifndef CPPTEST_UUID_HPP
#define CPPTEST_UUID_HPP


#include <ostream>

class UUID
{
public:
    UUID(unsigned long most, unsigned long least);

    friend std::ostream& operator<<(std::ostream& os, UUID& uuid);
private:
    unsigned long most;
    unsigned long least;
};


#endif //CPPTEST_UUID_HPP
