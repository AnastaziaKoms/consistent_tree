#include <iostream>
#include <list>

using std::list;

template<class T>
class consistent_list : public list<T> {

};

int main() {
    consistent_list<int> l;
    l.assign({1,2,3,4,5});
    return 0;
}
