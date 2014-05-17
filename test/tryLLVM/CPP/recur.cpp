#include<vector>

long fact(long arg)
{
  std::vector<long long> backpos(10);
  int j;

  if (arg <= 1) return 1;

  j = backpos[1];
  return arg * fact(arg - 1);

}
