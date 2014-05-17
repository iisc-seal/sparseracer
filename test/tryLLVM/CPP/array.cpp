int main(){
  int D1[100], D2[20][20], D3[10][10][10], i;
  int j;
  for (i=0; i<10; i++)
    for (j=1; j<9; j++)
      for (int k=2; k<8; k++)
        D3[i][j][k] = 0;

  for (int i=0; i<9; i++)
	D3[i+1][0][0] = D3[i][0][0];
}
