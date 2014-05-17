int main()
{
  int m, n, p, q, c, d, k, sum = 0;
  int first[10][10]={0}, second[10][10]={0}, multiply[10][10]={0};
 
  
  for (int c = 1 ; c < m ; c++ )
    for (d = 0 ; d < n-1 ; d++ )
      first[c-1][d+1] = 0;
 
  for ( c = 0 ; c < p ; c++ )
    for ( d = 0 ; d < q ; d++ )
      second[c][d] = 0;
 
  for ( c = 0 ; c < m ; c++ )
    for ( d = 0 ; d < q ; d++ ){
      for ( k = 0 ; k < p ; k++ )
	sum = sum + first[c][k]*second[k][d];
      
      multiply[c][d] = sum;
      sum = 0;
    }
  
  
  
      for ( c = 1 ; c < m ; c++ )
	multiply[c][0] = -1;

      for ( d = 0 ; d < q ; d++ )
	multiply[1][d] = 0;
 
	    return 0;
}
