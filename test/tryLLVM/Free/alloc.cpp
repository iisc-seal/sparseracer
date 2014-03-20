#include <iostream>

class CCC
{
public:
  CCC(){};
  CCC(int);
  CCC(int, double);
  int ii;
  double dd;
};

CCC::CCC(int _ii)
  : ii(_ii)
{
};

CCC::CCC(int _ii, double _dd)
  : ii(_ii), dd(_dd)
{
};
   
using namespace std;

int main()
{
  CCC *cc1 = new CCC(4, 5.5);   // Pointer. Contructor called.
  CCC *cc2 = new CCC[5];        // Pointer to an array of objects.
  CCC &cc3 = *new CCC;          // Reference
  CCC **c4 = new CCC * [5];     // Array of pointers to pointers

  cc1->ii   = 5;
  cc2[3].ii = 6;
  cc3.ii    = 7;
  c4[0]     = new CCC(8);
  c4[1]     = new CCC(9);

  cout << cc1->ii   << endl;
  cout << cc2[3].ii << endl;
  cout << cc3.ii    << endl;
  cout << c4[0]->ii << endl;
  cout << c4[1]->ii << endl;

  delete cc1;
  delete [] cc2;
  delete & cc3;

  delete [] c4[0];   // First delete pointer content
  delete [] c4[1];
  delete [] c4;      // then delete array of pointers
}
