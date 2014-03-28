#include "stack.h"
#include <iostream>
#include <cstdio>
int main()
{
  typedef Stack<float> FloatStack;
  typedef Stack<int> IntStack;

  IntStack integerStack(6);
  FloatStack fs(5) ;
  float f = 1.1 ;
  int i = 1;
  while (fs.push(f))
    {
      std::cout << f << ' ' ;
      f += 1.1 ;
    }

  while (fs.pop(f))


  
  

  while (integerStack.push(i))
    {

      i += 1 ;
    }

  while (integerStack.pop(i)){}
  
}
