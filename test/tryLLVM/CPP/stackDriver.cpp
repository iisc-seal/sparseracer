#include "stack.h"
using namespace std ;
void main()
{
  typedef Stack<float> FloatStack ;
  typedef Stack<int> IntStack ;

  FloatStack fs(5) ;
  float f = 1.1 ;
  
  while (fs.push(f))
    {
      cout << f << ' ' ;
      f += 1.1 ;
    }

  while (fs.pop(f))


  IntStack is ;
  int i = 1.1 ;

  while (is.push(i))
    {

      i += 1 ;
    }

  while (is.pop(i))

}
