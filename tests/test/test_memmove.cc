#include <iostream>

class Base {
public:
    Base(int v);
    void fun();
protected:
    virtual void fun1();
protected:
    int m_base = 1;
};

Base::Base(int v) : m_base(v){}
void Base::fun() {
    std::cout << m_base << std::endl;
}
void Base::fun1() {
    std::cout << "i am base" << std::endl;
}

class child : public Base {
public:
    child(int v);
    void fun1();
private:
    int m_child = 0;
};

child::child(int v ) : Base(v) {
    m_child = v;
}
void child::fun1() {
    std::cout << "i am child" << std::endl;
}

int main() {

    child* a = new Base(1);
    return 0;
}