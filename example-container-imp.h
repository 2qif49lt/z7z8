// stl style collect
#include <functional>
#include <algorithm>
#include <utility>
#include <iostream>
using namespace std;
 template<class t_>
 class test_swap_t
 {
 public:
    typedef t_ value_type;
    typedef test_swap_t<t_> mytype_t;
    typedef value_type* iterator;

     test_swap_t(int n):_size(n),_ptr(_size ? new t_[_size] : nullptr){}
     test_swap_t():_ptr(nullptr),_size(0){}
     ~test_swap_t(){if(_ptr)delete [] _ptr;}
     test_swap_t(const mytype_t& other)
     {
             _size = other._size;
             _ptr = _size ? new t_[_size] : nullptr;
             std::copy(other._ptr,other._ptr+_size,_ptr);
     }
     test_swap_t(mytype_t&& other)
     {
         swap(*this,other);
     }
//   mytype_t& operator=(const mytype_t& other)
     mytype_t& operator=( mytype_t other) // 如果你函数内部需要一个拷贝，则让编译器在函数参数上做，给编译器优化的机会。
     {
    //   mytype_t ret(other); // 当operator参数是复制时这行代码就可以删掉了。
         swap(*this,other) ;
         return *this;
     }
     friend void swap(mytype_t& l,mytype_t& r)
     {
         std::swap(l._ptr,r._ptr);
         std::swap(l._size,r._size);
     }
    t_& operator[] (int idx)
    {
        return _ptr[idx];
    }
    t_ operator[] (int idx) const
    {
        return _ptr[idx];
    }

    iterator begin()
    {
        return _ptr;
    }
    iterator end()
    {
        return _ptr + _size;
    }
    size_t size()
    {
        return _size;
    }
 private:
    int _size;
    t_* _ptr;
 };

 int main(_In_ int _Argc, _In_reads_(_Argc) _Pre_z_ char ** _Argv, _In_z_ char ** _Env)
{   
     test_swap_t<int> t(10);
      int n = 0;
      generate_n(t.begin(),t.size(),bind([](int& i){return ++i;},ref(n)));
      for(auto& a: t)
      {
          cout<<a<<endl;
      }
     
      {
          auto t1 = t;
          for(auto& a: t1)
          {
              cout<<++a<<endl;
          }
      }
      for(auto& a: t)
      {
          cout<<a<<endl;
      }
    return 0;
}